#!/usr/bin/env python
"""Package the authored building cameos for the Commander sidebar.

The art is 1254x1254 24-bit TGA (3-6 MB each), which is far more resolution than a sidebar
card ever draws and far more bytes than the client should keep resident.  The sidebar reads a
descriptor's ``build.icon_texture`` through ``WW3DAssetManager::Get_Texture``, which searches
the mix files and then loose files under the game's ``Data`` directory -- so the packaging step
is: resize to the dimensions the existing cameos use, convert to 32-bit, rename with the
project's ``ow_`` prefix, and drop the result where the client looks.

Target format is taken from the cameos already shipping in ``Data``: 128x128, 32 bits,
uncompressed TGA.

Usage::

    python -I tools/build_commander_icons.py [--source DIR] [--out DIR] [--dry-run]

Defaults: source ``models/building images/purchase_images_2/tga``, out
``C:/Westwood/Renegade_full/Data``.
"""

import argparse
import os
import sys

try:
    from PIL import Image
except ImportError:  # pragma: no cover - the message is the whole point
    sys.exit("Pillow is required: python -m pip install pillow")


ICON_SIZE = (128, 128)
NAME_PREFIX = "ow_bld_"

DEFAULT_SOURCE = os.path.join("models", "building images", "purchase_images_2", "tga")
DEFAULT_OUT = r"C:/Westwood/Renegade_full/Data"


def convert_one(source_path, out_dir, dry_run=False):
    stem = os.path.splitext(os.path.basename(source_path))[0].lower()
    out_name = NAME_PREFIX + stem + ".tga"
    out_path = os.path.join(out_dir, out_name)

    if dry_run:
        return out_name, None

    with Image.open(source_path) as image:
        #   RGBA so the packaged cameo matches the 32-bit cameos already in Data; the authored
        #   art is opaque, so the alpha channel is solid rather than invented.
        image = image.convert("RGBA")
        image = image.resize(ICON_SIZE, Image.LANCZOS)
        image.save(out_path, format="TGA")

    return out_name, os.path.getsize(out_path)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", default=DEFAULT_SOURCE)
    parser.add_argument("--out", default=DEFAULT_OUT)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args(argv)

    if not os.path.isdir(args.source):
        sys.exit("source directory not found: %s" % args.source)
    if not args.dry_run and not os.path.isdir(args.out):
        sys.exit("output directory not found: %s" % args.out)

    sources = sorted(
        os.path.join(args.source, name)
        for name in os.listdir(args.source)
        if name.lower().endswith(".tga")
    )
    if not sources:
        sys.exit("no .tga files in %s" % args.source)

    total = 0
    for source_path in sources:
        out_name, size = convert_one(source_path, args.out, args.dry_run)
        if size is None:
            print("would write %s" % out_name)
        else:
            total += size
            print("%-40s %7d bytes" % (out_name, size))

    print("%d cameos, %d bytes total" % (len(sources), total))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
