"""Validate the four project-state files for bookkeeping contradictions.

Roadmap 0A.11: catch a stale or contradictory handoff before it happens, by
checking four narrow things -- a missing file, a colliding or duplicated ID,
a `Q-###` reference in WIP_features.md that does not resolve to a question
actually recorded in awaiting_answers.md, and a file that went empty because
a generation or edit failed partway through. It does **not** and must not
judge whether an item is really finished; that is what code, tests and gates
are for, never a text scan.

    python tools/validate_feature_state.py

Exit code 0 means no contradiction was found; a non-zero exit and one
`FAIL:` line per problem otherwise.

A note on how IDs are read, because the four files do not share one heading
grammar (they evolved by hand, under the persistent-status-protocol rule
that keeps them terse rather than templated):

- `completed_features.md` and `unstarted_features.md` write `## <id>: ...`;
  `WIP_features.md` writes `## <id> -- ...`. `extract_id` accepts either
  separator and, for unstarted's own `(roadmap Section N)` suffix, also
  drops anything from the first ` (` onward.
- `unstarted_features.md`'s own header says its `## ` headings are the
  roadmap's fixed phase list (P00-P65) and are `delete-only` at the *line*
  level -- a phase's header outlives every `- [ ]` bullet under it being
  removed one at a time as work migrates elsewhere. So a heading there only
  counts as a live, tracked claim on that ID while at least one `- [ ]` line
  still sits under it before the next heading; an empty header is scaffolding,
  not an open item, and must not be flagged as colliding with a WIP or
  completed entry for the same ID.
"""

import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

COMPLETED = os.path.join(ROOT, 'completed_features.md')
WIP = os.path.join(ROOT, 'WIP_features.md')
UNSTARTED = os.path.join(ROOT, 'unstarted_features.md')
ANSWERS = os.path.join(ROOT, 'awaiting_answers.md')

REQUIRED_FILES = [COMPLETED, WIP, UNSTARTED, ANSWERS]

# Comfortably under the smallest of the four today, generous enough that
# ordinary trimming never trips it -- only a file truncated to (near)
# nothing by a failed edit should ever hit this floor.
MIN_BYTES = 200

TITLE_RE = re.compile(r'^#\s+\S')
HEADING_RE = re.compile(r'^##\s+(.+?)\s*$', re.MULTILINE)
BULLET_RE = re.compile(r'^-\s+\[ \]', re.MULTILINE)
QID_RE = re.compile(r'\bQ-(\d{3})\b')


def read(path):
	return io.open(path, encoding='utf-8').read()


def extract_id(heading_text):
	"""The stable token a heading starts with, before its prose."""
	text = heading_text.strip()
	for sep in (' -- ', ':'):
		idx = text.find(sep)
		if idx != -1:
			text = text[:idx]
			break
	paren = text.find(' (')
	if paren != -1:
		text = text[:paren]
	return text.strip()


def section_bodies(text):
	"""Yield (id, body) for every level-2 '## ' section; body is everything
	up to the next '## ' heading or end of file."""
	matches = list(HEADING_RE.finditer(text))
	for i, m in enumerate(matches):
		start = m.end()
		end = matches[i + 1].start() if (i + 1) < len(matches) else len(text)
		yield extract_id(m.group(1)), text[start:end]


def check_files_exist():
	problems = []
	for path in REQUIRED_FILES:
		if not os.path.isfile(path):
			problems.append('missing required file: %s' % path)
	return problems


def check_not_empty_and_titled(path, text):
	problems = []
	size = os.path.getsize(path)
	if size < MIN_BYTES:
		problems.append('%s is %d bytes, suspiciously close to empty (floor %d) -- '
						 'looks like a generation or edit failed partway through'
						 % (path, size, MIN_BYTES))
	if not TITLE_RE.search(text):
		problems.append('%s has no top-level "# " title -- looks truncated' % path)
	return problems


# The persistent-status-protocol's own convention for a genuinely not-applicable item:
# "That still gets a one-line entry in completed_features.md marked N/A: with the reason."
# N/A is a marker prefix, deliberately reused for every cancelled/impossible item -- not a
# feature ID, and repeating it is not a collision.
NOT_AN_ID = frozenset(['N/A'])


def check_unique_ids(label, text):
	problems = []
	seen = {}
	for ident, _ in section_bodies(text):
		if not ident or ident in NOT_AN_ID:
			continue
		seen[ident] = seen.get(ident, 0) + 1
	for ident, count in sorted(seen.items()):
		if count > 1:
			problems.append('%s: heading %r repeated %d times' % (label, ident, count))
	return problems


def tracked_ids(text, only_if_bulleted=False):
	"""The set of ids a file is actually claiming ownership of right now.
	See the module docstring for why unstarted_features.md needs
	only_if_bulleted."""
	ids = set()
	for ident, body in section_bodies(text):
		if not ident or ident in NOT_AN_ID:
			continue
		if only_if_bulleted and not BULLET_RE.search(body):
			continue
		ids.add(ident)
	return ids


def check_exactly_one_status(completed_text, wip_text, unstarted_text):
	completed_ids = tracked_ids(completed_text)
	wip_ids = tracked_ids(wip_text)
	unstarted_ids = tracked_ids(unstarted_text, only_if_bulleted=True)

	problems = []
	pairs = [
		('completed_features.md', completed_ids, 'WIP_features.md', wip_ids),
		('completed_features.md', completed_ids, 'unstarted_features.md (has an unchecked bullet)', unstarted_ids),
		('WIP_features.md', wip_ids, 'unstarted_features.md (has an unchecked bullet)', unstarted_ids),
	]
	for name_a, ids_a, name_b, ids_b in pairs:
		for ident in sorted(ids_a & ids_b):
			problems.append('%r is tracked in both %s and %s' % (ident, name_a, name_b))
	return problems


def check_q_refs(wip_text, answers_text):
	referenced = set(m.group(1) for m in QID_RE.finditer(wip_text))
	recorded = set(m.group(1) for m in QID_RE.finditer(answers_text))
	problems = []
	for num in sorted(referenced - recorded):
		problems.append('WIP_features.md references Q-%s, which does not appear anywhere '
						 'in awaiting_answers.md' % num)
	return problems


def main():
	problems = check_files_exist()
	if problems:
		# A missing file makes every text-level check below meaningless against it.
		for p in problems:
			print('FAIL: %s' % p)
		print('%d problem(s) found.' % len(problems))
		return 1

	texts = {}
	for path in REQUIRED_FILES:
		text = read(path)
		texts[path] = text
		problems += check_not_empty_and_titled(path, text)

	problems += check_unique_ids('completed_features.md', texts[COMPLETED])
	problems += check_unique_ids('WIP_features.md', texts[WIP])
	problems += check_unique_ids('unstarted_features.md', texts[UNSTARTED])
	problems += check_exactly_one_status(texts[COMPLETED], texts[WIP], texts[UNSTARTED])
	problems += check_q_refs(texts[WIP], texts[ANSWERS])

	if problems:
		for p in problems:
			print('FAIL: %s' % p)
		print('%d problem(s) found.' % len(problems))
		return 1

	print('OK: the four project-state files are internally consistent.')
	return 0


if __name__ == '__main__':
	sys.exit(main())
