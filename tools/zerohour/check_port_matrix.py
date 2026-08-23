"""The port matrix has to stay a matrix.

Roadmap Section 13.6 lists eighteen things every accepted capability records
and a fixed vocabulary of migration classes.  A row that quietly loses a column,
a capability that never gets a row, or a migration class invented by a typo all
look like decisions when they are accidents.  This is what tells them apart.

Checked here:

  * every ZH id named in ZeroHourCapabilityMatrix.md has exactly one row;
  * every row has all eighteen fields, none of the required ones blank;
  * the migration class is one Section 13.6 permits;
  * the classification is one Section 13.2 permits;
  * a row claiming a rejected subpart gives a reason, not just a name;
  * every row names at least one test.

`EAValidation` may be blank -- Section 13.5 makes consulting EA's tree
optional, and a blank there means it was not needed, which is information.
"""

import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DOCS = os.path.join(ROOT, 'docs', 'zerohour')
MATRIX = os.path.join(DOCS, 'OpenW3DPortMatrix.tsv')
CAPABILITIES = os.path.join(DOCS, 'ZeroHourCapabilityMatrix.md')

COLUMNS = [
    'ID', 'Capability', 'DonorSources', 'Tree', 'Classification', 'EAValidation',
    'OpenW3DEquivalent', 'TargetModule', 'TTDependency', 'RendererDependency',
    'PhysicsDependency', 'AIDependency', 'NetworkDependency',
    'SaveLoadDependency', 'ClientServer', 'MigrationClass', 'RejectedSubparts',
    'Tests',
]

#	May be empty, and empty means something.
OPTIONAL = ('EAValidation',)

MIGRATION = set("""
DIRECT_ADAPT
ADAPTED_BACKPORT
ALGORITHM_ONLY
ARCHITECTURE_ONLY
BUGFIX_ONLY
PERFORMANCE_IDEA_ONLY
ALREADY_BETTER_EXTEND
REJECTED_SUBPART
""".split())

CLASSIFICATION = set("""
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
""".split())

TREES = ('Core', 'GeneralsMD', 'Core+GeneralsMD')

ID = re.compile(r'\bZH-(\d\d)\b')
MINIMUM_REASON = 12


def main():
    if not os.path.exists(MATRIX):
        raise SystemExit('missing %s' % MATRIX)

    with io.open(MATRIX, encoding='utf-8') as handle:
        lines = [line.rstrip('\n') for line in handle if line.strip()]

    header = lines[0].split('\t')
    if header != COLUMNS:
        raise SystemExit('the matrix header is not the eighteen fields Section '
                '13.6 asks for; got %d: %s' % (len(header), ', '.join(header)))

    problems = []
    rows = {}
    for line in lines[1:]:
        fields = line.split('\t')
        if len(fields) != len(COLUMNS):
            problems.append('%s has %d fields, not %d'
                    % (fields[0] if fields else '?', len(fields), len(COLUMNS)))
            continue
        row = dict(zip(COLUMNS, fields))
        if row['ID'] in rows:
            problems.append('%s has two rows' % row['ID'])
        rows[row['ID']] = row

    for ident in sorted(rows):
        row = rows[ident]
        for column in COLUMNS:
            if column in OPTIONAL:
                continue
            if not row[column].strip():
                problems.append('%s leaves %s blank' % (ident, column))
        if row['Tree'] not in TREES:
            problems.append("%s has tree '%s'" % (ident, row['Tree']))
        for label in row['Classification'].split(';'):
            if label.strip() and label.strip() not in CLASSIFICATION:
                problems.append("%s has classification '%s'" % (ident, label.strip()))
        if row['MigrationClass'] not in MIGRATION:
            problems.append("%s has migration class '%s', which Section 13.6 "
                    'does not list' % (ident, row['MigrationClass']))
        rejected = row['RejectedSubparts'].strip()
        if rejected.lower() != 'none' and len(rejected) < MINIMUM_REASON:
            problems.append('%s names a rejected subpart without saying why'
                    % ident)

    #	Every capability the matrix document names must have a row.
    with io.open(CAPABILITIES, encoding='utf-8') as handle:
        named = set('ZH-%s' % match for match in ID.findall(handle.read()))
    for ident in sorted(named - set(rows)):
        problems.append('%s is a capability in ZeroHourCapabilityMatrix.md with '
                'no port-matrix row' % ident)
    for ident in sorted(set(rows) - named):
        problems.append('%s has a port-matrix row but is not a capability in '
                'ZeroHourCapabilityMatrix.md' % ident)

    if problems:
        for problem in problems:
            sys.stderr.write('%s\n' % problem)
        return 1

    consulted = sum(1 for row in rows.values() if row['EAValidation'].strip())
    classes = {}
    for row in rows.values():
        classes[row['MigrationClass']] = classes.get(row['MigrationClass'], 0) + 1
    sys.stdout.write('%d capabilities, all eighteen fields present, %d needing '
            'EA validation\n' % (len(rows), consulted))
    for name in sorted(classes):
        sys.stdout.write('  %-24s %2d\n' % (name, classes[name]))
    return 0


if __name__ == '__main__':
    sys.exit(main())
