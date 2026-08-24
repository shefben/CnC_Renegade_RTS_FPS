# Missing assets

Art that engine code already asks for and that nobody has made yet.

Every file in this folder is a list of assets the running game would load if they existed,
with the exact name the code passes to the asset manager, what it is for, and the
constraints the geometry it goes on imposes on it. When the art gets made, dropping a file
with the listed name where Renegade looks for textures is the whole of the work — nothing
in the engine has to change to pick it up.

## The rule this folder exists for

**A missing asset is never a reason to skip the code.** Write the system, have it name the
asset it wants, let it behave sanely when the asset is absent, and record the name here.
The alternative — leaving a feature unwritten until somebody paints a texture — puts the
engine work behind the art work for no reason, and loses the one moment when it is obvious
what the art has to be.

"Behave sanely when absent" means what it says in each case: untextured geometry, a layer
with a rule and no content, a model slot that draws nothing. Not a crash, not a blocking
error, and not a stand-in file checked into the tree that somebody later ships by accident.

## How names resolve

Textures are fetched by bare filename through `WW3DAssetManager::Get_Texture(name)`, which
searches the mix files (`always.dat` and the level's own) and then loose files under the
game directory. Stock Renegade art is `.tga`, and so is everything listed here.

Names in this folder are prefixed `ow_` so that nothing here can ever collide with a stock
Renegade asset name. Stock asset compatibility is a hard rule: original models, maps and
textures load unmodified, and no name we invent may shadow one of theirs.

## The lists

| File | Covers | Count |
|------|--------|-------|
| [TerrainLayers.md](TerrainLayers.md) | Runtime terrain texture layers (roadmap Section 18) | 6 textures |
| [RoadSurfaces.md](RoadSurfaces.md) | Runtime road surfaces (roadmap Section 19) | 3 textures |
| [BridgeSections.md](BridgeSections.md) | Sectional bridge models (roadmap Section 20) | 60 models, 3 debris |
| [FoliageModels.md](FoliageModels.md) | Batched foliage models and impostors (roadmap Section 21) | 5 models, 5 textures, 5 impostors |
| [WaterModels.md](WaterModels.md) | Water surface textures (roadmap Section 22) | 6 textures |
| [RibbonSurfaces.md](RibbonSurfaces.md) | Ground mark textures -- tracks, scorches, drag marks (roadmap Section 23) | 5 textures |

## Format of an entry

Each entry gives:

- **Name** — the exact string the code passes, or would be given;
- **What it is for** — the thing on screen, in words an artist can act on;
- **Where it is read** — file and function, so the claim can be checked;
- **Without it** — what happens today, so the gap's cost is honest;
- **Constraints** — tiling, projection, aspect, anything about the geometry that decides
  whether a given image will read correctly on it. This is the part worth writing down
  now, while the code that imposes the constraint is being written.
