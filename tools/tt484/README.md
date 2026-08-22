# TT 4.8.4 analysis generators

Regenerate the Phase 1 parity deliverables in `docs/tt484/`. Run from the repository
root, in this order:

```sh
python tools/tt484/hooks2.py      # -> docs/tt484/TTHookSites.tsv  (extract)
python tools/tt484/resolve2.py    # -> docs/tt484/TTHookSites.tsv  (add owner columns)
python tools/tt484/classdiff.py   # -> docs/tt484/TTClassDeltas.md
python tools/tt484/files2.py      # -> docs/tt484/TTFileInventory.tsv
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
