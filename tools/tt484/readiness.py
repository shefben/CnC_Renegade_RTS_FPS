"""Which in-scope files the engine can already answer completely."""
import glob
import io
import os
import sys
import re
from collections import Counter, defaultdict

D = 'tt_4.8.4/scripts/'
TAB = chr(9)
WORD = '[A-Za-z_]' + chr(92) + 'w*'

IN_SCOPE = sorted(set(
    glob.glob(D + 'jfw*.cpp') + glob.glob(D + 'gm*.cpp')
    + [D + 'agtfix.cpp', D + 'obelfix.cpp', D + 'dan.cpp']))


def read(path):
    return io.open(path, encoding='utf-8', errors='replace').read()


disp = {}
for line in read('docs/tt484/TTScriptApiGap.tsv').split('\n')[1:]:
    if line.strip():
        name, header, calls, d = line.split(TAB)
        disp[name] = d


#	Which of a donor file's registered names the engine already answers, so a
#	file that is fully ported stops competing for attention with one that is
#	not.  The catalog checker already knows what a registration looks like --
#	including aliases on merged scripts -- so it is asked rather than guessed at.
REGISTRANT = re.compile(r'ScriptRegistrant<[^>]+>' + chr(92) + r's+' + WORD
                        + chr(92) + r's*' + chr(92) + r'("([^"]+)"')

sys.path.insert(0, os.path.join('tools'))
import check_script_catalog

NATIVE = set(check_script_catalog.scan('Code/Scripts'))


CALL = re.compile(r'\b(' + WORD + r')\s*\(')

rows = []
for path in IN_SCOPE:
    text = read(path)
    stripped = re.sub(r'Commands->' + WORD + r'\s*\(', ' (', text)
    blockers = Counter()
    for m in CALL.finditer(stripped):
        d = disp.get(m.group(1))
        #	An n/a disposition is a decision, not a gap: nothing is owed and
        #	the file is not held up by it.
        if d is not None and d != 'done' and not d.startswith('n/a'):
            blockers[m.group(1) + ':' + d] += 1
    donor = [m.group(1) for m in REGISTRANT.finditer(text)]
    ported = sum(1 for n in donor if n.lower() in NATIVE)
    rows.append((len(text.split(chr(10))), os.path.basename(path),
                 len(donor), ported, sum(blockers.values()),
                 blockers.most_common(4)))

#	A file with nothing left to port is not work; sort it out of the way.
rows.sort(key=lambda r: (r[2] > 0 and r[3] >= r[2], r[4], r[0]))

print('%-18s %6s %7s %6s %9s  %s'
      % ('file', 'lines', 'scripts', 'done', 'blocked', 'top blockers'))
for lines_count, name, scripts, ported, blocked, top in rows:
    print('%-18s %6d %7d %6s %9d  %s'
          % (name, lines_count, scripts,
             ('all' if scripts and ported >= scripts else ported),
             blocked,
             ', '.join('%s x%d' % (k.split(':')[0], v) for k, v in top)))
