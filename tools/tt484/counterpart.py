"""Which OpenW3D class is a TT class actually the same thing as?

`classdiff.py` matches TT classes to OpenW3D classes by name.  That is right most
of the time and badly wrong occasionally: TT renamed several stock Renegade
classes, so a name lookup reports "absent in OpenW3D" and the parity matrix then
plans a from-scratch implementation of a class the engine already has.

This scores every OpenW3D class against a TT class by member-name overlap and
prints the best candidates, so a `NEW` disposition can be confirmed or rejected
on evidence rather than on the name matching or not.

Scoring is containment, not Jaccard: `|shared| / |TT members|`.  A TT header
re-declares only what TT's scripts need to call, so the OpenW3D class is
routinely much larger, and Jaccard would punish exactly the right answer.

Run with no arguments to check every class the matrix currently calls NEW, or
pass class names to check specific ones.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ttparse import find_class, members, scan_classes

# Classes docs/tt484/TTParityMatrix.md currently dispositions as NEW.
NEW = ['AirFactoryGameObj', 'NavalFactoryGameObj', 'ConstructionYardGameObj',
       'SuperweaponGameObj', 'HUDSurfaceClass', 'ScriptedDialogClass',
       'ExtendedNetworkObject']

names = sys.argv[1:] or NEW

print('scanning Code/ ...')
OW = scan_classes('Code')
# One entry per class name; keep the largest declaration when a name repeats.
best = {}
for c in OW:
    prev = best.get(c['name'])
    if prev is None or len(c['methods']) > len(prev['methods']):
        best[c['name']] = c
OW = list(best.values())
print('  %d classes' % len(OW))
print('')

for name in names:
    tp, tl, tb, tbase = find_class('tt_4.8.4', name)
    if not tp:
        print('%s: not found in TT' % name)
        continue
    tm, td = members(tb, name)
    tkeys = set(k.split('/')[0] for k in tm) | set(td)
    if not tkeys:
        print('%s: no members parsed' % name)
        continue

    scored = []
    for c in OW:
        okeys = set(k.split('/')[0] for k in c['methods']) | set(c['fields'])
        shared = tkeys & okeys
        if len(shared) < 3:
            continue
        scored.append((len(shared) / float(len(tkeys)), len(shared), c))
    scored.sort(key=lambda s: (-s[0], -s[1]))

    print('=== %s  (%s:%d, %d members)' % (name, tp, tl, len(tkeys)))
    print('    bases: %s' % (tbase or '-'))
    if not scored:
        print('    no OpenW3D class shares 3+ member names -> genuinely NEW')
        print('')
        continue
    for score, n, c in scored[:5]:
        print('    %5.0f%%  %3d/%-3d shared  %-30s %s:%d' % (
            score * 100, n, len(tkeys), c['name'], c['path'], c['line']))
    top = scored[0]
    missing = sorted(tkeys - (set(k.split('/')[0] for k in top[2]['methods']) | set(top[2]['fields'])))
    if missing:
        print('    absent from %s (%d): %s' % (top[2]['name'], len(missing), ', '.join(missing)))
    print('')
