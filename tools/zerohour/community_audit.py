"""What TheSuperHackers changed after EA's release, per subsystem.

Roadmap Section 13.4 says not to stop at the EA 2003 feature set: the pinned
donor carries years of community maintenance, and the useful parts of it are
part of what gets ported.  Finding those parts by reading 1700 commits by hand
is not a plan, so this attributes each commit to the Zero Hour systems whose
files it touched, using the mapping `capability_map.py` already resolved.

Two things make the result trustworthy rather than a word count:

  * a commit is attributed by the files it changed, not by its subject line,
    so a fix whose message never says "pathfinding" still lands on ZH-24;
  * commits that only touch `Generals/` are dropped, because that tree is not
    a donor and its history is not ours to mine.

The donor's history begins at EA's source release, so every commit in it is
community work by definition.  Writes
`docs/zerohour/ZeroHourCommunityCommits.tsv`.
"""

import io
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DONOR = os.path.join(ROOT, 'GeneralsGameCode')
SOURCES = os.path.join(ROOT, 'docs', 'zerohour', 'ZeroHourCapabilitySources.tsv')
OUT = os.path.join(ROOT, 'docs', 'zerohour', 'ZeroHourCommunityCommits.tsv')

#	`bugfix(pathfinder): ...` -- the donor's convention.
SUBJECT = re.compile(r'^([a-z]+)\s*(?:\(([^)]*)\))?\s*:\s*(.*)$')

#	The kinds Section 13.4 asks to be found.  Anything else is recorded with
#	its own type rather than being dropped: what "chore" turns out to mean is
#	a judgement for the audit, not for this tool.
INTERESTING = ('bugfix', 'fix', 'perf', 'unify', 'refactor', 'feat', 'tweak')

#	Enough history for the whole donor; `git log` here truncates without it.
DEPTH = '100000'


def git(*arguments):
    result = subprocess.run(['git', '--no-pager'] + list(arguments),
            cwd=DONOR, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        raise SystemExit('git %s failed: %s'
                % (' '.join(arguments), result.stderr.decode('utf-8', 'replace')))
    return result.stdout.decode('utf-8', 'replace')


def load_systems():
    """system id -> (title, set of donor-relative paths)."""
    if not os.path.exists(SOURCES):
        raise SystemExit('run capability_map.py first: %s is missing' % SOURCES)
    systems = {}
    with io.open(SOURCES, encoding='utf-8') as handle:
        header = handle.readline()
        del header
        for line in handle:
            fields = line.rstrip('\n').split('\t')
            if len(fields) < 8:
                continue
            paths = set()
            for group in (fields[6], fields[7]):
                paths.update(p for p in group.split(';') if p)
            systems[fields[0]] = (fields[2], paths)
    return systems


def commits():
    """Every commit, as (sha, author date, subject, [paths])."""
    #	A record separator no path or subject can contain.
    text = git('log', '-n', DEPTH, '--no-merges', '--name-only',
            '--format=\x01%H\t%aI\t%s')
    for block in text.split('\x01'):
        block = block.strip('\n')
        if not block:
            continue
        lines = block.split('\n')
        head = lines[0].split('\t', 2)
        if len(head) < 3:
            continue
        paths = [line for line in lines[1:] if line.strip()]
        yield head[0], head[1], head[2], paths


def main():
    systems = load_systems()
    #	path -> [system ids], so attribution is a lookup rather than a scan
    owner = {}
    for ident, (_title, paths) in systems.items():
        for path in paths:
            owner.setdefault(path, []).append(ident)

    rows = []
    skipped_generals = 0
    total = 0
    for sha, date, subject, paths in commits():
        total += 1
        if paths and all(p.startswith('Generals/') for p in paths):
            skipped_generals += 1
            continue

        match = SUBJECT.match(subject)
        if match:
            kind = match.group(1)
            scope = match.group(2) or ''
        else:
            kind = 'other'
            scope = ''

        touched = set()
        for path in paths:
            for ident in owner.get(path, ()):
                touched.add(ident)

        trees = set()
        for path in paths:
            top = path.split('/')[0]
            if top in ('Core', 'GeneralsMD'):
                trees.add(top)

        rows.append((sha[:9], date[:10], kind, scope, subject.replace('\t', ' '),
                ';'.join(sorted(touched)), ';'.join(sorted(trees)), len(paths)))

    directory = os.path.dirname(OUT)
    if not os.path.isdir(directory):
        os.makedirs(directory)
    with io.open(OUT, 'w', encoding='utf-8', newline='') as handle:
        handle.write('SHA\tAuthorDate\tType\tScope\tSubject\tSystems\tTrees\tFiles\n')
        for row in rows:
            handle.write('\t'.join(str(field) for field in row) + '\n')

    attributed = sum(1 for row in rows if row[5])
    interesting = sum(1 for row in rows if row[2] in INTERESTING)
    per_system = {}
    for row in rows:
        for ident in row[5].split(';'):
            if ident:
                per_system[ident] = per_system.get(ident, 0) + 1

    sys.stdout.write('%s: %d commits (%d total, %d Generals-only dropped), '
            '%d touch a mapped system, %d are fixes/perf/refactors\n'
            % (os.path.relpath(OUT, ROOT).replace('\\', '/'), len(rows), total,
               skipped_generals, attributed, interesting))
    for ident in sorted(systems):
        sys.stdout.write('  %s %-45s %4d\n'
                % (ident, systems[ident][0], per_system.get(ident, 0)))


if __name__ == '__main__':
    main()
