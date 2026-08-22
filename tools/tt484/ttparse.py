"""Shared C++ declaration parsing for the TT 4.8.4 analysis generators.

Extracted verbatim from classdiff.py so defcheck.py can reuse the same class
lookup and member extraction.  Both generators must see identical declarations
or their outputs disagree about what a "TT-only method" is.
"""

import io
import os
import re
from collections import OrderedDict

BS = chr(92)

# The 22 public engine classes roadmap Section 7 names by hand.
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


def arity_of(raw):
    raw = raw.strip()
    if raw in ('', 'void'):
        return 0
    depth = 0
    n = 1
    for ch in raw:
        if ch in '(<[':
            depth += 1
        elif ch in ')>]':
            depth -= 1
        elif ch == ',' and depth == 0:
            n += 1
    return n


DECORATOR = re.compile(r'(virtual|static|inline|const|explicit|friend|\w+_API)')


def members(body, cls=None):
    """Parsed members of a class body.

    `cls` is the class's own name.  Pass it: METH allows an empty return type so
    that constructors parse, which also matches a bare call statement inside an
    inline body -- `Set_Object_Dirty_Bit(BIT_RARE, true);` reads as a method
    declaration and inflates the TT-only count.  A declaration with no return
    type can only be a constructor or destructor, so knowing the class name
    rejects the rest.
    """
    meths = OrderedDict()
    data = OrderedDict()
    for ln in body:
        s = ln.strip()
        if not s or s.startswith('//') or s.startswith('*') or s.startswith('#'):
            continue
        m = METH.match(ln)
        if m and m.group(3) not in KEYWORDS:
            name = m.group(3)
            pre = DECORATOR.sub('', ln[:m.start(3)]).strip()
            if cls and not pre and name != cls and not name.startswith('~'):
                continue
            sig = '%s/%d' % (name, arity_of(m.group(4)))
            meths.setdefault(sig, bool(m.group(1)))
            continue
        d = DATA.match(ln)
        if d and d.group(2) not in KEYWORDS and '(' not in ln:
            ty = re.sub(r'\s+', ' ', d.group(1).strip())
            if ty in KEYWORDS or not ty:
                continue
            data.setdefault(d.group(2), ty)
    return meths, data


CLASSHEAD = re.compile(r'^\s*(?:class|struct)\s+(?:\w+_API\s+|__declspec\([^)]*\)\s+)?(\w+)\b(.*)$')


def scan_classes(root):
    """Every class definition under `root` -> list of dicts.

    One pass over the tree, unlike find_class which re-walks per name.  Used to
    find the best OpenW3D counterpart for a TT class by member-name overlap.
    """
    out = []
    for p in walk(root):
        try:
            lines = io.open(p, encoding='utf-8', errors='replace').read().splitlines()
        except Exception:
            continue
        for i, ln in enumerate(lines):
            m = CLASSHEAD.match(ln)
            if not m:
                continue
            rest = m.group(2)
            if rest.strip().startswith(';') or (';' in rest and '{' not in rest):
                continue
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
            bases = re.sub(r'\s+', ' ', txt.split('{')[0].strip().lstrip(':').strip())
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
            meths, data = members(body, m.group(1))
            if not meths and not data:
                continue
            out.append({'name': m.group(1), 'path': p, 'line': i + 1,
                        'bases': bases, 'methods': meths, 'fields': data})
    return out
