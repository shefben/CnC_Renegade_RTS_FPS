# Vehicle collision dimensions for hybrid rewrite I2

Measured on 2026-09-27 from `C:/Westwood/Renegade_full/Data/always.dat` and
`Always2.dat`, as requested by the user. The full inventory is
[VehicleCollisionMetrics.json](VehicleCollisionMetrics.json); the reusable reader is
`tools/w3d_vehicle_metrics.py`.

## Measured reference assets

Lengths are Renegade world units (metres per `WorldMetricsClass`), with X forward,
Y lateral, and Z vertical. Values below are rounded to six decimal places; JSON
retains the decoded float values, half-extents, centre offsets, source archive,
W3D SHA-256, collision attributes, and hierarchy-binding evidence.

| Asset | Length X | Width Y | Height Z | Effective archive |
| --- | ---: | ---: | ---: | --- |
| `v_gdi_humvee.w3d` | 4.722971 | 2.555953 | 1.548488 | always.dat |
| `v_gdi_mammth.w3d` | 10.500445 | 6.801906 | 3.064066 | always.dat |
| `v_gdi_hrvstr.w3d` | 7.428105 | 4.514225 | 2.754598 | always.dat |
| `v_nod_hrvstr.w3d` | 7.399116 | 4.514225 | 2.754598 | always2.dat |

Each measurement is `2 * W3dBoxStruct.Extent` from the named `WORLDBOX` chunk
(`0x740`, 68-byte payload), not the model's visual `BOUNDINGBOX`. All four have
physical collision enabled and HLOD references to bone 0; the corresponding
root pivots have identity rotation and zero translation. Thus these box dimensions
are already in vehicle-root coordinates. Off-centre collision boxes retain their
actual centre offsets; height is the box's full Z extent, not its top coordinate.

`Code/ww3d2/w3d_file.h` defines the decoded structs. Game startup mounts Always2
ahead of Always (`Code/Commando/init.cpp`); both source variants are retained in
the inventory. The Nod Harvester's Always2 override has the same WORLDBOX
dimensions as the original, although the W3D files differ.

The Mammoth row supplies the measured heavy-reference asset, not a new rule for
classifying all loaded vehicles. I2 must still resolve required physics definitions
and the canonical heavy-ground aggregate through `WorldMetricsClass`; do not infer
definition IDs or locomotion classes from these asset filenames. Both faction
Harvesters are recorded rather than substituting an arbitrary vehicle width.

## Derived Section 0.5 values

Using the actual Humvee lateral collision width and one world unit per metre:

```text
H = 2.555953264
C = max(0.35, 0.15 * H) = 0.383392990
W = max(H + 2*C, 1.30*H) = 3.322739244
```

The audited plan's `H=2.556` and `W=3.3228` are rounded reference values, not
replacement measurements. Feed the measured extents through the shared helpers
and apply the plan's quantization at its specified boundaries.

## Reproduce

Run from the OpenW3D repository root:

```powershell
python tools/always_dat_extract.py extract-all C:/Westwood/Renegade_full/Data/always.dat build/asset_metrics/always .w3d
python tools/always_dat_extract.py extract-all C:/Westwood/Renegade_full/Data/always2.dat build/asset_metrics/always2 .w3d
python tools/w3d_vehicle_metrics.py --extracted-root build/asset_metrics --output docs/procedural_world/VehicleCollisionMetrics.json
```

Extraction produced 3,069 and 21 W3D files respectively. The inventory covers 63
`v_*.w3d` variants, including records without a WORLDBOX; filenames are enumerated
from extracted content. All 63 were compared byte-for-byte with their archive
entries. The reader rejects truncated chunks, invalid box extents, and unverified
root bindings for the four required reference assets. Extracted copyrighted assets
remain under ignored `build/asset_metrics/`; only measurements and tooling belong
in source control.

These are verified offline measurements. No hard-coded dimension fallback or
runtime manifest consumer was added by this extraction.
