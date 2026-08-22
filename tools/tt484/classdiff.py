import io, os, re
import sys
from collections import OrderedDict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ttparse import CLASSES, walk, find_class, members, METH, DATA, KEYWORDS, BS

rows = []
detail = []
for c in CLASSES:
    tp, tl, tb, tbase = find_class('tt_4.8.4', c)
    op, ol, ob, obase = find_class('Code', c)
    if not tp and not op:
        rows.append((c, 'absent both', '-', '-', '-', '-'))
        continue
    tm, td = members(tb, c) if tb else ({}, {})
    om, od = members(ob, c) if ob else ({}, {})
    add_m = [k for k in tm if k not in om]
    add_d = [k for k in td if k not in od]
    rows.append((c,
                 '%s:%d' % (tp, tl) if tp else 'ABSENT in TT',
                 '%s:%d' % (op, ol) if op else 'ABSENT in OpenW3D',
                 len(tm), len(om), '+%d methods / +%d fields' % (len(add_m), len(add_d))))
    detail.append((c, tp, tl, tbase, op, ol, obase, tm, om, td, od, add_m, add_d))

print('%-28s %-46s %-42s %5s %5s  %s' % ('class', 'TT', 'OpenW3D', 'TTm', 'OWm', 'delta'))
for r in rows:
    print('%-28s %-46s %-42s %5s %5s  %s' % r)



# ---- emit deliverable ------------------------------------------------------
import writedoc

NEW = set(['AirFactoryGameObj', 'NavalFactoryGameObj', 'ConstructionYardGameObj',
           'SuperweaponGameObj', 'HUDSurfaceClass', 'ScriptedDialogClass',
           'ExtendedNetworkObject'])

srows = []
sdetail = []
for (c, tp, tl, tbase, op, ol, obase, tm, om, td, od, add_m, add_d) in detail:
    if c in NEW:
        disp = '**NEW**'
    elif c == 'SamSiteGameObj':
        disp = '**NAMING FIX**'
    else:
        disp = 'MERGE'
    srows.append((c,
                  ('`%s:%d`' % (tp, tl)) if tp else 'absent',
                  ('`%s:%d`' % (op, ol)) if op else '**absent**',
                  len(tm), len(om), len(add_m), len(add_d), disp))

    sdetail.append('### `%s` - %s\n\n' % (c, disp.replace('*', '')))
    sdetail.append('| | file | bases | methods | fields |\n')
    sdetail.append('| --- | --- | --- | --- | --- |\n')
    sdetail.append('| TT | %s | `%s` | %d | %d |\n' % (
        ('`%s:%d`' % (tp, tl)) if tp else '**absent**', tbase or '-', len(tm), len(td)))
    sdetail.append('| OpenW3D | %s | `%s` | %d | %d |\n' % (
        ('`%s:%d`' % (op, ol)) if op else '**absent**', obase or '-', len(om), len(od)))
    sdetail.append('\n')
    if tbase and obase and tbase != obase and op:
        sdetail.append('> Base-list difference - reconcile before merging.\n\n')
    if add_m:
        names = []
        for m in add_m:
            n = m.split('/')[0]
            if n not in names:
                names.append(n)
        sdetail.append('TT-only methods (%d): %s\n\n' % (
            len(add_m), ', '.join('`%s`' % n for n in names)))
    if add_d:
        sdetail.append('TT-only fields (%d): %s\n\n' % (
            len(add_d), ', '.join('`%s`' % m for m in add_d)))
    if not add_m and not add_d:
        sdetail.append('No declaration-level delta.\n\n')

writedoc.emit(srows, sdetail)
print('')
print('wrote docs/tt484/TTClassDeltas.md')
