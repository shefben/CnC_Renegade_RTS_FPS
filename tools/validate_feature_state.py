#!/usr/bin/env python3
"""Check the four persistent project-state files against the rules that govern them.

Roadmap 0A.11 asks for this, and CLAUDE.md's status protocol is what it enforces: the
files are the project's memory, they are edited by hand at the end of every session, and
the ways they go wrong are quiet ones.  A duplicated feature id, an entry left in two
lifecycle files at once, a `Q-###` that nothing answers, a heading renamed so that the
next session's search misses it, a file truncated to nothing by a bad write -- none of
those announce themselves, and all of them cost a later session more than the check does.

What it checks:

  * all four files exist, are readable, and are not accidentally empty;
  * each file still carries the heading it is found by;
  * feature ids are unique inside each lifecycle file;
  * no feature id appears in two lifecycle files at once;
  * `unstarted_features.md` is still delete-only in shape: after its preamble it holds
    nothing but headings, backlog lines and blank lines;
  * every `Q-###` referred to anywhere resolves to an entry in `awaiting_answers.md`,
    and that file's question ids are unique;
  * a phase whose backlog is empty is accounted for somewhere -- reported as a warning,
    because a phase that was never going to have items of its own is normal.

Note that any `Q-###` written in a state file is read as a reference, including one
written as an example, so a state file cannot name a question that does not exist.

What it cannot check is the conservation rule itself.  Whether a line deleted from the
backlog really is the entry that appeared in `completed_features.md` is a question about
meaning, and nothing here can answer it.  This checks the arithmetic around it.

Exit code is 0 when nothing failed, 1 otherwise.  Warnings never fail the run.
"""

import os
import re
import sys


REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

COMPLETED = "completed_features.md"
WIP = "WIP_features.md"
UNSTARTED = "unstarted_features.md"
ANSWERS = "awaiting_answers.md"

#	The heading each file is found by.  Renaming one is how a later session stops finding
#	the file's contents, so it is checked rather than assumed.
REQUIRED_HEADINGS = {
    COMPLETED: "# Completed Features",
    WIP: "# WIP Features",
    UNSTARTED: "# Unstarted Features",
    ANSWERS: "# Awaiting Answers",
}

#	A file this small has been truncated by something, not written by somebody.
MINIMUM_BYTES = 200

#	What counts as a feature id: a phase, optionally with a letter or a word after it, or
#	one of the named non-phase work items.  Anything else in a heading -- "Carried forward
#	out of P05", "Working notes", "N/A: ..." -- is prose or a deliberately repeated marker
#	and is not an id.
ID_PATTERN = re.compile(r"^(P\d+(?:-[A-Za-z0-9]+)?|P-[A-Z]+)$")

PHASE_PATTERN = re.compile(r"^(P\d+|P-[A-Z]+)")
QUESTION_REFERENCE = re.compile(r"Q-(\d{3})")
ANSWER_ENTRY = re.compile(r"\*\*Q-(\d{3})")


class Report:
    def __init__(self):
        self.failures = []
        self.warnings = []

    def fail(self, message):
        self.failures.append(message)

    def warn(self, message):
        self.warnings.append(message)

    def print_and_exit(self):
        for message in self.warnings:
            print("warning: %s" % message)
        for message in self.failures:
            print("FAIL: %s" % message)

        if self.failures:
            print("feature state: %d failure(s), %d warning(s)"
                  % (len(self.failures), len(self.warnings)))
            sys.exit(1)

        print("feature state: pass (%d warning(s))" % len(self.warnings))
        sys.exit(0)


def read(report, name):
    path = os.path.join(REPO_ROOT, name)

    if not os.path.isfile(path):
        report.fail("%s does not exist" % name)
        return None

    with open(path, "r", encoding="utf-8") as handle:
        text = handle.read()

    if len(text.encode("utf-8")) < MINIMUM_BYTES:
        report.fail("%s is %d bytes, which is not a state file but the remains of one"
                    % (name, len(text.encode("utf-8"))))
        return None

    heading = REQUIRED_HEADINGS[name]
    if not text.lstrip().startswith(heading):
        report.fail("%s does not start with %r" % (name, heading))

    return text


#	An entry is a heading, `## P22-A: what got done`, or -- in the later half of the
#	completed file, which changed style -- a bold line, `**P04-AO: what got done.**`.  In
#	both the id is the token a colon directly follows, which is what keeps a section label
#	like `## P02 CLOSED: ...` or a repeated `## N/A: ...` marker from being read as one.
ENTRY_PATTERNS = (
    re.compile(r"^## (\S+):"),
    re.compile(r"^\*\*(\S+):"),
    #	An in-flight entry names what is left rather than what was done, so it is written
    #	`## P22 -- the half that is not finished`.
    re.compile(r"^## (\S+) -- "),
)


def entry_ids(text):
    """Every (id, line) an entry declares, in file order."""
    found = []
    for line in text.splitlines():
        for pattern in ENTRY_PATTERNS:
            match = pattern.match(line)
            if match is None:
                continue
            token = match.group(1)
            if ID_PATTERN.match(token):
                found.append((token, line.strip()))
            break
    return found


def collect_ids(report, name, text):
    """The ids a lifecycle file declares, failing on any it declares twice."""
    found = {}
    for feature, line in entry_ids(text):
        if feature in found:
            report.fail("%s declares %s twice: %r and %r"
                        % (name, feature, found[feature], line))
        else:
            found[feature] = line
    return found


def check_unstarted_shape(report, text):
    """The backlog is delete-only, so its body holds no prose.

    Everything before the first phase heading is the file's own instructions and is left
    alone; after it, a line is a heading, a backlog item or blank.
    """
    lines = text.splitlines()
    started = False

    for number, line in enumerate(lines, start=1):
        if line.startswith("## "):
            started = True
            continue
        if not started:
            continue
        if not line.strip():
            continue
        if line.startswith("#") or line.lstrip().startswith("- ["):
            continue
        report.fail("%s:%d is neither a heading, a backlog line nor blank: %r"
                    % (UNSTARTED, number, line.strip()))


def check_questions(report, texts):
    answers = texts[ANSWERS]
    if answers is None:
        return

    if "## Active" not in answers:
        report.fail("%s has no Active section" % ANSWERS)

    declared = []
    for line in answers.splitlines():
        match = ANSWER_ENTRY.search(line)
        if match:
            declared.append(match.group(1))

    seen = set()
    for question in declared:
        if question in seen:
            report.fail("%s declares Q-%s twice" % (ANSWERS, question))
        seen.add(question)

    if not seen:
        report.warn("%s declares no questions at all" % ANSWERS)

    for name, text in texts.items():
        if text is None:
            continue
        for number, line in enumerate(text.splitlines(), start=1):
            for question in QUESTION_REFERENCE.findall(line):
                if question not in seen:
                    report.fail("%s:%d refers to Q-%s, which %s does not answer"
                                % (name, number, question, ANSWERS))


def check_exactly_one_status(report, per_file_ids):
    """No feature id in two lifecycle files at once.

    A phase heading in the backlog is not an id -- it is a container for backlog lines,
    and it stays after the last of them is done -- so the backlog is compared by the ids
    its *items* name, which is nothing today.  What this catches is an entry copied from
    the in-flight file into the completed one without being removed from the first.
    """
    wip = per_file_ids.get(WIP, {})
    completed = per_file_ids.get(COMPLETED, {})

    for feature in sorted(set(wip) & set(completed)):
        report.fail("%s is an entry in both %s (%r) and %s (%r)"
                    % (feature, WIP, wip[feature], COMPLETED, completed[feature]))


def check_backlog_accounted(report, texts, per_file_ids):
    """A phase whose backlog is empty should be recorded somewhere.

    Warning rather than failure: several phases were closed by a single entry that names
    the items it absorbed, and a few never had items of their own.
    """
    text = texts[UNSTARTED]
    if text is None:
        return

    current = None
    items = {}
    for line in text.splitlines():
        if line.startswith("## "):
            match = PHASE_PATTERN.match(line[3:].strip())
            current = match.group(1) if match else None
            if current is not None:
                items.setdefault(current, 0)
        elif current is not None and line.lstrip().startswith("- ["):
            items[current] += 1

    #	A phase is recorded when it is named anywhere in either file -- several were closed
    #	by one entry that names the phases it absorbed rather than by an entry of their own.
    recorded = set()
    for name in (WIP, COMPLETED):
        if texts[name] is None:
            continue
        for phase in re.findall(r"\b(P\d+|P-[A-Z]+)\b", texts[name]):
            recorded.add(phase)

    for phase, count in sorted(items.items()):
        if count == 0 and phase not in recorded:
            report.warn("%s has no backlog left and no entry in %s or %s"
                        % (phase, WIP, COMPLETED))


def main():
    report = Report()

    texts = {name: read(report, name) for name in REQUIRED_HEADINGS}

    per_file_ids = {}
    for name in (COMPLETED, WIP):
        if texts[name] is not None:
            per_file_ids[name] = collect_ids(report, name, texts[name])

    if texts[UNSTARTED] is not None:
        check_unstarted_shape(report, texts[UNSTARTED])

    check_exactly_one_status(report, per_file_ids)
    check_backlog_accounted(report, texts, per_file_ids)
    check_questions(report, texts)

    report.print_and_exit()


if __name__ == "__main__":
    main()
