"""What the Zero Hour donor actually builds, read from the donor itself.

Roadmap Section 13.2 asks for an inventory of the build surface that
participates in the Zero Hour target, and says to follow the actual CMake
target graph rather than assuming an EA-era directory layout.  So this walks
the graph: it starts at the donor's root `CMakeLists.txt`, follows
`add_subdirectory` only where the Zero Hour configuration would follow it, and
records every `add_library`/`add_executable` it reaches.

The Zero Hour configuration is `RTS_BUILD_ZEROHOUR` on and `RTS_BUILD_GENERALS`
off, which the donor's own root list file turns into `Core` plus `GeneralsMD`
and no `Generals/`.  That is not an assumption of ours; it is the donor's
condition, and this tool honours it, so the `Generals/` tree never appears in
the output.

Writes `docs/zerohour/ZeroHourDonorSurface.tsv`.
"""

import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DONOR = os.path.join(ROOT, 'GeneralsGameCode')
OUT = os.path.join(ROOT, 'docs', 'zerohour', 'ZeroHourDonorSurface.tsv')

#	`add_subdirectory(x)` / `add_subdirectory(x EXCLUDE_FROM_ALL)`
SUBDIR = re.compile(r'^\s*add_subdirectory\s*\(\s*([^\s)]+)', re.MULTILINE | re.IGNORECASE)
#	`add_library(name ...)` and `add_executable(name ...)`
TARGET = re.compile(r'^\s*add_(library|executable)\s*\(\s*([A-Za-z0-9_]+)([^)]*)',
        re.MULTILINE | re.IGNORECASE)
#	`if(RTS_BUILD_GENERALS)` and friends, to know which arm we are in
IFLINE = re.compile(r'^\s*(if|elseif|else|endif)\s*\(([^)]*)\)', re.MULTILINE | re.IGNORECASE)

SOURCE_EXT = ('.cpp', '.c', '.cc', '.h', '.hpp', '.inl')

#	Conditions that are false in the Zero Hour configuration.  Everything else
#	is followed: a subdirectory guarded by a condition we cannot evaluate is
#	better inventoried and classified than silently dropped.
FALSE_IN_ZH = (
    'RTS_BUILD_GENERALS',
    'RTS_BUILD_GENERALS_TOOLS',
    'RTS_BUILD_GENERALS_DOCS',
    'RTS_BUILD_GENERALS_WORLDBUILDER',
    'RTS_BUILD_GENERALS_GUIEDIT',
    'IS_VS6_BUILD',
)


def guarded_out(text, position):
    """True when `position` sits inside an if-arm the Zero Hour build skips.

    A crude reading of the surrounding `if`/`endif` nesting is enough here:
    the donor guards whole subdirectories with a single flat condition, and
    the only conditions that matter are the Generals-only ones.
    """
    depth = []
    for match in IFLINE.finditer(text):
        if match.start() > position:
            break
        keyword = match.group(1).lower()
        condition = match.group(2).strip()
        if keyword == 'if':
            depth.append(any(flag in condition for flag in FALSE_IN_ZH))
        elif keyword == 'elseif':
            if depth:
                depth[-1] = False
        elif keyword == 'else':
            if depth:
                depth[-1] = not depth[-1]
        elif keyword == 'endif':
            if depth:
                depth.pop()
    return any(depth)


def read(path):
    with io.open(path, encoding='utf-8', errors='replace') as handle:
        return handle.read()


def count_sources(directory):
    """Source files directly under `directory`, and under it in total."""
    here = 0
    below = 0
    for name in sorted(os.listdir(directory)):
        full = os.path.join(directory, name)
        if os.path.isdir(full):
            if os.path.exists(os.path.join(full, 'CMakeLists.txt')):
                #	counted by its own row instead
                continue
            for _walk_root, _dirs, files in os.walk(full):
                below += sum(1 for f in files if f.lower().endswith(SOURCE_EXT))
        elif name.lower().endswith(SOURCE_EXT):
            here += 1
    return here, here + below


def walk(directory, rows, seen):
    """Record `directory`'s targets, then follow the subdirectories it adds."""
    listfile = os.path.join(directory, 'CMakeLists.txt')
    if not os.path.exists(listfile) or listfile in seen:
        return
    seen.add(listfile)
    text = read(listfile)
    relative = os.path.relpath(directory, DONOR).replace('\\', '/')

    targets = []
    for match in TARGET.finditer(text):
        if guarded_out(text, match.start()):
            continue
        kind = match.group(1).lower()
        name = match.group(2)
        tail = match.group(3).upper()
        if 'INTERFACE' in tail.split():
            kind = 'interface'
        elif kind == 'library' and 'STATIC' in tail.split():
            kind = 'static'
        targets.append('%s:%s' % (kind, name))

    direct, total = count_sources(directory)
    if targets or direct:
        rows.append((relative, ';'.join(targets), direct, total))

    for match in SUBDIR.finditer(text):
        if guarded_out(text, match.start()):
            continue
        child = match.group(1).strip('"')
        if '${' in child:
            #	a generated path; nothing to follow statically
            continue
        walk(os.path.normpath(os.path.join(directory, child)), rows, seen)


def main():
    if not os.path.isdir(DONOR):
        raise SystemExit('donor submodule is not checked out at %s' % DONOR)

    rows = []
    walk(DONOR, rows, set())

    for relative, _targets, _direct, _total in rows:
        if relative.split('/')[0] == 'Generals':
            raise SystemExit('the Generals/ tree reached the inventory: %s' % relative)

    directory = os.path.dirname(OUT)
    if not os.path.isdir(directory):
        os.makedirs(directory)
    with io.open(OUT, 'w', encoding='utf-8', newline='') as handle:
        handle.write('Directory\tTargets\tSourcesHere\tSourcesBelow\n')
        for relative, targets, direct, total in rows:
            handle.write('%s\t%s\t%d\t%d\n' % (relative, targets, direct, total))

    files = sum(row[2] for row in rows)
    targets = sum(len(row[1].split(';')) for row in rows if row[1])
    sys.stdout.write('%s: %d directories, %d targets, %d source files\n'
            % (os.path.relpath(OUT, ROOT).replace('\\', '/'), len(rows), targets, files))


if __name__ == '__main__':
    main()
