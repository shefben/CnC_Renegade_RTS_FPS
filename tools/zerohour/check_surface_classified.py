"""Nothing in the Zero Hour build surface may stay UNREVIEWED.

Roadmap Section 13.2 ends with that sentence, and a sentence is not a
mechanism.  This is the mechanism: `donor_surface.py` generates what the Zero
Hour configuration builds, `ZeroHourSurfaceClassification.tsv` says what each
of those directories is and why, and this fails if the two disagree in either
direction -- a directory with no classification, or a classification for a
directory the donor no longer builds.

It also enforces the vocabulary.  Section 13.2 lists the permitted
classifications; a typo that invents a new one would otherwise look like a
decision.

Run it after the donor pin moves.  Exit code is non-zero when something needs
a human.
"""

import io
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SURFACE = os.path.join(ROOT, 'docs', 'zerohour', 'ZeroHourDonorSurface.tsv')
CLASSIFIED = os.path.join(ROOT, 'docs', 'zerohour',
        'ZeroHourSurfaceClassification.tsv')

#	Roadmap Section 13.2, verbatim.
VOCABULARY = set("""
PORT_NEW_ENGINE_CAPABILITY
PORT_ENGINE_ENHANCEMENT
PORT_RENDER_FEATURE
PORT_RENDER_CORRECTNESS
PORT_AI_IMPROVEMENT
PORT_PATHFINDING_IMPROVEMENT
PORT_RESOURCE_MANAGEMENT
PORT_PERFORMANCE
PORT_MEMORY_SAFETY
PORT_STABILITY
PORT_PORTABILITY_IDEA
PORT_TOOLING
PORT_DATA_VALIDATION
PORT_DEBUG_PROFILING
PORT_REFACTOR_IDEA
OPENW3D_ALREADY_EQUIVALENT
OPENW3D_ALREADY_BETTER_EXTEND_IF_NEEDED
CONTENT_ONLY_REJECT
BALANCE_ONLY_REJECT
CAMPAIGN_CHALLENGE_REJECT
RETAIL_INFRASTRUCTURE_REJECT
SAGE_ARCHITECTURE_REJECT
NOT_RELEVANT_TO_OPENW3D
""".split())

MINIMUM_REASON = 20


def rows(path, columns):
    if not os.path.exists(path):
        raise SystemExit('missing %s; run tools/zerohour/donor_surface.py'
                % os.path.relpath(path, ROOT).replace('\\', '/'))
    out = []
    with io.open(path, encoding='utf-8') as handle:
        header = handle.readline()
        del header
        for line in handle:
            line = line.rstrip('\n')
            if not line.strip():
                continue
            fields = line.split('\t')
            if len(fields) < columns:
                raise SystemExit('short row in %s: %s' % (path, line))
            out.append(fields)
    return out


def main():
    built = [row[0] for row in rows(SURFACE, 4)]
    classified = {}
    for directory, label, reason in ((r[0], r[1], r[2]) for r in rows(CLASSIFIED, 3)):
        if directory in classified:
            raise SystemExit('%s is classified twice' % directory)
        classified[directory] = (label, reason)

    problems = []
    for directory in built:
        if directory not in classified:
            problems.append('%s is built by the Zero Hour configuration and has '
                    'no classification' % directory)
    for directory in classified:
        if directory not in built:
            problems.append('%s is classified but the Zero Hour configuration '
                    'no longer builds it' % directory)
    for directory, (label, reason) in sorted(classified.items()):
        if label not in VOCABULARY:
            problems.append("%s has classification '%s', which is not one of "
                    'the ones Section 13.2 permits' % (directory, label))
        if len(reason.strip()) < MINIMUM_REASON:
            problems.append('%s is classified %s without a written reason'
                    % (directory, label))

    if problems:
        for problem in problems:
            sys.stderr.write('%s\n' % problem)
        return 1

    counts = {}
    for label, _reason in classified.values():
        counts[label] = counts.get(label, 0) + 1
    ported = sum(count for label, count in counts.items() if label.startswith('PORT_'))
    rejected = sum(count for label, count in counts.items()
            if label.endswith('_REJECT') or label == 'NOT_RELEVANT_TO_OPENW3D')
    equivalent = sum(count for label, count in counts.items()
            if label.startswith('OPENW3D_'))
    sys.stdout.write('%d directories classified, none unreviewed '
            '(%d to port, %d already here, %d rejected with a reason)\n'
            % (len(classified), ported, equivalent, rejected))
    return 0


if __name__ == '__main__':
    sys.exit(main())
