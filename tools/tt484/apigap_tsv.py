"""Emit docs/tt484/TTScriptApiGap.tsv from the same analysis tools/tt484/apigap.py runs.

Columns: name, header, calls, disposition.
"""
import glob
import io
import os
import re
from collections import Counter

D = 'tt_4.8.4/scripts/'
TAB = chr(9)
WORD = '[A-Za-z_]' + chr(92) + 'w*'
TYPE = '[' + chr(92) + 'w:<>,&* ]*?'

IN_SCOPE = sorted(set(
    glob.glob(D + 'jfw*.cpp') + glob.glob(D + 'gm*.cpp')
    + [D + 'agtfix.cpp', D + 'obelfix.cpp', D + 'dan.cpp']))
IN_SCOPE = [p for p in IN_SCOPE if os.path.exists(p)]


def read(path):
    return io.open(path, encoding='utf-8', errors='replace').read()


header = read('Code/Combat/scriptcommands.h')
have = set(re.findall(r'\b([A-Z][A-Za-z0-9_]*)\s*\(',
                      header[header.index('namespace ScriptEngine'):]))

sdk_decl = {}
for path in glob.glob(D + 'engine_*.h'):
    text = read(path)
    for m in re.finditer(r'SCRIPTS_API\s+(?:extern\s+)?' + TYPE + r'(' + WORD + r')\s*\(', text):
        sdk_decl.setdefault(m.group(1), os.path.basename(path))
    for m in re.finditer(r'SCRIPTS_API\s+extern\s+\w+\s+(' + WORD + r')\s*;', text):
        sdk_decl[m.group(1)] = os.path.basename(path)

IMPL = re.compile(r'(?m)^(?:[A-Za-z_]' + TYPE + r')?\bSCRIPTS_API\b' + TYPE
                  + r'(' + WORD + r')\s*\(')
sdk_impl = set()
for path in glob.glob(D + 'engine_*.cpp'):
    if path.endswith('engine_tt.cpp'):
        continue
    for m in IMPL.finditer(read(path)):
        sdk_impl.add(m.group(1))

commands = Counter()
frees = Counter()
CALL = re.compile(r'\b(' + WORD + r')\s*\(')
for path in IN_SCOPE:
    text = read(path)
    for m in re.finditer(r'Commands->(' + WORD + r')\s*\(', text):
        commands[m.group(1)] += 1
    for m in CALL.finditer(re.sub(r'Commands->' + WORD + r'\s*\(', ' (', text)):
        if m.group(1) in sdk_decl:
            frees[m.group(1)] += 1

PER_CLIENT = ('_Player', '_Team')
HOOKISH = ('AddObjectCreateHook', 'AddPowerupPurchaseHook', 'AddKeyHook',
           'RemoveKeyHook', 'AddChatHook', 'AddRadioHook', 'AddStockDamageHook',
           'AddTtDamageHook', 'AddLoadLevelHook', 'AddPreLoadLevelHook',
           'AddGameOverHook', 'AddPlayerJoinHook', 'AddPlayerLeaveHook',
           'AddVehiclePurchaseHook', 'AddCharacterPurchaseHook', 'AddThinkHook',
           'AddConsoleOutputHook', 'AddLogFileHook', 'AddRefillHook',
           'AddDialogHook', 'RemovePowerupPurchaseHook', 'RemoveObjectCreateHook')


RENAMED = set(l.split(TAB)[0] for l in
              read('docs/tt484/TTScriptApiRenames.tsv').split(chr(10))[1:] if l.strip())


def disposition(name):
    if name in have or name in RENAMED:
        return 'done'
    if name in HOOKISH:
        return 'n/a-plugin-hook'
    if name.startswith('REF_'):
        return 'n/a-data-binding'
    if name in sdk_impl:
        return 'port-portable-source'
    if name.endswith(PER_CLIENT) or name.startswith('Send_Message'):
        return 'blocked-per-client-delivery'
    return 'port-engine-work'


rows = []
for name, count in commands.items():
    rows.append((name, 'Commands->', count,
                 'done' if name in have else 'port-engine-work'))
for name, count in frees.items():
    rows.append((name, sdk_decl.get(name, '?'), count,
                 'done' if name in have else disposition(name)))

rows.sort(key=lambda r: (r[3], -r[2], r[0]))

out = ['name' + TAB + 'declared_in' + TAB + 'calls' + TAB + 'disposition']
out += [r[0] + TAB + r[1] + TAB + str(r[2]) + TAB + r[3] for r in rows]
io.open('docs/tt484/TTScriptApiGap.tsv', 'w', encoding='utf-8',
        newline='').write('\n'.join(out) + '\n')

tally = Counter(r[3] for r in rows)
calls = Counter()
for r in rows:
    calls[r[3]] += r[2]
print('%d rows' % len(rows))
for k in sorted(tally):
    print('%-30s %4d names %6d calls' % (k, tally[k], calls[k]))
