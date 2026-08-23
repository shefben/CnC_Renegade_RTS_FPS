"""Collect every rejected, declined or not-done item into one document.

Rejections are recorded in four different places for four good reasons -- an N/A
entry in `completed_features.md`, a disposition in `docs/tt484/TTHookSites.tsv`,
a classification in `docs/zerohour/ZeroHourSurfaceClassification.tsv`, a
`RejectedSubparts` cell in `docs/zerohour/OpenW3DPortMatrix.tsv` -- and that is
four places to look before anyone can answer "what did we decide not to do".

This gathers them into `docs/RejectedItems.md`, by phase.  It reads the same
files the phases write, so it cannot drift from them; re-run it after any pass
that declines something.

    python tools/rejected_items.py
"""

import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

COMPLETED = os.path.join(ROOT, 'completed_features.md')
HOOKS = os.path.join(ROOT, 'docs', 'tt484', 'TTHookSites.tsv')
SURFACE = os.path.join(ROOT, 'docs', 'zerohour', 'ZeroHourSurfaceClassification.tsv')
MATRIX = os.path.join(ROOT, 'docs', 'zerohour', 'OpenW3DPortMatrix.tsv')
OUTPUT = os.path.join(ROOT, 'docs', 'RejectedItems.md')


#	------------------------------------------------------------------------------
#	The rules that reject things everywhere, rather than in one phase.  These come
#	from the roadmap's numbered directives and are quoted, not summarized, because
#	the wording is what a later session has to obey.
#	------------------------------------------------------------------------------

STANDING = [
    ('0.4  one canonical implementation',
     'Where TT patched, hooked, overrode or replaced stock behaviour, the TT result is '
     'merged into the canonical OpenW3D owner and the superseded path is deleted. '
     'Rejected by this rule: every "keep both and select with a flag" arrangement, '
     'stock-vs-TT wrappers, and "EA behaviour mode" / "TheSuperHackers behaviour mode" '
     'switches on donor code.'),
    ('0.5  no DLL-hook or proxy architecture',
     'The historical TT arrangement -- a patched Game.exe, a proxy DLL and scripts2.dll '
     '-- is not reintroduced, and neither is a binary plugin compatibility layer. '
     'Rejected by this rule: TT\'s injection scaffolding, its plugin hook family, and '
     'the byte patches whose only purpose was to reach an injected DLL.'),
    ('0.6  shaders/ is out of scope',
     'The TT shaders.dll renderer replacement is not ported. Rejected by this rule: the '
     'renderer, texture, shadow, dazzle, vertex-buffer and Bink hook families that exist '
     'only to route drawing through shaders.dll.'),
    ('Zero Hour donor shape',
     'The donor is TheSuperHackers `GeneralsGameCode` configured as `Core + GeneralsMD`. '
     'Rejected: `Generals/` as a donor stage or predecessor implementation source, and '
     'any plan to build or stage through a predecessor-game implementation first. Two '
     'tools error if a `Generals/` path reaches an inventory.'),
    ('Physics and world geometry',
     'OpenW3D/WWPhys is kept; no Jolt or replacement-physics migration. Renegade FPS '
     'world geometry and interiors are preserved. Rejected: replacing authored W3D world '
     'geometry with a heightmap outright (see ZH-04), and any second unrelated world '
     'database (see ZH-19).'),
    ('Stock asset compatibility',
     'Renegade models, maps and textures load unmodified. Engine work may augment, fix '
     'and extend them, never require re-authoring or re-export. Rejected: any change '
     'whose correctness depends on content being rebuilt.'),
]


def read_text(path):
    with io.open(path, encoding='utf-8') as handle:
        return handle.read()


def read_rows(path):
    with io.open(path, encoding='utf-8') as handle:
        lines = [line.rstrip('\n') for line in handle if line.strip()]
    header = lines[0].split('\t')
    return header, [dict(zip(header, line.split('\t'))) for line in lines[1:]]


def one_line(text):
    """Collapse a markdown paragraph into one readable table cell."""
    text = text.replace('|', r'\|')
    text = re.sub(r'\s+', ' ', text).strip()
    return text


#	------------------------------------------------------------------------------
#	completed_features.md: the N/A entries, attributed to the phase they sit in.
#	------------------------------------------------------------------------------

PHASE_HEADING = re.compile(r'^## (P\d\d)[-: ]')
NA_HEADING = re.compile(r'^## N/A: (.+)$')
NA_BOLD = re.compile(r'^\*\*(?:(P\d\d)-[A-Z]+: )?N/A: (.+?)\*\*(.*)$')


def collect_na_entries():
    entries = []
    phase = 'P00'
    lines = read_text(COMPLETED).split('\n')

    index = 0
    while index < len(lines):
        line = lines[index]

        match = PHASE_HEADING.match(line)
        if match:
            phase = match.group(1)
            index += 1
            continue

        heading = NA_HEADING.match(line)
        bold = NA_BOLD.match(line)
        if heading or bold:
            if heading:
                title = heading.group(1)
                body = []
                index += 1
            else:
                if bold.group(1):
                    phase = bold.group(1)
                title = bold.group(2)
                body = [bold.group(3)]
                index += 1

            #	The reason is the paragraph under the heading, past the blank line
            #	markdown puts between them.
            while index < len(lines) and not lines[index].strip():
                index += 1
            while index < len(lines) and lines[index].strip() and not lines[index].startswith('## '):
                body.append(lines[index])
                index += 1

            #	An N/A heading can carry its phase in parentheses instead.
            in_title = re.search(r'\((P\d\d),', title)
            entries.append({
                'phase': in_title.group(1) if in_title else phase,
                'item': one_line(title),
                'reason': one_line(' '.join(body)),
            })
            continue

        index += 1

    return entries


#	------------------------------------------------------------------------------
#	TTHookSites.tsv: the 4.8.4 patch sites that were declined or ruled out of scope.
#	------------------------------------------------------------------------------

def collect_hook_rejections():
    _header, rows = read_rows(HOOKS)

    groups = {}
    for row in rows:
        disposition = row.get('disposition', '').strip()
        if disposition not in ('n/a', 'out-of-scope'):
            continue
        intent = row.get('intent_comment', '').strip() or '(no stated intent)'
        key = (disposition, intent)
        entry = groups.setdefault(key, {'count': 0, 'files': set()})
        entry['count'] += 1
        entry['files'].add(row.get('file', '').strip())

    return groups, len(rows)


#	------------------------------------------------------------------------------
#	The Zero Hour matrices.
#	------------------------------------------------------------------------------

REJECT_LABELS = (
    'CONTENT_ONLY_REJECT', 'BALANCE_ONLY_REJECT', 'CAMPAIGN_CHALLENGE_REJECT',
    'RETAIL_INFRASTRUCTURE_REJECT', 'SAGE_ARCHITECTURE_REJECT',
    'NOT_RELEVANT_TO_OPENW3D',
)


def collect_surface_rejections():
    _header, rows = read_rows(SURFACE)
    return [row for row in rows if row['Classification'] in REJECT_LABELS]


def collect_matrix_rejections():
    _header, rows = read_rows(MATRIX)
    rejected = []
    for row in rows:
        subpart = row['RejectedSubparts'].strip()
        if subpart and subpart.lower() != 'none':
            rejected.append(row)
    return rejected


#	------------------------------------------------------------------------------
#	Rejections a phase made in its own code, which have no generated home yet.
#	Add to this as phases land; each entry names the phase that made the call.
#	------------------------------------------------------------------------------

PHASE_DECISIONS = [
    ('P08', 'The donor\'s `PrototypeClass::DeleteSelf()`',
     'Not adopted. OpenW3D prototypes are deleted with `delete`, which is what '
     '`Free_Assets` already does; adding a second destruction protocol would leave two '
     'ways to destroy a prototype and no rule about which one a caller owes.'),
    ('P08', 'SAGE prototype / `Thing` binding in the asset manager',
     'Rejected subpart of ZH-01. OpenW3D has definitions and its own game loop, so the '
     'donor\'s binding of render prototypes to `ThingTemplate` has no counterpart here '
     'and importing it would mean importing the ownership model directive 0.4 forbids '
     'keeping alongside the existing one.'),
    ('P08', '`Core/Tools/assetcull` as a build step',
     'Not ported as a tool. It culls an asset set offline against a SAGE map format '
     'OpenW3D does not read. The residency service does the same job at run time '
     'against the assets actually loaded, which is also the only form that can work '
     'with unmodified Renegade `.mix` content.'),
    ('P08', 'Freeing an animation that still has a live reference',
     'Deliberately not done. `Free_All_Anims_With_Exclusion_List` releases only '
     'animations the manager alone holds. An animation something is still playing is '
     'kept whatever the keep-list says, because the alternative is a dangling pointer '
     'in a live `Animatable3DObj`.'),
    ('P08', 'Claiming prototypes permanently at startup',
     'Deliberately not done. `Capture_Loaded_Textures` claims the startup textures and '
     'nothing claims a prototype, so every prototype is still freed on a level change. '
     'Prototypes are where the memory is, and keeping a menu-full of 3D content for the '
     'life of the process buys nothing; which prototypes are genuinely shared between levels '
     'is a decision to make against a profile, not in advance.'),
    ('P08', 'A mode-scope claim in the menu game mode',
     'Declined. The menu could claim its own art at mode entry, but the first sweep in '
     '`CombatGameModeClass::Init` already gives menu leftovers an owner with the lifetime '
     'they already had. Two major modes writing one scope would mean two release paths '
     'for a lifetime that is already correct.'),
    ('P08', 'Releasing the mode scope in `Core_Shutdown`',
     'Rejected in favour of `Shutdown`. `Core_Restart` calls `Core_Shutdown` without ever '
     'running `Init` again, so releasing there would empty the mode scope for the rest of '
     'the session with nothing left to re-claim it.'),
    ('P08', 'Naming textures, materials and world buffers in the exclusion list',
     'Rejected. The exclusion list matches w3d file names and has no way to answer for '
     'an asset that has none, so `Build_Retained_List` emits only prototypes, hierarchy '
     'trees and animations. Those kinds are retained the way they always have been, by '
     'reference count, via `Release_Unused_Textures`.'),
]


def main():
    na_entries = collect_na_entries()
    hook_groups, hook_total = collect_hook_rejections()
    surface = collect_surface_rejections()
    matrix = collect_matrix_rejections()

    out = []
    add = out.append

    add('# Rejected, declined and not done')
    add('')
    add('Every item this project decided **not** to do, with the reason, by phase.')
    add('')
    add('Generated by `tools/rejected_items.py` from the files the phases actually write:')
    add('`completed_features.md`, `docs/tt484/TTHookSites.tsv`,')
    add('`docs/zerohour/ZeroHourSurfaceClassification.tsv` and')
    add('`docs/zerohour/OpenW3DPortMatrix.tsv`. Editing this file by hand loses the edit')
    add('on the next regeneration -- change the source, or add to `PHASE_DECISIONS` in')
    add('the tool. Re-run it after any pass that declines something.')
    add('')
    add('A rejection here is a decision, not an oversight. Something genuinely unfinished')
    add('lives in `unstarted_features.md` or `WIP_features.md` instead.')
    add('')

    #	--------------------------------------------------------------- standing rules
    add('## Standing rules that reject things in every phase')
    add('')
    for name, text in STANDING:
        add('**%s.** %s' % (name, text))
        add('')

    #	------------------------------------------------------------------- TT phases
    add('## P00 through P06 -- TT 4.8.4')
    add('')
    add('### Items recorded N/A in `completed_features.md`')
    add('')
    add('%d entries.' % len(na_entries))
    add('')
    add('| Phase | Item | Why not |')
    add('| --- | --- | --- |')
    for entry in na_entries:
        add('| %s | %s | %s |' % (entry['phase'], entry['item'], entry['reason']))
    add('')

    declined = sum(g['count'] for (d, _i), g in hook_groups.items() if d == 'n/a')
    out_of_scope = sum(g['count'] for (d, _i), g in hook_groups.items() if d == 'out-of-scope')

    add('### 4.8.4 patch sites not merged')
    add('')
    add('Of %d hook sites in `docs/tt484/TTHookSites.tsv`, %d were declined with a reason '
        '(`n/a`) and %d are out of scope under a standing rule (`out-of-scope`). Grouped '
        'by what the site was for:' % (hook_total, declined, out_of_scope))
    add('')
    add('| Disposition | What the sites did | Sites | Donor files |')
    add('| --- | --- | --- | --- |')
    for (disposition, intent) in sorted(hook_groups,
            key=lambda key: (key[0], -hook_groups[key]['count'], key[1])):
        group = hook_groups[(disposition, intent)]
        files = sorted(name for name in group['files'] if name)
        shown = ', '.join('`%s`' % name for name in files[:3])
        if len(files) > 3:
            shown += ' and %d more' % (len(files) - 3)
        add('| %s | %s | %d | %s |'
            % (disposition, one_line(intent), group['count'], shown or '--'))
    add('')

    #	------------------------------------------------------------------ Zero Hour
    add('## P07 -- the Zero Hour donor')
    add('')
    add('### Donor build directories rejected outright')
    add('')
    add('%d of the donor\'s 51 built directories are not ported. Each carries a written '
        'reason in `ZeroHourSurfaceClassification.tsv`, which a ctest entry '
        '(`zerohour_surface_classified`) refuses to let go missing.' % len(surface))
    add('')
    add('| Directory | Classification | Why not |')
    add('| --- | --- | --- |')
    for row in sorted(surface, key=lambda row: (row['Classification'], row['Directory'])):
        add('| `%s` | %s | %s |'
            % (row['Directory'], row['Classification'], one_line(row['Reason'])))
    add('')

    add('### Parts of an accepted capability that are not taken')
    add('')
    add('%d of the 40 port-matrix rows accept a capability while rejecting part of it. '
        'The checker (`zerohour_port_matrix`) fails if one of these names a subpart '
        'without saying why.' % len(matrix))
    add('')
    add('| ID | Capability | Rejected subpart, and why |')
    add('| --- | --- | --- |')
    for row in matrix:
        add('| %s | %s | %s |'
            % (row['ID'], one_line(row['Capability']), one_line(row['RejectedSubparts'])))
    add('')

    add('### Categories of community work not accepted')
    add('')
    add('From `ZeroHourCommunityEnhancementAudit.md`. These are not rejections of '
        'individual commits but of whole categories, because the code they change is '
        'either content OpenW3D does not have or architecture it does not keep.')
    add('')
    add('| Category | Why not |')
    add('| --- | --- |')
    add('| Balance and content changes | Zero Hour faction balance. OpenW3D has no Zero '
        'Hour factions. |')
    add('| Generals challenge and campaign shell | Campaign structure OpenW3D does not '
        'implement. |')
    add('| GameSpy, WOL and retail patcher maintenance | Retail infrastructure that is '
        'gone. |')
    add('| Replay system and SAGE networking fixes | They fix code that is rejected '
        'architecture here; OpenW3D keeps its own networking. |')
    add('| WorldBuilder and GUIEdit improvements | Tools for formats OpenW3D does not '
        'read. |')
    add('')

    #	--------------------------------------------------------- per-phase decisions
    phases = sorted(set(phase for phase, _item, _reason in PHASE_DECISIONS))
    add('## Decisions made inside an implementation phase')
    add('')
    add('Rejections a phase made while writing code, which have no generated home. Kept '
        'in `PHASE_DECISIONS` in the tool.')
    add('')
    for phase in phases:
        add('### %s' % phase)
        add('')
        add('| Item | Why not |')
        add('| --- | --- |')
        for entry_phase, item, reason in PHASE_DECISIONS:
            if entry_phase == phase:
                add('| %s | %s |' % (one_line(item), one_line(reason)))
        add('')

    with io.open(OUTPUT, 'w', encoding='utf-8', newline='\n') as handle:
        handle.write('\n'.join(out))

    sys.stdout.write('%s: %d N/A entries, %d declined hook groups, %d rejected '
            'directories, %d rejected subparts, %d in-phase decisions\n'
            % (os.path.relpath(OUTPUT, ROOT), len(na_entries), len(hook_groups),
               len(surface), len(matrix), len(PHASE_DECISIONS)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
