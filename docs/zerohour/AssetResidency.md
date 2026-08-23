# Asset exclusion and residency

Roadmap Section 14, port-matrix row **ZH-01**. The first Zero Hour capability that
writes engine code rather than describing one.

Donor sources at the pinned SHA (`9fee97a17`, see `ZeroHourDonorBaseline.md`):

```text
Core/Libraries/Source/WWVegas/WW3D2/w3dexclusionlist.cpp/.h
GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/assetmgr.cpp   Free_Assets_With_Exclusion_List, Create_Asset_List
GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/htreemgr.cpp   Free_All_Trees_With_Exclusion_List
GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/hanimmgr.cpp   Free_All_Anims_With_Exclusion_List, Create_Asset_List
```

Migration class `ADAPTED_BACKPORT`. It is an adaptation rather than a copy for one
reason and it is worth stating plainly: `w3dexclusionlist.cpp` is Westwood code whose
own archive path is `/Commando/Code/ww3d2/w3dexclusionlist.cpp` — this tree's path.
Zero Hour kept a file Renegade shipped without. Bringing it here is closer to
restoring it than to porting it, which is why the naming conventions it assumes are
already the ones stock Renegade content uses.

## What was missing

`WW3DAssetManager` could free everything (`Free_Assets`) or free what nothing
references (`Release_Unused_Assets`). The case a level change actually wants is
neither: free what the level being left owned, keep what the next level needs anyway,
keep permanently what is permanent. Renegade had no way to say that, so
`LevelManager::Release_Level` freed the lot and the next level reloaded assets it had
just thrown away.

## The exclusion list

`Code/ww3d2/w3dexclusionlist.*`. A keep-list of names, and the rules for deciding
whether an asset's name is on it. The rules are the naming conventions the rest of
the engine already uses:

| Name | Rule |
| --- | --- |
| `v_gdi_mrls` | a w3d file; kept when named |
| `v_gdi_mrls.turret` | a sub-object; kept when its **file** is named |
| `v_gdi_mrls#01` | munged at run time; never kept — it belongs to whatever built it |
| `s_a_human.c_ag_gdi` | an animation; kept when the part **after** the dot is named |

The animation rule is the one that is easy to get backwards. An animation is
`<skeleton>.<animname>`, and the part after the dot is the file it came out of, so
matching the skeleton would keep every animation of a kept character and free the
shared ones — exactly inverted.

One deliberate difference from the donor: names are matched without regard to case.
The donor gets away with case-sensitive matching because its keep-lists come from
`Create_Asset_List`, so the cases always agree. Ours will also be built by hand and
from level data, stock content is not consistent about case, and the hierarchy tree
manager already lower-cases every name it hashes. A name that fails to match here
silently frees an asset somebody asked to keep, which is the worst failure this code
has, so it matches the way `HTreeManagerClass` does.

## The residency service

`Code/ww3d2/assetresidency.*`. `AssetResidencyManagerClass` records which scope owns
each asset and turns "release this scope" into a keep-list.

```text
PERMANENT   loaded once, never released   -- HUD, cursor, fonts
GAME_MODE   as long as one mode of play
WORLD       as long as one level
SECTOR      a streamable part of a level
TRANSIENT   the current moment only
```

The order of that enumeration is its meaning. Releasing a scope releases it **and
everything shorter-lived than it**, and keeps everything longer-lived. Releasing
`WORLD` keeps `PERMANENT` and `GAME_MODE`; releasing `SECTOR` also keeps the world.
Releasing `PERMANENT` is releasing everything, and is routed to `Free_Assets`.

Registering a name twice keeps the **longer-lived** of the two scopes. Something the
HUD holds permanently does not stop being permanent because a level also wants it.

Kinds — prototype, hierarchy tree, animation, texture, material, world buffer — do
not change what is released, since the exclusion list works on names. They make the
report readable and tell the dangling-reference check which records it can verify.

### Dependencies

`Add_Dependency(owner, dependent)` says the owner cannot be kept without the
dependent. The keep-list closes over these transitively, so a kept character keeps
its skeleton and the skeleton keeps its animations, whatever scope those were first
seen in. The closure iterates to a fixed point rather than recursing, so a
dependency cycle terminates instead of overflowing the stack — and cycles are real
here, since a skeleton and the things posed by it can each be said to need the other.

### Claiming assets without touching every loader

`Capture_Loaded_Assets(scope)` gives every currently loaded w3d file that nobody has
claimed to that scope. This is what makes the service usable against stock Renegade
content: the level loader does not announce what it loads, and much of it arrives
through load-on-demand, so asking the asset manager afterwards is the only account
that is actually complete.

## How it is wired

| Site | Call |
| --- | --- |
| `Code/Commando/init.cpp`, after `EncyclopediaMgrClass::Initialize` | `Capture_Loaded_Textures(ASSET_SCOPE_PERMANENT)` |
| `Code/Commando/combatgmode.cpp`, end of `Load_Level` | `Capture_Loaded_Assets(ASSET_SCOPE_WORLD)`, then `Log_Report` under `WWDEBUG` |
| `Code/Commando/level.cpp`, `LevelManager::Release_Level` | `Release_Scope(ASSET_SCOPE_WORLD)` |

`Release_Scope` with nothing retained calls `Free_Assets`, not
`Free_Assets_With_Exclusion_List` with an empty list. Those are not the same:
`Free_Assets` also drops the fonts and every texture reference, while the exclusion
path releases only unreferenced textures. On a dedicated server nothing renders, so
nothing is resident to claim, and a level release there behaves exactly as it always
has.

## What the permanent scope claims

`Capture_Loaded_Textures(ASSET_SCOPE_PERMANENT)` runs once at the end of startup, and
what it claims is simply **whatever the asset manager is holding a texture for before
the first level loads** -- the HUD reticles, the cursor, the dialog art, the font
pages. There is no manifest, no list of names in a header, and nothing for a stock
installation to author: the permanent set is defined by when it loaded, not by what it
is called.

That claim is what turns the level release from `Free_Assets` into the exclusion path,
and the win is not in the keep-list -- it is in what the exclusion path *doesn't* do.
It leaves `Font3DDatas` and `FontChars` alone, so the font caches are no longer thrown
away and rebuilt for every level, and it leaves `TextureHash` alone, so a texture the
HUD is still holding stays known to the manager instead of being reloaded from disk as
a second copy under the same name.

Prototypes are deliberately **not** claimed at startup. A menu's 3D content would be
kept for the life of the process for no benefit, and prototypes are where the memory
actually is. Nothing yet claims a prototype permanently, so every prototype is still
freed on a level change -- which is the correct behaviour until something is genuinely
shared between levels.

### Why a texture is retained without being named

The exclusion list matches w3d file names. A texture, a material and a generated world
buffer have no name it can match, and putting one in the keep-list would ask it a
question it has no way to answer -- so `Build_Retained_List` emits names only for the
three kinds it can act on. Those assets are retained the way they have always been
retained, by reference count: `Free_Assets_With_Exclusion_List` ends in
`Release_Unused_Textures`, which frees only what nothing else holds.

That splits one question into two. *Is anything retained* is a question about records
and is answered by `Get_Retained_Count`. *Which names must survive* is a question about
the exclusion list and is answered by `Build_Retained_List`. A permanent scope holding
nothing but the user interface answers one and not the other, and `Release_Scope` has
to ask the first: it is still being asked to leave the fonts and the texture hash
alone.

## Renegade asset compatibility

Nothing here changes how an asset is loaded, parsed or named, and no `.w3d`, `.mix`,
`.dds`, `.tga` or level file has to be re-exported or re-authored. The exclusion
rules read the conventions stock content already follows; the residency service
learns what is loaded by asking the asset manager rather than by reading a manifest
somebody would have to write. An asset nobody ever registers behaves precisely as it
does today.

## Diagnostics

Section 14 asks for four, and the report prints all four:

- **live prototypes** — `WW3DAssetManager::Prototype_Count()`, added for this;
- **retained assets** — per scope and per kind;
- **dangling references** — tracked prototypes the asset manager no longer holds,
  which means something released an asset out from under the scope that claimed it;
- **per-scope memory** — summed from recorded sizes; `Register_Texture` takes the
  real figure from `TextureClass::Get_Texture_Memory_Usage`.

## What is checked

`renegade -assetselfcheck exclusion` and `-assetselfcheck residency`, registered as
the ctest entries `asset_exclusion` / `asset_residency` and their `fds_` twins. Both
are pure decisions over names, so they need no device, no window, no level and no
game data, and they run before anything is initialised — the arrangement the script
catalog check and the 4.8.4 gate already use.

`asset_residency` also covers the split above: that a retained texture is not named in
the keep-list, that a scope holding only textures reports one retained record and zero
retained names, and that the record counts and the name counts diverge in the way the
kinds say they should. It ends by running sixteen load/release cycles and asserting that the
record count, the dependency count, the retained permanent asset and the accounted
memory are all exactly where they started. That is Section 14's acceptance condition
stated as arithmetic. What the checks do **not** cover is the release itself, which
needs a loaded level and is a runtime check rather than a ctest one.

## Not taken

- **SAGE prototype / `Thing` binding.** The rejected subpart named in the ZH-01 row.
  OpenW3D has definitions and its own game loop; importing the donor's binding of
  render prototypes to `ThingTemplate` would mean keeping two ownership models,
  which directive 0.4 forbids.
- **`PrototypeClass::DeleteSelf()`.** OpenW3D deletes prototypes with `delete`, which
  is what `Free_Assets` already does. A second destruction protocol would leave two
  ways to destroy a prototype and no rule about which one a caller owes.
- **`Core/Tools/assetcull` as a build step.** It culls an asset set offline against a
  SAGE map format OpenW3D does not read. The residency service does the same job at
  run time against the assets actually loaded — also the only form that can work with
  unmodified `.mix` content.
- **Freeing a referenced animation.** `Free_All_Anims_With_Exclusion_List` releases
  only animations the manager alone holds. One that something is still playing is
  kept whatever the keep-list says; the alternative is a dangling pointer inside a
  live `Animatable3DObj`.

All four are in `docs/RejectedItems.md`, which is generated.

## What is left

`PERMANENT` and `WORLD` both have callers. What is left belongs to later work rather
than here:

1. `GAME_MODE` needs a caller at mode entry and exit;
2. `SECTOR` has no consumer until there is something that streams part of a level,
   which is the terrain and procedural-world work, not this;
3. nothing claims a prototype permanently, on purpose (above). The first real candidate
   is whatever ends up shared between every level -- the player characters, most
   likely -- and that is a decision to make against a profile, not in advance;
4. materials and generated world buffers have record kinds and no registration site,
   for the same reason `SECTOR` has no consumer.

The permanent claim is verified as arithmetic by `asset_residency`; confirming the
resident texture count and the font caches actually survive a map change is a runtime
observation of the `Log_Report` line, not a ctest.
