"""What the in-scope 4.8.4 script library asks the engine for, and what is there.

Two questions, answered by counting rather than by reading:
  - which `Commands->X(...)` calls have no ScriptEngine::X;
  - which free SDK functions the files call, and whether each has portable
    source in engine_*.cpp or is one of engine_tt.h's externs into the closed
    binary.
"""
import glob
import io
import os
import re
from collections import Counter

D = 'tt_4.8.4/scripts/'

IN_SCOPE = sorted(set(
    glob.glob(D + 'jfw*.cpp') + glob.glob(D + 'gm*.cpp')
    + [D + 'agtfix.cpp', D + 'obelfix.cpp', D + 'dan.cpp']))
IN_SCOPE = [p for p in IN_SCOPE if os.path.exists(p)]

WORD = '[A-Za-z_]' + chr(92) + 'w*'
TYPE = '[' + chr(92) + 'w:<>,&* ]*?'


def read(path):
    return io.open(path, encoding='utf-8', errors='replace').read()


# ---- what ScriptEngine offers
header = read('Code/Combat/scriptcommands.h')
body = header[header.index('namespace ScriptEngine'):]
have = set(re.findall(r'\b([A-Z][A-Za-z0-9_]*)\s*\(', body))

# ---- what the donor SDK declares
sdk_decl = {}
for path in glob.glob(D + 'engine_*.h'):
    text = read(path)
    for m in re.finditer(r'SCRIPTS_API\s+(?:extern\s+)?' + TYPE + r'(' + WORD + r')\s*\(', text):
        sdk_decl.setdefault(m.group(1), os.path.basename(path))
    for m in re.finditer(r'SCRIPTS_API\s+extern\s+\w+\s+(' + WORD + r')\s*;', text):
        sdk_decl[m.group(1)] = os.path.basename(path)

# ---- which of them have portable source
IMPL = re.compile(r'(?m)^(?:[A-Za-z_]' + TYPE + r')?\bSCRIPTS_API\b' + TYPE
                  + r'(' + WORD + r')\s*\(')
sdk_impl = set()
for path in glob.glob(D + 'engine_*.cpp'):
    if path.endswith('engine_tt.cpp'):
        continue
    for m in IMPL.finditer(read(path)):
        sdk_impl.add(m.group(1))

# ---- what the in-scope files call
commands = Counter()
frees = Counter()
CALL = re.compile(r'\b(' + WORD + r')\s*\(')

for path in IN_SCOPE:
    text = read(path)
    for m in re.finditer(r'Commands->(' + WORD + r')\s*\(', text):
        commands[m.group(1)] += 1
    stripped = re.sub(r'Commands->' + WORD + r'\s*\(', ' (', text)
    for m in CALL.finditer(stripped):
        if m.group(1) in sdk_decl:
            frees[m.group(1)] += 1

missing_commands = {k: v for k, v in commands.items() if k not in have}
portable = {k: v for k, v in frees.items() if k in sdk_impl}
bound = {k: v for k, v in frees.items() if k not in sdk_impl}

print('in-scope files          : %d' % len(IN_SCOPE))
print('ScriptEngine names      : %d' % len(have))
print('SDK declared            : %d, with portable source %d'
      % (len(sdk_decl), len(sdk_impl)))
print()
print('Commands-> methods used : %d distinct, %d calls'
      % (len(commands), sum(commands.values())))
print('  MISSING from ScriptEngine: %d distinct, %d calls'
      % (len(missing_commands), sum(missing_commands.values())))
print()
print('SDK free functions used : %d distinct, %d calls'
      % (len(frees), sum(frees.values())))
print('  portable source       : %d distinct, %d calls'
      % (len(portable), sum(portable.values())))
print('  extern-bound only     : %d distinct, %d calls'
      % (len(bound), sum(bound.values())))
print()
if missing_commands:
    print('--- missing Commands-> methods ---')
    for name, count in sorted(missing_commands.items(), key=lambda kv: -kv[1]):
        print('%6d  %s' % (count, name))
    print()
print('--- extern-bound free functions, by call count ---')
for name, count in sorted(bound.items(), key=lambda kv: -kv[1]):
    print('%6d  %s   [%s]' % (count, name, sdk_decl.get(name, '?')))
print()
print('--- per-file size ---')
for path in IN_SCOPE:
    print('%6d  %s' % (len(read(path).split('\n')), os.path.basename(path)))
