"""Where does each TT-only member actually come from?

`classdiff.py` answers "what does TT declare that OpenW3D does not".  That is a
declaration-level answer, and on its own it overstates the merge work by a lot:
a TT header declares plenty of members whose bodies are not in the donor at all.

For every TT-only method on the 22 roadmap Section 7 classes this classifies the
declaration into one of five kinds:

  defined           an ordinary out-of-line body exists in the donor.  Real TT
                    code; this is what P02 merges.
  inline-header     the body is on the declaration line.  Also real TT code, and
                    usually trivial (an accessor over a TT-only field).
  address-stub      declared `RENEGADE_FUNCTION ... AT2(client, server)`, which
                    is a naked thunk that jumps to a hard-coded address in the
                    stock Renegade executable.  There is NO TT implementation:
                    the behaviour is stock, and OpenW3D already has that source.
                    Nothing to merge, and by roadmap directive 0.4 / the P03
                    acceptance wording nothing here may survive as an address.
  pure-virtual      `= 0`, no body by construction.
  declaration-only  declared in the TT header, defined nowhere in the donor.
                    TT ships this so its scripts can link against a binary we do
                    not have the source for.  It cannot be merged.

The last kind is the one that matters, and it splits again.  `classdiff.py`
compares a TT class only against the OpenW3D class of the SAME NAME, so a member
OpenW3D already has on a different owner reads as TT-only.  Every
declaration-only row therefore also records which OpenW3D classes declare that
method name, in `openw3d_owners`:

  owned elsewhere   OpenW3D has the method, on another class (often a subclass,
                    e.g. TT puts Add_Camera_Shake on PhysicsSceneClass while
                    OpenW3D reaches it through COMBAT_SCENE).  Reconcile the
                    owner; no new behaviour to write.
  nowhere           genuinely absent from the engine.  This, and only this, is
                    code that has to be written from the declared semantics.

Counting a declaration-only method as "merge work" plans a port of source that
does not exist; counting an owned-elsewhere one as new work plans a
reimplementation of something already shipping.

Writes docs/tt484/TTMethodSources.tsv.
"""

import io
import os
import re
import sys
from collections import OrderedDict, Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ttparse import (CLASSES, walk, find_class, members, scan_classes,
                     METH, KEYWORDS, arity_of, DECORATOR)

TT = 'tt_4.8.4'
OUT = 'docs/tt484/TTMethodSources.tsv'

K_DEF = 'defined'
K_INLINE = 'inline-header'
K_STUB = 'address-stub'
K_PURE = 'pure-virtual'
K_NONE = 'declaration-only'

ORDER = [K_DEF, K_INLINE, K_STUB, K_PURE, K_NONE]


def decls(body, cls):
    """sig -> (offset, text, inline_body, pure_virtual) for each declaration.

    Must apply the same empty-return-type filter as ttparse.members, or the two
    disagree about which lines are declarations.
    """
    out = OrderedDict()
    for i, ln in enumerate(body):
        s = ln.strip()
        if not s or s.startswith('//') or s.startswith('*') or s.startswith('#'):
            continue
        m = METH.match(ln)
        if not m or m.group(3) in KEYWORDS:
            continue
        name = m.group(3)
        if not DECORATOR.sub('', ln[:m.start(3)]).strip() and name != cls and not name.startswith('~'):
            continue
        sig = '%s/%d' % (name, arity_of(m.group(4)))
        if sig in out:
            continue
        tail = ln[m.end(4):]
        out[sig] = (i, s, m.group(0).rstrip()[-1] == '{', bool(re.search(r'=\s*0\s*;', tail)))
    return out


# ---- index every `Class::method` occurrence in the donor --------------------

print('indexing %s ...' % TT)
FILES = []
for p in walk(TT):
    try:
        FILES.append((p, io.open(p, encoding='utf-8', errors='replace').read().splitlines()))
    except Exception:
        pass
print('  %d files' % len(FILES))

WANT = set(CLASSES)
QUAL = re.compile(r'(?<![\w:])(' + '|'.join(sorted(WANT)) + r')\s*::\s*(~?\w+)\s*\(')
AT = re.compile(r'\bAT([123])\s*\(([^)]*)\)')

# occurrences[(class, method)] = list of (path, lineno, kind, detail)
occurrences = {}

for path, lines in FILES:
    for i, ln in enumerate(lines):
        st = ln.strip()
        if st.startswith('//'):
            continue
        m = QUAL.search(ln)
        if not m:
            continue
        cls, meth = m.group(1), m.group(2)

        # look ahead far enough to see the body or the ATx thunk
        window = '\n'.join(lines[i:i + 12])
        after = window[m.end():]
        # skip past the argument list
        depth, j = 1, 0
        while j < len(after) and depth:
            if after[j] == '(':
                depth += 1
            elif after[j] == ')':
                depth -= 1
            j += 1
        rest = after[j:]

        back = '\n'.join(lines[max(0, i - 4):i])
        at = AT.search(rest[:400])
        if at and ('RENEGADE_FUNCTION' in back or 'RENEGADE_FUNCTION' in ln):
            addrs = re.sub(r'\s+', '', at.group(2))
            kind, detail = K_STUB, 'AT%s(%s)' % (at.group(1), addrs)
        elif re.match(r'\s*(const\s*)?(:[^:][^{]*)?\{', rest) or re.match(r'\s*(const\s*)?\n?\s*\{', rest):
            kind, detail = K_DEF, ''
        elif at:
            addrs = re.sub(r'\s+', '', at.group(2))
            kind, detail = K_STUB, 'AT%s(%s)' % (at.group(1), addrs)
        else:
            continue  # a call site, not a definition

        occurrences.setdefault((cls, meth), []).append((path, i + 1, kind, detail))


# ---- index OpenW3D method names by owning class -----------------------------

print('indexing Code/ ...')
OWNERS = {}
for c in scan_classes('Code'):
    for sig in c['methods']:
        OWNERS.setdefault(sig.split('/')[0], set()).add(c['name'])
print('  %d distinct method names' % len(OWNERS))


# ---- classify every TT-only method -----------------------------------------

rows = []
per_class = OrderedDict()

for c in CLASSES:
    tp, tl, tb, _tbase = find_class(TT, c)
    op, _ol, ob, _obase = find_class('Code', c)
    if not tp:
        continue
    tm, _td = members(tb, c)
    om, _od = members(ob, c) if ob else ({}, {})
    td = decls(tb, c)
    onames = set(k.split('/')[0] for k in om)

    counts = Counter()
    for sig in tm:
        if sig in om:
            continue
        name, ar = sig.split('/')
        off, text, inline, pure = td.get(sig, (0, '', False, False))
        hits = occurrences.get((c, name), [])
        # prefer a real body over a thunk when both spellings exist
        hits = sorted(hits, key=lambda h: ORDER.index(h[2]))

        if inline:
            kind, src, ln, detail = K_INLINE, tp, tl + off, ''
        elif hits:
            kind, src, ln, detail = hits[0][2], hits[0][0], hits[0][1], hits[0][3]
        elif pure:
            kind, src, ln, detail = K_PURE, tp, tl + off, ''
        else:
            kind, src, ln, detail = K_NONE, '', 0, ''

        counts[kind] += 1
        if kind == K_NONE:
            owners = sorted(OWNERS.get(name, set()) - set([c]))
            if name in onames:
                # Same class in OpenW3D, different parameter count: TT added an
                # overload, it did not add a method.
                owned, owners = 'other arity', []
            elif owners:
                owned = 'owned elsewhere'
            else:
                owned = 'nowhere'
            counts['none/' + owned] += 1
        else:
            owners, owned = [], ''
        rows.append((c, name, ar, '1' if tm[sig] else '0',
                     'absent' if not op else 'present',
                     kind, src, str(ln) if ln else '', detail,
                     owned, ','.join(owners[:6]),
                     re.sub(r'\s+', ' ', text)[:160]))
    per_class[c] = counts

rows.sort(key=lambda r: (r[0], ORDER.index(r[5]), r[1]))

hdr = ('class', 'method', 'arity', 'virtual', 'openw3d_class', 'source_kind',
       'source_file', 'source_line', 'address', 'openw3d_has', 'openw3d_owners',
       'declaration')
with io.open(OUT, 'w', encoding='utf-8', newline='\n') as f:
    f.write('\t'.join(hdr) + '\n')
    for r in rows:
        f.write('\t'.join(r) + '\n')

total = Counter()
for c, counts in per_class.items():
    total.update(counts)

print('')
COLS = ('class', 'defined', 'inline', 'stub', 'pure', 'overload', 'elsewhere', 'MISSING')


def line(label, k):
    print('%-26s %8s %7s %6s %5s %9s %10s %8s' % (
        label, k[K_DEF], k[K_INLINE], k[K_STUB], k[K_PURE],
        k['none/other arity'], k['none/owned elsewhere'], k['none/nowhere']))


print('%-26s %8s %7s %6s %5s %9s %10s %8s' % COLS)
for c, counts in per_class.items():
    if not sum(v for kk, v in counts.items() if '/' not in kk):
        continue
    line(c, counts)
line('TOTAL', total)
print('')
print('wrote %s (%d rows)' % (OUT, len(rows)))
