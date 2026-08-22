"""Which in-scope files the engine can already answer completely."""
import glob
import io
import os
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

CALL = re.compile(r'\b(' + WORD + r')\s*\(')

rows = []
for path in IN_SCOPE:
    text = read(path)
    stripped = re.sub(r'Commands->' + WORD + r'\s*\(', ' (', text)
    blockers = Counter()
    for m in CALL.finditer(stripped):
        d = disp.get(m.group(1))
        if d is not None and d not in ('done',):
            blockers[m.group(1) + ':' + d] += 1
    rows.append((len(text.split('\n')), os.path.basename(path),
                 text.count('ScriptRegistrant<'), sum(blockers.values()),
                 blockers.most_common(4)))

rows.sort(key=lambda r: (r[3], r[0]))
print('%-18s %6s %7s %9s  %s' % ('file', 'lines', 'scripts', 'blocked', 'top blockers'))
for lines, name, scripts, blocked, top in rows:
    print('%-18s %6d %7d %9d  %s'
          % (name, lines, scripts, blocked,
             ', '.join('%s x%d' % (k.split(':')[0], v) for k, v in top)))
