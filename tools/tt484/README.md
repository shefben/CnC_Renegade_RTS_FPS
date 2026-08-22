# TT 4.8.4 analysis generators

Regenerate the Phase 1 parity deliverables in `docs/tt484/`. Run from the repository
root, in this order:

```sh
python tools/tt484/hooks2.py      # -> docs/tt484/TTHookSites.tsv  (extract)
python tools/tt484/resolve2.py    # -> docs/tt484/TTHookSites.tsv  (add owner columns)
python tools/tt484/classdiff.py   # -> docs/tt484/TTClassDeltas.md
python tools/tt484/files2.py      # -> docs/tt484/TTFileInventory.tsv
python tools/tt484/defcheck.py    # -> docs/tt484/TTMethodSources.tsv
python tools/tt484/counterpart.py # -> stdout; checks the NEW dispositions
```

`files2.py` must run after `resolve2.py`; it reads `TTHookSites.tsv` to place files
that only resolve through the hook sites they install.

`writedoc.py` is the markdown template used by `classdiff.py`; it is not run
directly.

`hooks2.py` scans `tt_4.8.4/{tt,scripts,ttinit,VFS,MemoryManager,Launcher}` for hook
installation sites. It must know every primitive in `tt/hooksupport.h` — the bare
functions *and* the inline-assembly macros (`hookAsJump`, `hookAsCall`,
`hookAsCall2`, `hookNop`, `hookVtable`, `typedHookAsJump`). Counting only the bare
functions undercounts the tree roughly fivefold. If `hooksupport.h` gains a
primitive, add it to `PRIMS`.

`resolve2.py` maps each site's TT replacement symbol onto an OpenW3D definition by
indexing `Code/`. Sites that patch raw bytes have no symbol and are mapped from
their intent comment through the `BYTE_MAP` / `FREE_MAP` / `PER_SITE` tables at the
top of the file — extend those rather than hand-editing the TSV, which is
regenerated wholesale.

Both `resolve2.py` and `classdiff.py` prefer an out-of-line definition over a call
site, and `classdiff.py` compares methods on name + arity so that parameter-name
differences and `override` do not create phantom deltas.

`files2.py` covers every C/C++ file in `tt_4.8.4/` and gives each a role and a
planned native destination. Four things in it are load-bearing and easy to break:

- `REJECT` drops `Code/sndapi`, `Code/fastalloc_test` and `Code/dxvk_wrapper` from
  candidate lookups entirely. They vendor verbatim copies of engine classes
  (`FileClass`, `CRC32`, `FastAllocator`), so leaving them in makes them win
  lookups against the library that actually owns the class. `SECONDARY` demotes
  tools and tests for the same reason without excluding them, since a tool can be a
  legitimate destination.
- TT has **no `DECLARE_SCRIPT` macro**. A gameplay script is
  `class X : public ScriptImpClass`, registered with `ScriptRegistrant<X>`.
  Searching for `DECLARE_SCRIPT` matches nothing in this tree.
- `UBIQUITOUS` suppresses include-fan-out votes from headers more than a twelfth of
  the tree includes (`Types.h`, `Singleton.h`, `engine.h`). They carry no locality
  information and otherwise drag unrelated files toward whichever bucket the common
  header landed in.
- `render/shader integration`, `physics/collision API` and `save/load` are assigned
  from the destination directory, never from content. `RenderObjClass`, `PhysClass`
  and `ChunkSaveClass` appear as members throughout the gameplay tree; scoring on
  them labels most of `Code/Combat` by whatever it holds a pointer to.

Hand corrections go in `PER_FILE`, not in the TSV, which is regenerated wholesale.

Output is deterministic; re-running with an unchanged tree reproduces the files
byte for byte.

`ttparse.py` holds the shared C++ declaration parsing (`find_class`, `members`,
`scan_classes`). It is imported, never run. One subtlety: `METH` allows an empty
return type so constructors parse, which also matches a bare call statement inside
an inline body — `Set_Object_Dirty_Bit(BIT_RARE, true);` reads as a declaration.
`members()` therefore takes the class name and rejects an empty-return-type match
that is neither the constructor nor the destructor. Callers must pass it.

`defcheck.py` answers the question `classdiff.py` cannot: for each TT-only method,
where is the body? A TT `scripts/` header is a link-time interface to a binary we
do not have, so a TT-only declaration is not TT-only code and is not necessarily
work at all. Rows land in one of `defined`, `inline-header`, `address-stub`
(`RENEGADE_FUNCTION ... AT2(...)`, a naked thunk into the shipped binary — no TT
implementation exists), `pure-virtual`, or `declaration-only`. Declaration-only
rows are split again by whether OpenW3D already declares that method name on some
other class, since `classdiff.py` only ever compares same-named classes. Output is
`docs/tt484/TTMethodSources.tsv`; the reading is in parity matrix 5.6.

`counterpart.py` finds the best OpenW3D counterpart for a TT class by member-name
overlap, so a `NEW` disposition can be checked rather than inferred from the name
not matching. It scores containment (`shared / TT members`), not Jaccard: a TT
header re-declares only what TT's scripts call, so the right OpenW3D class is
usually much larger and Jaccard would rank it below a coincidental small match.
Run it with no arguments to re-check every class the matrix calls NEW. A flat
score across several unrelated classes means the overlap is only base-class
boilerplate, i.e. genuinely new.
