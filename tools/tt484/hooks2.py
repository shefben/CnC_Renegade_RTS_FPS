import io, os, re
from collections import Counter

ROOT = 'tt_4.8.4'
AREAS = ['tt', 'scripts', 'ttinit', 'VFS', 'MemoryManager', 'Launcher']

PRIMS = [
    'hookAsJump', 'hookAsCall2', 'hookAsCall', 'hookNop', 'hookVtable',
    'typedHookAsJump', 'WriteVtable2', 'WriteVtableForPlatform',
    'WriteJumpForPlatform', 'WriteCallForPlatform2', 'WriteCallForPlatform',
    'WriteNopForPlatform', 'WriteVtable', 'WriteJump', 'WriteCall',
    'WriteNop', 'WriteMemory', 'VirtualProtect',
    'JumpHookRegistrant', 'CallHookRegistrant',
]
PRIM_RE = re.compile(r'(?<![A-Za-z0-9_])(' + '|'.join(PRIMS) + r')\s*[\(\s]')
ADDR_RE = re.compile(r'0[xX][0-9a-fA-F]{5,8}')
ADDR_FULL = re.compile(r'^0[xX][0-9a-fA-F]{1,8}$')

DEF_FILES = set(['hooksupport.cpp', 'hooksupport.h',
                 'HookRegistrant.h', 'HookRegistrant.cpp'])

BS = chr(92)

rows = []
for area in AREAS:
    base = os.path.join(ROOT, area)
    if not os.path.isdir(base):
        continue
    for dirpath, _dirs, files in os.walk(base):
        for fn in files:
            if not fn.lower().endswith(('.cpp', '.h', '.c', '.hpp', '.inl')):
                continue
            if fn in DEF_FILES:
                continue
            p = os.path.join(dirpath, fn)
            try:
                txt = io.open(p, encoding='utf-8', errors='replace').read()
            except Exception:
                continue
            for i, ln in enumerate(txt.splitlines(), 1):
                s = ln.strip()
                if s.startswith('//') or s.startswith('*'):
                    continue
                if s.startswith('#define') or s.startswith('__asm'):
                    continue
                m = PRIM_RE.search(ln)
                if not m:
                    continue
                prim = m.group(1)
                if re.match(r'^\s*(static\s+)?(void|int|bool)\s+' + prim, ln):
                    continue
                addrs = ','.join(ADDR_RE.findall(ln))
                cm = re.search(r'//\s*(.*)$', ln)
                comment = cm.group(1).strip() if cm else ''
                repl = ''
                am = re.search(re.escape(prim) + r'\s*\((.*)\)\s*;?\s*(//.*)?$', ln)
                if am:
                    args = am.group(1)
                    depth = 0
                    parts = []
                    curp = ''
                    for ch in args:
                        if ch in '(<[':
                            depth += 1
                        elif ch in ')>]':
                            depth -= 1
                        if ch == ',' and depth == 0:
                            parts.append(curp.strip())
                            curp = ''
                        else:
                            curp += ch
                    parts.append(curp.strip())
                    idx = None
                    if prim in ('hookAsJump', 'hookAsCall', 'hookVtable',
                                'typedHookAsJump', 'WriteJumpForPlatform',
                                'WriteCallForPlatform', 'WriteVtableForPlatform',
                                'hookAsCall2', 'WriteCallForPlatform2'):
                        idx = 2
                    elif prim in ('WriteJump', 'WriteCall', 'WriteVtable', 'WriteVtable2'):
                        idx = 1
                    if idx is not None and len(parts) > idx:
                        repl = parts[idx]
                    if repl and (ADDR_FULL.match(repl) or repl in ('0', 'NULL')):
                        repl = ''
                    tail = re.search(r'[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_~][A-Za-z0-9_]*)*$', repl.strip())
                    repl = tail.group(0) if tail else ''
                    repl = repl.replace('&', '')
                rows.append((p.replace(BS, '/'), i, prim, addrs, repl, comment))

rows.sort(key=lambda r: (r[0], r[1]))
with io.open('docs/tt484/TTHookSites.tsv', 'w', encoding='utf-8', newline='\n') as f:
    f.write('file\tline\tprimitive\taddresses\ttt_replacement\tintent_comment\n')
    for r in rows:
        f.write('\t'.join(str(x) for x in r) + '\n')

print('total sites: %d' % len(rows))
print('')
print('by file:')
for k, v in Counter(r[0] for r in rows).most_common():
    print('  %-52s %d' % (k, v))
print('')
print('by primitive:')
for k, v in Counter(r[2] for r in rows).most_common():
    print('  %-24s %d' % (k, v))
print('')
print('with intent comment: %d' % sum(1 for r in rows if r[5]))
print('with named TT replacement: %d' % sum(1 for r in rows if r[4]))
print('with neither: %d' % sum(1 for r in rows if not r[4] and not r[5]))
