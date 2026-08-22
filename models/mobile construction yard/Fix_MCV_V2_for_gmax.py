#!/usr/bin/env python3
"""Patch TSR_GDI_MCV_V2 MAXScript for gmax 1.2 top-level declaration rules."""

from __future__ import annotations

import re
import sys
from pathlib import Path


PATTERN = re.compile(
    r"^(?P<indent>[ \t]*)local[ \t]+(?P<name>obj_[0-9]+)[ \t]*=",
    re.MULTILINE,
)


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: python Fix_MCV_V2_for_gmax.py TSR_GDI_MCV_V2_GMAX_3DSMAX.ms")
        return 2

    source = Path(sys.argv[1]).expanduser().resolve()
    if not source.is_file() or source.suffix.lower() != ".ms":
        print(f"Error: not a readable .ms file: {source}")
        return 2

    text = source.read_text(encoding="utf-8-sig")
    fixed, count = PATTERN.subn(
        lambda m: f"{m.group('indent')}global {m.group('name')} =",
        text,
    )

    if count == 0:
        print("Error: no top-level 'local obj_N =' declarations were found.")
        return 1

    remaining = len(PATTERN.findall(fixed))
    destination = source.with_name(f"{source.stem}_GMAX_FIXED.ms")
    destination.write_text(fixed, encoding="utf-8", newline="\n")

    print(f"Patched declarations: {count}")
    print(f"Remaining invalid object locals: {remaining}")
    print(f"Created: {destination}")

    return 0 if remaining == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
