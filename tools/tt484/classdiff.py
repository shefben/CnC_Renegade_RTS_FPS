import io, os, re
from collections import OrderedDict

BS = chr(92)

CLASSES = [
    'BuildingGameObj', 'BuildingGameObjDef', 'SoldierGameObj', 'VehicleGameObj',
    'AirFactoryGameObj', 'NavalFactoryGameObj', 'SoldierFactoryGameObj',
    'ConstructionYardGameObj', 'SuperweaponGameObj', 'SamSiteGameObj',
    'PhysicsSceneClass', 'GridCullSystemClass', 'AABTreeCullSystemClass',
    'CullSystemClass', 'RenderObjClass', 'SceneClass', 'NetworkObjectClass',
    'ExtendedNetworkObject', 'HUDSurfaceClass', 'ScriptedDialogClass',
    'DefinitionMgrClass', 'DynamicShadowManagerClass',
]


def walk(root, exts=('.h', '.hpp', '.cpp')):
    for dp, _d, fs in os.walk(root):
        if 'shaders' in dp.replace(BS, '/').split('/'):
            continue
        for f in fs:
            if f.lower().endswith(exts):
                yield os.path.join(dp, f).replace(BS, '/')


def find_class(root, name):
    """Return (path, line, body_lines, bases) for the `class name` definition."""
    cands = []
    head = re.compile(r'^\s*(?:class|struct)\s+(?:\w+\s+)?(' + name + r')' + r'\b' + r'(.*)$', re.I)
    for p in walk(root):
        try:
            lines = io.open(p, encoding='utf-8', errors='replace').read().splitlines()
        except Exception:
            continue
        for i, ln in enumerate(lines):
            m = head.match(ln)
            if not m:
                continue
            rest = m.group(2)
            # forward-declaration or variable decl, not a definition
            if rest.strip().startswith(';') or (';' in rest and '{' not in rest):
                continue
            # collect the base-clause, which may wrap over several lines
            j, guard, txt = i, 0, rest
            while '{' not in txt and guard < 8:
                j += 1
                guard += 1
                if j >= len(lines):
                    break
                if ';' in lines[j] and '{' not in lines[j]:
                    txt = None
                    break
                txt += ' ' + lines[j]
            if txt is None or '{' not in txt:
                continue
            bases = txt.split('{')[0].strip().lstrip(':').strip()
            bases = re.sub(r'\s+', ' ', bases)
            depth, body, k, seen = 0, [], i, False
            while k < len(lines):
                depth += lines[k].count('{') - lines[k].count('}')
                if '{' in lines[k]:
                    seen = True
                if k > i:
                    body.append(lines[k])
                if seen and depth <= 0:
                    break
                k += 1
            cands.append((p, i + 1, body, bases))
    if not cands:
        return (None, 0, [], '')
    # prefer the largest real definition, and de-prioritise the tools tree
    cands.sort(key=lambda c: (0 if '/Tools/' in c[0] else 1, len(c[2])), reverse=True)
    return cands[0]


METH = re.compile(
    r'^\s*(virtual\s+)?(static\s+)?(?:inline\s+)?(?:const\s+)?[\w:<>,\s\*&]*?'
    r'(?<![\w:])(~?\w+)\s*\(([^)]*)\)'
    r'(?:\s*(?:const|override|final|noexcept))*'
    r'(?:\s*=\s*(?:0|default|delete))?\s*[;{]')
DATA = re.compile(r'^\s*(?:mutable\s+|static\s+|const\s+)*([\w:<>,\s\*&\[\]]+?)\s+(\w+)\s*(\[[^\]]*\])?\s*;')

KEYWORDS = set(['if', 'for', 'while', 'switch', 'return', 'sizeof', 'else',
                'REF_DECL', 'REF_DECL2', 'REF_DEF', 'REF_DEF2', 'DECLARE_SCRIPT',
                'public', 'private', 'protected', 'typedef', 'enum', 'struct',
                'class', 'union', 'namespace', 'template', 'operator'])


def members(body):
    meths = OrderedDict()
    data = OrderedDict()
    for ln in body:
        s = ln.strip()
        if not s or s.startswith('//') or s.startswith('*') or s.startswith('#'):
            continue
        m = METH.match(ln)
        if m and m.group(3) not in KEYWORDS:
            nm = m.group(3)
            raw = m.group(4).strip()
            if raw in ('', 'void'):
                arity = 0
            else:
                depth = 0
                arity = 1
                for ch in raw:
                    if ch in '(<[':
                        depth += 1
                    elif ch in ')>]':
                        depth -= 1
                    elif ch == ',' and depth == 0:
                        arity += 1
            sig = '%s/%d' % (nm, arity)
            meths.setdefault(sig, bool(m.group(1)))
            continue
        d = DATA.match(ln)
        if d and d.group(2) not in KEYWORDS and '(' not in ln:
            ty = re.sub(r'\s+', ' ', d.group(1).strip())
            if ty in KEYWORDS or not ty:
                continue
            data.setdefault(d.group(2), ty)
    return meths, data


rows = []
detail = []
for c in CLASSES:
    tp, tl, tb, tbase = find_class('tt_4.8.4', c)
    op, ol, ob, obase = find_class('Code', c)
    if not tp and not op:
        rows.append((c, 'absent both', '-', '-', '-', '-'))
        continue
    tm, td = members(tb) if tb else ({}, {})
    om, od = members(ob) if ob else ({}, {})
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
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
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
