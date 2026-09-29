#!/usr/bin/env python3
"""Inventory vehicle WORLDBOX metadata from separately extracted Always/Always2 W3Ds.

Uses the chunk/struct layouts in Code/ww3d2/w3d_file.h. No render objects,
textures, game process, or third-party packages are needed. This reports asset
measurements; it does not assign physics-definition IDs or locomotion classes.
"""

import argparse
import hashlib
import json
import math
import struct
from pathlib import Path


def name(raw):
    return raw.split(b"\0", 1)[0].decode("latin1")


def chunks(data, start=0, end=None):
    end = len(data) if end is None else end
    while start < end:
        if end - start < 8:
            raise ValueError("truncated W3D chunk header")
        kind, size = struct.unpack_from("<II", data, start)
        stop = start + 8 + (size & 0x7fffffff)
        if stop > end:
            raise ValueError("W3D child exceeds parent bounds")
        yield kind, data[start + 8:stop]
        if size & 0x80000000:
            yield from chunks(data, start + 8, stop)
        start = stop


def read_vehicle(path):
    data = path.read_bytes()
    boxes, pivots, references = [], [], []
    for kind, payload in chunks(data):
        if kind == 0x740:  # W3D_CHUNK_BOX / W3dBoxStruct
            if len(payload) != 68:
                raise ValueError("unexpected W3dBoxStruct size")
            version, attributes, raw_name, _color, *values = struct.unpack("<II32s4s6f", payload)
            box_name = name(raw_name)
            if box_name.upper().split(".")[-1] != "WORLDBOX":
                continue
            center, extent = values[:3], values[3:]
            if not all(math.isfinite(v) for v in values) or min(extent) <= 0:
                raise ValueError("invalid collision box coordinates")
            boxes.append({"name": box_name, "version": version, "attributes": attributes,
                          "center_xyz": center, "half_extents_xyz": extent,
                          "length_x": 2 * extent[0], "width_y": 2 * extent[1],
                          "height_z": 2 * extent[2]})
        elif kind == 0x102:  # W3D_CHUNK_PIVOTS / W3dPivotStruct
            if len(payload) % 60:
                raise ValueError("partial W3dPivotStruct")
            for offset in range(0, len(payload), 60):
                raw_name, parent, *values = struct.unpack_from("<16sI10f", payload, offset)
                pivots.append({"name": name(raw_name), "parent": parent,
                               "translation": values[:3], "quaternion": values[6:]})
        elif kind == 0x704:  # W3D_CHUNK_HLOD_SUB_OBJECT
            bone, raw_name = struct.unpack("<I32s", payload)
            references.append((name(raw_name).upper(), bone))

    for box in boxes:
        bones = sorted({bone for ref, bone in references if ref == box["name"].upper()})
        # Exact raw extents are vehicle-root extents only when the binding is proven.
        # Do not silently interpret a rotated/animated child box as a root-space box.
        root_identity = False
        if bones == [0] and pivots:
            root = pivots[0]
            root_identity = (root["parent"] == 0xffffffff
                             and root["translation"] == [0.0, 0.0, 0.0]
                             and root["quaternion"][:3] == [0.0, 0.0, 0.0]
                             and abs(root["quaternion"][3]) == 1.0)
        box["hlod_bone_indices"] = bones
        box["vehicle_root_identity_verified"] = root_identity
    return {"file": path.name, "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(), "worldboxes": boxes}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--extracted-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    records, effective = [], {}
    # Game startup mounts Always2 ahead of Always (Code/Commando/init.cpp).
    for archive in ("always", "always2"):
        directory = args.extracted_root / archive
        if not directory.is_dir():
            parser.error(f"missing extracted directory: {directory}")
        for path in sorted(directory.glob("*.w3d"), key=lambda p: p.name.lower()):
            if not path.name.lower().startswith("v_"):
                continue
            record = read_vehicle(path)
            record["archive"] = archive + ".dat"
            records.append(record)
            effective[path.name.lower()] = record

    required = {}
    for role, filename in (("humvee", "v_gdi_humvee.w3d"),
                           ("mammoth_heavy_reference", "v_gdi_mammth.w3d"),
                           ("gdi_harvester", "v_gdi_hrvstr.w3d"),
                           ("nod_harvester", "v_nod_hrvstr.w3d")):
        record = effective.get(filename)
        if record is None or len(record["worldboxes"]) != 1:
            raise ValueError(f"{role}: expected exactly one WORLDBOX")
        if not record["worldboxes"][0]["vehicle_root_identity_verified"]:
            raise ValueError(f"{role}: unverified WORLDBOX hierarchy binding")
        required[role] = record
    report = {"format_version": 1, "units": "Renegade world units (1 metre per WorldMetricsClass)",
              "axes": {"length": "X", "width": "Y", "height": "Z"},
              "measurement": "W3dBoxStruct WORLDBOX half-extents multiplied by two; not visual BOUNDINGBOX",
              "precedence": ["always2.dat", "always.dat"],
              "scope": "W3D asset measurements; runtime physics-definition/class selection remains WorldMetricsClass responsibility",
              "i2_reference_assets": required,
              "records": records}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Measured {len(records)} vehicle W3D variants; wrote {args.output}")
    for role, record in required.items():
        box = record["worldboxes"][0]
        print(f"{role}: L={box['length_x']:.6f} W={box['width_y']:.6f} H={box['height_z']:.6f} ({record['archive']}/{record['file']})")


if __name__ == "__main__":
    main()
