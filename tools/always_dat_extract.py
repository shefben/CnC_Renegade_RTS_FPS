#!/usr/bin/env python3
"""Reader/extractor for Renegade's `MIX1` archive format (e.g. always.dat).

Reimplements the read side of `Code/wwlib/mixfile.cpp` (`MixFileFactoryClass`)
in Python so reference art can be pulled out of a retail install without a
running game. Layout (see `MIXFILE_HEADER` / `FileInfoStruct` in mixfile.cpp):

    offset 0:  char signature[4]      "MIX1"
    offset 4:  int32 header_offset    -> absolute offset of the file table
    offset 8:  int32 names_offset     -> absolute offset of the name table
    header_offset:  int32 file_count, then file_count * (u32 crc, u32 offset,
                    u32 size), sorted ascending by crc. `offset`/`size` are
                    absolute byte ranges in the archive (data starts right
                    after the 16-byte header).
    names_offset:   int32 file_count, then that many (u8 len, len bytes
                    latin1, null-terminated) filenames, in original add order
                    (NOT the crc-sorted order of the file table above).

Lookup by name recomputes `CRC_Stringi` (`Code/wwlib/realcrc.cpp`): standard
CRC-32 of the name upper-cased, i.e. `zlib.crc32(name.upper().encode())`.

Usage:
    python always_dat_extract.py list <always.dat> [substring]
    python always_dat_extract.py extract <always.dat> <out_dir> <name> [name...]
    python always_dat_extract.py extract-all <always.dat> <out_dir> <substring>
"""
from __future__ import annotations

import os
import struct
import sys
import zlib


class MixFile:
    def __init__(self, path: str):
        self.path = path
        self.f = open(path, "rb")
        sig, header_offset, names_offset = struct.unpack("<4sii", self.f.read(12))
        if sig != b"MIX1":
            raise ValueError(f"{path}: not a MIX1 archive (signature {sig!r})")
        self.header_offset = header_offset
        self.names_offset = names_offset

        self.f.seek(header_offset)
        (file_count,) = struct.unpack("<i", self.f.read(4))
        self.file_count = file_count
        raw = self.f.read(file_count * 12)
        self.crc_map = {}
        for i in range(file_count):
            crc, off, size = struct.unpack_from("<III", raw, i * 12)
            self.crc_map[crc] = (off, size)

        self.f.seek(names_offset)
        (name_count,) = struct.unpack("<i", self.f.read(4))
        names = []
        for _ in range(name_count):
            (nlen,) = struct.unpack("<B", self.f.read(1))
            raw_name = self.f.read(nlen)
            names.append(raw_name.split(b"\x00", 1)[0].decode("latin1"))
        self.names = names

    @staticmethod
    def crc_stringi(name: str) -> int:
        return zlib.crc32(name.upper().encode("latin1")) & 0xFFFFFFFF

    def find(self, name: str):
        return self.crc_map.get(self.crc_stringi(name))

    def read(self, name: str):
        hit = self.find(name)
        if hit is None:
            return None
        off, size = hit
        self.f.seek(off)
        return self.f.read(size)


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 1
    cmd, mix_path = argv[1], argv[2]
    mf = MixFile(mix_path)

    if cmd == "list":
        needle = argv[3].lower() if len(argv) > 3 else None
        for n in sorted(mf.names):
            if needle is None or needle in n.lower():
                print(n)
        return 0

    if cmd == "extract":
        out_dir = argv[3]
        os.makedirs(out_dir, exist_ok=True)
        missing = []
        for name in argv[4:]:
            data = mf.read(name)
            if data is None:
                missing.append(name)
                continue
            with open(os.path.join(out_dir, name), "wb") as fh:
                fh.write(data)
        print(f"extracted {len(argv[4:]) - len(missing)} of {len(argv[4:])}")
        if missing:
            print("missing:", missing)
        return 1 if missing else 0

    if cmd == "extract-all":
        out_dir = argv[3]
        needle = argv[4].lower()
        os.makedirs(out_dir, exist_ok=True)
        count = 0
        for n in mf.names:
            if needle in n.lower():
                data = mf.read(n)
                if data is not None:
                    with open(os.path.join(out_dir, n), "wb") as fh:
                        fh.write(data)
                    count += 1
        print(f"extracted {count} files matching {needle!r}")
        return 0

    print(__doc__)
    return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
