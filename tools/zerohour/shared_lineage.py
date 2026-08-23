"""How much of the donor OpenW3D already is.

The port matrix needs an "OpenW3D current equivalent" for every capability,
and guessing at that is how a port matrix becomes fiction.  There is a much
better answer available here than usual: OpenW3D and the Zero Hour donor
descend from the same Westwood W3D libraries, so a large part of the donor's
renderer, math and save/load code exists in this tree under the same file
names.  Where a name matches, the OpenW3D equivalent is not a judgement call.

This pairs the donor's library directories with OpenW3D's and reports, per
pair, how many file names are shared, how many are donor-only, and how many
are OpenW3D-only.  Donor-only names are the interesting column: they are the
capabilities that are genuinely new to us rather than a newer take on
something already here.

Writes `docs/zerohour/ZeroHourSharedLineage.tsv`.
"""

import io
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DONOR = os.path.join(ROOT, 'GeneralsGameCode')
OUT = os.path.join(ROOT, 'docs', 'zerohour', 'ZeroHourSharedLineage.tsv')

CORE_WW = 'Core/Libraries/Source/WWVegas'
ZH_WW = 'GeneralsMD/Code/Libraries/Source/WWVegas'

#	(pair name, [donor directories], OpenW3D directory)
#
#	A donor library can be split across `Core` and `GeneralsMD` -- WW3D2 is,
#	with 68 Zero Hour-specific files that have not been unified into `Core`
#	yet -- so the donor side is a list and the union is what gets compared.
PAIRS = [
    ('WW3D2', ['%s/WW3D2' % CORE_WW, '%s/WW3D2' % ZH_WW], 'Code/ww3d2'),
    ('WWMath', ['%s/WWMath' % CORE_WW], 'Code/WWMath'),
    ('WWLib', ['%s/WWLib' % CORE_WW], 'Code/wwlib'),
    ('WWDebug', ['%s/WWDebug' % CORE_WW], 'Code/wwdebug'),
    ('WWSaveLoad', ['%s/WWSaveLoad' % CORE_WW], 'Code/wwsaveload'),
    ('WWAudio', ['%s/WWAudio' % CORE_WW, '%s/WWAudio' % ZH_WW], 'Code/WWAudio'),
    ('WWDownload', ['%s/WWDownload' % CORE_WW, '%s/WWDownload' % ZH_WW],
            'Code/Libs/WWDownload'),
]

SOURCE_EXT = ('.cpp', '.c', '.h', '.hpp', '.inl')


def names(directory):
    """Source file names directly in `directory`, lower-cased."""
    if not os.path.isdir(directory):
        return set()
    return set(name.lower() for name in os.listdir(directory)
            if name.lower().endswith(SOURCE_EXT))


def main():
    rows = []
    for title, donor_dirs, own_dir in PAIRS:
        theirs = set()
        for relative in donor_dirs:
            theirs |= names(os.path.join(DONOR, relative))
        ours = names(os.path.join(ROOT, own_dir))
        if not theirs:
            raise SystemExit('donor directory for %s is missing: %s'
                    % (title, ', '.join(donor_dirs)))
        shared = theirs & ours
        donor_only = sorted(theirs - ours)
        own_only = sorted(ours - theirs)
        rows.append((title, own_dir, len(theirs), len(ours), len(shared),
                len(donor_only), len(own_only), ';'.join(donor_only)))

    directory = os.path.dirname(OUT)
    if not os.path.isdir(directory):
        os.makedirs(directory)
    with io.open(OUT, 'w', encoding='utf-8', newline='') as handle:
        handle.write('Library\tOpenW3DDirectory\tDonorFiles\tOpenW3DFiles'
                '\tSharedNames\tDonorOnly\tOpenW3DOnly\tDonorOnlyNames\n')
        for row in rows:
            handle.write('\t'.join(str(field) for field in row) + '\n')

    theirs = sum(row[2] for row in rows)
    shared = sum(row[4] for row in rows)
    sys.stdout.write('%s: %d donor library files, %d share a name with this '
            'tree (%d%%)\n' % (os.path.relpath(OUT, ROOT).replace('\\', '/'),
            theirs, shared, (100 * shared) // theirs))
    for row in rows:
        sys.stdout.write('  %-12s donor %4d  ours %4d  shared %4d  donor-only %3d\n'
                % (row[0], row[2], row[3], row[4], row[5]))


if __name__ == '__main__':
    main()
