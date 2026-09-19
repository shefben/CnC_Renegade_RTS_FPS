# Cumulative automated-test coverage catalog

Roadmap Section 48.1/48.2, Phase 42.

## What this file is, and is not

Section 48.1: *"cumulative catalog, not a per-session checklist."* This file exists so a
session can ask "does a family like this already run?" by reading one table instead of
re-deriving coverage from `CMakeLists.txt` and `consolefunction.cpp` every time. It is
**not** re-verified per edit, and finding a row here is not permission to skip a targeted
check a change actually calls for.

It is generated once, by reading `add_test(NAME ...)` in `Code/Commando/CMakeLists.txt`,
`Code/WWAudio/CMakeLists.txt` and `Code/wwlib/CMakeLists.txt`, and cross-referencing the
family names against Section 48.2's required list. Update it — do not regenerate it from
scratch — whenever a new `-shaderselfcheck`/`-ttselfcheck`/`-terrainselfcheck`/
`-assetselfcheck` family or ctest entry is added.

**Family-level, not bullet-level.** A row below means *a registered automated check runs
under that name*, checked against `Code/Commando/CMakeLists.txt`. It does not mean every
individual bullet Section 48.2 lists under that family's heading has its own separate pin
inside that check — confirming that would mean reading every self-check function body,
which is exactly the "recursively audit unrelated subsystems" this project's anti-drift
rule says not to do. Where a family is registered, its own design doc (linked in the
existing per-system `docs/zerohour/*.md` files) is the authority on what it actually pins.

## Registered families

Client (`renegade -X selfcheck Y`) and dedicated server (`renegadeserver`, prefixed
`fds_`) each get their own ctest entry for most families; both are device-less and run in
CI. Counts below are the client-side family count; assume a matching `fds_` twin unless
noted.

| Section 48.2 group | Registered families (ctest name, client side) |
|---|---|
| TT | `scripts_catalog_stock`, `scripts_catalog_tt`, `scripts_catalog_duplicates`, `scripts_catalog_recreate`, `tt_objects`, `tt_visibility`, `tt_hooks`, `tt_dialogs`, `tt_collision`, `tt_modules` |
| Asset residency | `asset_exclusion`, `asset_residency` |
| Zero Hour / SAGE donor (terrain family) | `terrain_sampling`, `terrain_rays`, `terrain_shaping`, `terrain_service`, `terrain_collision`, `terrain_textures`, `terrain_roads`, `terrain_bridges`, `terrain_foliage`, `terrain_water`, `terrain_ribbons`, `terrain_marks`, `terrain_shadows`, `terrain_lights`, `terrain_far`, `terrain_shroud` |
| Zero Hour / SAGE donor (renderer family) | `shader_programs`, `shader_pipelines`, `shader_particles`, `shader_weather`, `shader_tracers`, `shader_debris`, `shader_attachments`, `shader_radar`, `shader_overlays`, `shader_debugrender` |
| AI / telemetry | `shader_guardai`, `shader_telemetry` |
| Building JSON / power / capabilities | `shader_buildingjson`, `shader_powergrid` |
| FPS-first Commander invariants | `shader_commandermode` |
| Commander UI / placement | `shader_sidebar`, `shader_placement` |
| Procedural construction | `shader_construction` |
| Commander AI production/control | `shader_production` |
| Save/load/networking | `shader_rtssaveload` |
| Engine-utility (not Section 48.2, kept for completeness) | `wwaudio_lifetime_tests`, `wwlib_mempool_tests` |

Roughly 90 ctest entries total (client + `fds_` server pairs plus the two utility suites),
all device-less, all runnable in the same CI pass that currently exists.

## Gaps against Section 48.2's literal bullet list

Reading the *names* above against 48.2's bullets, three TT bullets have no obviously
matching family name:

- **"script save/load"** — no `tt_*` family names save/load explicitly; `tt_objects`
  may cover it incidentally, unconfirmed without opening its source (see the note above
  on why that is not done here).
- **"TT network dirty state"** — no obvious match; candidate for `tt_hooks` or a new
  family.
- **"TT building definitions" / "TT factory definitions"** — no obvious match distinct
  from `shader_buildingjson`, which is the Commander/Zero-Hour-donor building schema, not
  TT's own building/factory definition tables.

These are recorded as gaps, not assumed covered. Closing them is ordinary TT-phase
(P01-P06) work, not a Section 48 task in itself — this file's job is to make the gap
visible, not to fill it.

Every other 48.2 heading has at least one family above under it.

## What Phase 42 does not do

- Does not add new test coverage — Section 0B still governs when a test gets written.
- Does not run any of the ~90 entries; none of them were executed to produce this catalog,
  only enumerated from `CMakeLists.txt`. Running the suite is ordinary CI/validation work,
  not part of building the catalog.
- Does not attempt Section 48.3's golden runtime scenarios; those need a live game and are
  out of reach of the standing no-run constraint this project is under.
