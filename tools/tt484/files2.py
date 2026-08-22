"""Regenerate docs/tt484/TTFileInventory.tsv.

Covers every C/C++ source file under tt_4.8.4/, assigns each one of the 15 roadmap
Section 7 roles and a planned native destination in the OpenW3D tree.

Run from the repository root:  python tools/tt484/files2.py
"""

import io, os, re
from collections import Counter, defaultdict

BS = chr(92)
TT = 'tt_4.8.4'
CODE = 'Code'
OUT = 'docs/tt484/TTFileInventory.tsv'
HOOKS = 'docs/tt484/TTHookSites.tsv'
EXT = ('.cpp', '.c', '.h', '.hpp', '.inl')

# --------------------------------------------------------------------------
# roles (roadmap Section 7 list, verbatim)
# --------------------------------------------------------------------------
R_SCRIPT = 'custom gameplay script'
R_STOCK = 'stock-script compatibility'
R_API = 'engine wrapper/API'
R_HOOK = 'binary hook/patch'
R_NET = 'network extension'
R_RENDER = 'render/shader integration'
R_UI = 'HUD/UI'
R_BUILD = 'building/factory class'
R_PHYS = 'physics/collision API'
R_SAVE = 'save/load'
R_EDIT = 'editor/LevelEdit'
R_SERVER = 'server-only'
R_CLIENT = 'client-only'
R_UTIL = 'utility'
R_INSTALL = 'installer/deployment'

# --------------------------------------------------------------------------
# per-area dispositions for the trees outside the six engine areas
# --------------------------------------------------------------------------
PLUGINS = ('example-plugin', 'CTF', 'Mute', 'NoPoints', 'FirstBlood',
           'SuddenDeath', 'AntiSpawnKill', 'CharacterRefund',
           'ExtraConsoleCommands', 'PointsDistribution',
           'randomstartingcredits', 'swap', 'crates', 'shared')

AREA_RULE = {
    'shaders':       (R_RENDER,  'EXCLUDED: directive 0.6 (D3D8-to-D3D9 layer, not imported)'),
    'dep':           (R_UTIL,    'EXCLUDED: vendored third-party dependency'),
    'Launcher':      (R_INSTALL, 'Code/Launcher'),
    'wwconfig':      (R_INSTALL, 'Code/Tools/WWConfig'),
    'keycfg':        (R_INSTALL, 'Code/Tools/WWConfig'),
    'ttle':          (R_EDIT,    'Code/Tools/LevelEdit'),
    'tdbedit':       (R_EDIT,    'Code/Strings Editor'),
    'makemix':       (R_EDIT,    'Code/Tools/MakeMix'),
    'bansystem':     (R_SERVER,  'Code/wwnet'),
    'teamspeak':     (R_CLIENT,  'DROP: out of engine scope (external voice client)'),
    'MemoryManager': (R_UTIL,    'Code/wwlib'),
}
for _p in PLUGINS:
    AREA_RULE[_p] = (R_SCRIPT, 'DROP: directive 0.5 (no binary-plugin compatibility layer)')

# VFS/ splits: the DLL is a standalone out-of-process filesystem provider built on
# COM class factories, WinInet namespace registration, RPC and its own crypto and
# libc.  None of that survives a native merge.  The only part with engine meaning is
# the consumer seam, and that already lives in tt/ as VfsFile + PackageManager,
# which subclass FileClass / FileFactoryClass and therefore land in Code/wwlib.
VFS_DROP = 'DROP: standalone out-of-process VFS provider, superseded by Code/wwlib file factory'

# in-scope area fallbacks, used only when nothing better resolves
AREA_DEFAULT = {
    'tt':     'Code/Combat',
    'scripts': 'Code/Scripts',
    'ttinit': 'DROP: directive 0.4 (injector/bootstrap architecture discarded)',
    'VFS':    VFS_DROP,
}

# an OpenW3D destination directory implies a role when content gives no signal
DEST_ROLE = {
    'Code/wwphys':      R_PHYS,
    'Code/ww3d2':       R_RENDER,
    'Code/wwui':        R_UI,
    'Code/wwnet':       R_NET,
    'Code/wwbitpack':   R_NET,
    'Code/WWOnline':    R_NET,
    'Code/wolapi':      R_NET,
    'Code/wwsaveload':  R_SAVE,
    'Code/Tools':       R_EDIT,
    'Code/Launcher':    R_INSTALL,
    'Code/Installer':   R_INSTALL,
    'Code/wwlib':       R_UTIL,
    'Code/wwutil':      R_UTIL,
    'Code/WWMath':      R_UTIL,
    'Code/wwdebug':     R_UTIL,
    'Code/fastalloc_test': R_UTIL,
    'Code/Scripts':     R_SCRIPT,
}

PER_FILE = {
    'tt_4.8.4/tt/VfsFile.h':  'Code/wwlib',
    'tt_4.8.4/tt/VfsFile.cpp': 'Code/wwlib',
    'tt_4.8.4/tt/ResourceMgr/Package.h': 'Code/wwlib',
    'tt_4.8.4/tt/ResourceMgr/Package.cpp': 'Code/wwlib',
    'tt_4.8.4/tt/ResourceMgr/PackageManager.h': 'Code/wwlib',
    'tt_4.8.4/tt/ResourceMgr/PackageManager.cpp': 'Code/wwlib',
    # the handful that reach the end of the resolver with no evidence, placed by
    # inspection rather than left on an area default
    'tt_4.8.4/scripts/ConnectionAcceptanceFilter.h': 'Code/wwnet',
    'tt_4.8.4/scripts/ExtendedNetworkObject.h': 'Code/wwnet',
    'tt_4.8.4/tt/NetworkObjectClassId.h': 'Code/wwnet',
    'tt_4.8.4/scripts/ObserverImpClass.h': 'Code/Combat',
    'tt_4.8.4/scripts/PostLoadableClass.h': 'Code/wwsaveload',
    'tt_4.8.4/scripts/Singleton.h': 'Code/wwlib',
    'tt_4.8.4/scripts/Types.h': 'Code/wwlib',
    'tt_4.8.4/tt/dialogresource.h': 'Code/Commando',
    'tt_4.8.4/tt/HookRegistrant.h':
        'DROP: directive 0.4 (hook installation machinery has no native equivalent)',
    'tt_4.8.4/tt/hooksupport.h':
        'DROP: directive 0.4 (hook installation machinery has no native equivalent)',
    'tt_4.8.4/tt/hooksupport.cpp':
        'DROP: directive 0.4 (hook installation machinery has no native equivalent)',
    'tt_4.8.4/ttinit/d3d8caps.h':
        'EXCLUDED: directive 0.6 (D3D8-to-D3D9 layer, not imported)',
    'tt_4.8.4/ttinit/shaderhooks.cpp':
        'EXCLUDED: directive 0.6 (D3D8-to-D3D9 layer, not imported)',
    'tt_4.8.4/ttinit/shaderhooks.h':
        'EXCLUDED: directive 0.6 (D3D8-to-D3D9 layer, not imported)',
    'tt_4.8.4/ttinit/ttinit.cpp':
        'DROP: directive 0.4 (injector/bootstrap architecture discarded)',
    # tt.cpp is one huge hook installer whose replacement bodies belong to many
    # different owners; the per-site mapping is the authority, not a single row.
    'tt_4.8.4/tt/tt.cpp': 'SPLIT: per hook site, see TTHookSites.tsv',
    'tt_4.8.4/tt/tt.h': 'SPLIT: per hook site, see TTHookSites.tsv',
    # the server-driven UI feature (see TTClassDeltas.md); no stock equivalent, so
    # nothing resolves it by name
    'tt_4.8.4/scripts/HUDSurfaceClass.h': 'Code/wwui',
    'tt_4.8.4/scripts/HUDSurfaceClass.cpp': 'Code/wwui',
    'tt_4.8.4/scripts/ScriptedDialogClass.h': 'Code/wwui',
    'tt_4.8.4/scripts/ScriptedDialogClass.cpp': 'Code/wwui',
    # fan-out guesses the hook data contradicts
    'tt_4.8.4/scripts/PurchaseSettingsDefClass.h': 'Code/Combat',
    'tt_4.8.4/scripts/DefaultConnectionAcceptanceFilter.h': 'Code/wwnet',
    'tt_4.8.4/scripts/DefaultConnectionAcceptanceFilter.cpp': 'Code/wwnet',
}

# --------------------------------------------------------------------------
# content signals, most specific first
# --------------------------------------------------------------------------
HOOKPRIM = re.compile(r'(?<![A-Za-z0-9_])(?:hookAsJump|hookAsCall2?|hookNop|'
                      r'hookVtable|typedHookAsJump|WriteVtable2?|WriteJump'
                      r'(?:ForPlatform)?|WriteCall(?:ForPlatform2?)?|'
                      r'WriteNop(?:ForPlatform)?|WriteMemory)\s*\(')

SIGNALS = [
    # TT declares a gameplay script as `class X : public ScriptImpClass` and
    # registers it with `ScriptRegistrant<X>`.  There is no DECLARE_SCRIPT macro in
    # this tree -- checking for one matches nothing at all.
    (R_SCRIPT, re.compile(r'(?<![A-Za-z0-9_])(?:public\s+ScriptImpClass|'
                          r'ScriptRegistrant\s*<|ScriptCommandsClass)')),
    (R_BUILD,  re.compile(r'(?<![A-Za-z0-9_])(?:BuildingGameObj|'
                          r'[A-Za-z]*FactoryGameObj|ConstructionYardGameObj|'
                          r'SuperweaponGameObj|BaseControllerClass|'
                          r'PowerPlantGameObj|RefineryGameObj)')),
    (R_NET,    re.compile(r'(?<![A-Za-z0-9_])(?:cPacket|cNetEvent|cConnection|'
                          r'cRemoteHost|PacketManager|ConnectionAcceptanceFilter|'
                          r'Send_Object_Update|Import_Occasional)')),
    (R_UI,     re.compile(r'(?<![A-Za-z0-9_])(?:DialogBaseClass|DialogControlClass|'
                          r'MenuDialogClass|Render2DTextClass|HUDSurfaceClass|'
                          r'ScriptedDialogClass|StyleMgrClass|ListCtrlClass|'
                          r'ComboBoxCtrlClass)')),
]
# render/shader, physics/collision and save/load are deliberately NOT content
# signals.  RenderObjClass, SceneClass, PhysClass and ChunkSaveClass appear as
# members or parameters throughout Combat and Commando, so scoring on them labels
# most of the gameplay tree by whatever it happens to hold a pointer to.  Those
# three roles are assigned from the destination directory instead, which is exact.


def read(p):
    try:
        return io.open(p, encoding='utf-8', errors='replace').read()
    except Exception:
        return ''


# --------------------------------------------------------------------------
# index the OpenW3D tree
# --------------------------------------------------------------------------
CLASSD = re.compile(r'^\s*(?:class|struct)\s+(?:__declspec\([^)]*\)\s*)?'
                    r'([A-Za-z_][A-Za-z0-9_]*)\s*(?::|$|\{)')

# Tools, tests and wrappers re-declare engine classes; they must never win a
# lookup against the engine library that actually owns the class.
SECONDARY = ('/Tools/', '/Tests/', '/BandTest/', '/fastalloc_test/',
             '/dxvk_wrapper/', '/Installer/', '/WOLBrowser/', '/SControl/',
             '/Strings Editor/', '/Launcher/', '/wolapi/', '/sndapi/')


# These trees are vendored copies of engine classes kept for a side project; they
# are never a valid destination, so they are dropped from the candidate lists
# rather than merely demoted.
REJECT = ('/sndapi/', '/fastalloc_test/', '/dxvk_wrapper/')


def rank(p):
    return (1 if any(s in p for s in SECONDARY) else 0, p)


def keep(paths):
    return [p for p in paths if not any(s in p for s in REJECT)]


ow_class = {}                       # class name -> owning header path
ow_file = defaultdict(list)         # basename lower -> [path]
_cand = defaultdict(list)
for dirpath, _d, files in os.walk(CODE):
    for fn in files:
        low = fn.lower()
        if not low.endswith(EXT):
            continue
        p = os.path.join(dirpath, fn).replace(BS, '/')
        ow_file[low].append(p)
        if not low.endswith(('.h', '.hpp')):
            continue
        for ln in read(p).splitlines():
            m = CLASSD.match(ln)
            if m:
                _cand[m.group(1)].append(p)
for k, v in _cand.items():
    v = keep(sorted(set(v), key=rank))
    if v:
        ow_class[k] = v[0]
for k in list(ow_file):
    ow_file[k] = keep(sorted(ow_file[k], key=rank))
    if not ow_file[k]:
        del ow_file[k]


def topdir(p):
    """Code/Tools/LevelEdit/x.cpp -> Code/Tools/LevelEdit; Code/Combat/x.cpp -> Code/Combat."""
    parts = p.split('/')
    if len(parts) > 3 and parts[1] == 'Tools':
        return '/'.join(parts[:3])
    return '/'.join(parts[:2])


# --------------------------------------------------------------------------
# hook-site owners, aggregated per TT file
# --------------------------------------------------------------------------
hook_owner = {}
if os.path.exists(HOOKS):
    per = defaultdict(Counter)
    for ln in io.open(HOOKS, encoding='utf-8').read().splitlines()[1:]:
        c = ln.split('|')
        if len(c) < 7:
            continue
        owner = c[6].strip()
        if owner.startswith('Code/'):
            per[c[0].strip()][topdir(owner.split(':')[0])] += 1
    for f, cnt in per.items():
        hook_owner[f] = cnt.most_common(1)[0][0]

# --------------------------------------------------------------------------
# enumerate the TT tree
# --------------------------------------------------------------------------
rows = []
for dirpath, _d, files in os.walk(TT):
    for fn in files:
        if fn.lower().endswith(EXT):
            rows.append(os.path.join(dirpath, fn).replace(BS, '/'))
rows.sort()

INC = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)

resolved = {}     # tt path -> (dest, method, confidence)


def area_of(p):
    return p.split('/')[1]


def pass1(p):
    area = area_of(p)
    if p in PER_FILE:
        return PER_FILE[p], 'per-file', 'high'
    if area == 'shaders':
        return AREA_RULE['shaders'][1], 'area-rule', 'high'
    if '/dep/' in p:
        return AREA_RULE['dep'][1], 'area-rule', 'high'
    if area == 'VFS':
        return VFS_DROP, 'area-rule', 'high'
    if area in AREA_RULE:
        return AREA_RULE[area][1], 'area-rule', 'high'

    low = os.path.basename(p).lower()
    is_hdr = low.endswith(('.h', '.hpp', '.inl'))

    # 0. a DECLARE_SCRIPT anywhere in the file (or its pair) is definitive: this is
    #    TT-authored gameplay script content, and it lands in Code/Scripts.
    src = read(p)
    stem = p[:p.rindex('.')]
    pair = stem + ('.cpp' if is_hdr else '.h')
    if SIGNALS[0][1].search(src) or (os.path.exists(pair)
                                     and SIGNALS[0][1].search(read(pair))):
        return 'Code/Scripts', 'script-decl', 'high'

    # 1. class declared here also declared in OpenW3D
    if is_hdr:
        for ln in src.splitlines():
            m = CLASSD.match(ln)
            if m and m.group(1) in ow_class:
                return topdir(ow_class[m.group(1)]), 'class-decl', 'high'
    else:
        # a .cpp resolves through its own header
        for cand in (low[:-4] + '.h', low[:-2] + '.h'):
            tt_hdr = os.path.dirname(p) + '/' + cand
            for real in (tt_hdr, p[:p.rindex('.')] + '.h'):
                if os.path.exists(real):
                    for ln in read(real).splitlines():
                        m = CLASSD.match(ln)
                        if m and m.group(1) in ow_class:
                            return (topdir(ow_class[m.group(1)]),
                                    'class-decl (paired header)', 'high')
        for m in re.finditer(r'^([A-Za-z_][A-Za-z0-9_]*)::', src, re.M):
            if m.group(1) in ow_class:
                return topdir(ow_class[m.group(1)]), 'member-def', 'high'

    # 2. same filename in OpenW3D
    if low in ow_file:
        return topdir(ow_file[low][0]), 'basename', 'high'

    # 3. this file installs hooks that were already mapped to an owner
    if p in hook_owner:
        return hook_owner[p], 'hook-owner', 'medium'
    return None


for p in rows:
    r = pass1(p)
    if r:
        resolved[p] = r


# Headers that nearly every TT translation unit pulls in (Types.h, Singleton.h,
# engine.h, ...) carry no locality information -- letting them vote drags unrelated
# files toward whatever bucket the common header landed in.
_incfreq = Counter()
for _p in rows:
    for _i in set(INC.findall(read(_p))):
        _incfreq[os.path.basename(_i.replace(BS, '/')).lower()] += 1
UBIQUITOUS = set(k for k, v in _incfreq.items() if v > len(rows) // 12)


def pass2(p):
    """include fan-out: vote over the destinations of the headers this file pulls in."""
    votes = Counter()
    for inc in INC.findall(read(p)):
        base = os.path.basename(inc.replace(BS, '/')).lower()
        if base in UBIQUITOUS:
            continue
        sib = os.path.dirname(p) + '/' + os.path.basename(inc.replace(BS, '/'))
        if sib in resolved and resolved[sib][0].startswith('Code/'):
            votes[resolved[sib][0]] += 1
            continue
        hits = [q for q in resolved
                if os.path.basename(q).lower() == base
                and resolved[q][0].startswith('Code/')]
        if hits:
            votes[resolved[hits[0]][0]] += 1
        elif base in ow_file:
            votes[topdir(ow_file[base][0])] += 1
    if votes:
        top, n = votes.most_common(1)[0]
        tot = sum(votes.values())
        return top, 'include-fanout (%d/%d)' % (n, tot), \
            'medium' if n * 2 > tot else 'low'
    return None


for p in rows:
    if p not in resolved:
        r = pass2(p)
        if r:
            resolved[p] = r

# a header and its .cpp always share a destination; let whichever one resolved
# carry the other
for p in rows:
    if p in resolved:
        continue
    stem = p[:p.rindex('.')]
    for pair in (stem + '.cpp', stem + '.h', stem + '.hpp'):
        if pair != p and pair in resolved:
            d, m, c = resolved[pair]
            resolved[p] = (d, 'paired-file (%s)' % m.split(' ')[0], c)
            break

for p in rows:
    if p not in resolved:
        resolved[p] = (AREA_DEFAULT.get(area_of(p), 'Code/Combat'),
                       'area-default', 'low')


# --------------------------------------------------------------------------
# roles
# --------------------------------------------------------------------------
SYMBOLIC = ('class-decl', 'basename', 'member-def', 'per-file', 'paired-file')


def role_of(p, dest, meth):
    """Destination first, content only where the destination is not decisive.

    A TT file whose class lands in Code/wwphys is a physics/collision API file no
    matter what else it mentions.  Content signals are only consulted for the
    general-purpose destinations (Combat, Commando, Scripts) and for files with no
    OpenW3D destination at all, and there they are scored by match count rather
    than first-hit, because a single incidental mention of ChunkSaveClass or
    cPacket is not evidence of a file's role.
    """
    area = area_of(p)
    src = read(p)
    if area != 'shaders' and '/dep/' in p:
        return R_UTIL
    if area in AREA_RULE:
        return AREA_RULE[area][0]
    if area == 'VFS':
        return R_API if p.lower().endswith(('.h', '.hpp')) else R_UTIL
    if HOOKPRIM.search(src):
        return R_HOOK
    if SIGNALS[0][1].search(src):           # DECLARE_SCRIPT is unambiguous
        return R_SCRIPT
    d = DEST_ROLE.get(dest)
    if d and d != R_SCRIPT:
        return d
    best, n = None, 0
    for role, rx in SIGNALS[1:]:
        c = len(rx.findall(src))
        if c > n:
            best, n = role, c
    if best and n >= 5:
        return best
    if d:
        return d
    # Last resort.  A file that resolved to an OpenW3D owner by symbol identity is
    # wrapping that engine class, which is what `engine wrapper/API` means; a file
    # that only got there by include fan-out or an area default is not, and stays
    # `utility`.
    if dest.startswith('Code/') and meth.startswith(SYMBOLIC):
        return R_API
    return R_UTIL


out = [['path', 'area', 'role', 'lines', 'native_destination',
        'dest_method', 'dest_confidence']]
for p in rows:
    dest, meth, conf = resolved[p]
    n = len(read(p).splitlines())
    out.append([p, area_of(p), role_of(p, dest, meth), str(n), dest, meth, conf])

io.open(OUT, 'w', encoding='utf-8', newline='\n').write(
    '\n'.join('\t'.join(r) for r in out) + '\n')

# --------------------------------------------------------------------------
# summary
# --------------------------------------------------------------------------
print('wrote %s: %d files' % (OUT, len(out) - 1))
for label, idx in (('role', 2), ('destination', 4), ('method', 5), ('confidence', 6)):
    print('\n-- by %s --' % label)
    c = Counter(r[idx] for r in out[1:])
    for k, v in c.most_common(40):
        print('  %-64s %4d' % (k[:64], v))
