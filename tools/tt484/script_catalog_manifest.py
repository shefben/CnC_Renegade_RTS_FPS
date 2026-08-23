"""Write the checked-in list of built-in script names the catalog tests expect.

`tools/check_script_catalog.py` reads the source and proves the catalog is
one-name-one-script.  It cannot prove that a script which is written down
actually reaches the runtime registry: the registrants are file-scope objects
nothing references, so a linker change, a build-system change, or a static
initialisation change can silently drop a whole file's worth of scripts and
every source-level check still passes.

This writes what the source says into a header, and
`Code/Scripts/tests/ScriptCatalogTests.cpp` compares it against what
`NativeScriptRegistry` actually holds at run time.  The two disagree exactly
when a registration stopped arriving.

Usage: script_catalog_manifest.py [<source-dir>] [<output-header>]
"""
import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

sys.path.insert(0, ROOT)

import check_script_catalog as catalog

#	the 4.8.4 forms.  A name reached by one of these is a script the TT package
#	brought with it; everything else in the catalog is stock, or stock with the
#	4.8.4 corrections merged into it.
TT_DECL = re.compile(
    r'\b(?:DECLARE|REGISTER)_SCRIPT_TT\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,')

TT_NAMED = re.compile(
    r'\b(?:DECLARE|REGISTER)_SCRIPT_TT_NAMED\s*\(\s*[A-Za-z_][A-Za-z0-9_]*\s*,'
    r'\s*"([^"]*)"')

MERGED = re.compile(
    r'\b(?:DECLARE|REGISTER)_SCRIPT_MERGED(?:_ALIAS)?\s*\('
    r'\s*([A-Za-z_][A-Za-z0-9_]*)\s*,')


SRC_LIST = re.compile(r'set\s*\(\s*SCRIPTS_SRC\b(.*?)\n\)', re.DOTALL)


def built(root):
    """The file names `SCRIPTS_SRC` compiles, lower-cased.

    Not every source in the directory is in the build.  Several are Westwood
    leftovers written against a script interface that never shipped, and one is
    truncated; they have never compiled.  The scripts they declare are not in
    the catalog, so they must not be in the manifest either -- what the build
    list says is what the game holds.
    """
    path = os.path.join(root, 'CMakeLists.txt')

    with io.open(path, encoding='utf-8', errors='replace') as handle:
        text = handle.read()

    match = SRC_LIST.search(text)
    if match is None:
        raise SystemExit('no SCRIPTS_SRC list in %s' % path)

    names = set()
    for line in match.group(1).split('\n'):
        line = line.split('#', 1)[0].strip()
        if line.lower().endswith(('.cpp', '.h')):
            names.add(line.lower())

    if not names:
        raise SystemExit('SCRIPTS_SRC in %s is empty' % path)

    return names


def sources(root):
    """The source files the build compiles, comments and `#if 0` blanked."""
    wanted = built(root)

    found = []
    for dirpath, _dirnames, filenames in os.walk(root):
        for name in sorted(filenames):
            if not name.lower().endswith(('.cpp', '.h')):
                continue
            if name.lower() not in wanted:
                continue
            path = os.path.join(dirpath, name)
            with io.open(path, encoding='utf-8', errors='replace') as handle:
                body = catalog.blank_if_zero(catalog.blank_comments(handle.read()))
            found.append((path, body))
    return found


MACRO_BODY = re.compile(r'^\s*#\s*define\s+%s\s*\(')


def macro_flavour(text, macro):
    """Which registration primitive `macro`'s body expands to."""
    lines = text.split('\n')
    opener = re.compile(MACRO_BODY.pattern % re.escape(macro))

    for index, line in enumerate(lines):
        if opener.match(line) is None:
            continue

        body = [line]
        cursor = index
        while lines[cursor].rstrip().endswith('\\') and cursor + 1 < len(lines):
            cursor += 1
            body.append(lines[cursor])

        joined = '\n'.join(body)

        if 'REGISTER_SCRIPT_TT' in joined:
            return 'tt'
        if 'REGISTER_SCRIPT_MERGED' in joined:
            return 'merged'
        return 'stock'

    return 'stock'


def tagged(root):
    """The catalog, split into the sets the tests want to check separately.

    `check_script_catalog.scan` counts an alias as a catalog entry, because for
    its purpose -- one name, one script -- an alias is a name like any other.
    A factory is not created for one, though: the registry reaches it through
    `Alias_Matches` on the script it belongs to.  So the two are separated
    here, and the union is checked against `scan` so the two tools cannot drift
    apart.
    """
    body_of = sources(root)

    #
    #	The macros of this tree that expand to a registration, which may be
    #	defined in one file and called from another.  Which primitive a macro
    #	expands to is what says whether the scripts it makes are 4.8.4's, so
    #	they are grouped by that rather than lumped in with the stock ones.
    #
    macros = {}
    for _path, body in body_of:
        for name in catalog.registration_macros(body):
            macros[name] = macro_flavour(body, name)

    calls = {}
    for flavour in set(macros.values()):
        named = sorted(name for name in macros if macros[name] == flavour)
        calls[flavour] = re.compile(
            r'\b(?:' + '|'.join(named) + r')\s*\(\s*'
            r'([A-Za-z_][A-Za-z0-9_]*)\s*,')

    factories = set()
    tt = set()
    merged = set()
    aliases = set()

    for _path, body in body_of:
        lines = body.split('\n')

        #	`re.compile` hands back the same object for two identical pattern
        #	strings, so which set a match belongs in travels alongside the
        #	pattern rather than being recovered from its identity.
        passes = [
            (factories, catalog.DECLARE),
            (factories, catalog.REGISTER),
            (factories, catalog.REGISTRANT),
            (factories, catalog.NAMED),
            (factories, catalog.ALIAS),
            (tt, TT_DECL),
            (tt, TT_NAMED),
            (merged, MERGED),
        ]
        for flavour, pattern in calls.items():
            passes.append(({'tt': tt, 'merged': merged}.get(flavour, factories),
                    pattern))
            if flavour != 'stock':
                passes.append((factories, pattern))

        for sink, pattern in passes:
            for match in pattern.finditer(body):
                line = body.count('\n', 0, match.start()) + 1
                if catalog.in_macro(lines, line):
                    continue

                name = match.group(1).strip()
                if name:
                    sink.add(name)

        #	the second name a merged script answers to
        for match in catalog.ALIAS_CALL.finditer(body):
            line = body.count('\n', 0, match.start()) + 1
            if catalog.in_macro(lines, line):
                continue
            args = catalog.split_arguments(body, match.end() - 1)
            if args is None or len(args) < 3:
                continue
            for name in catalog.literal_names(args[2]):
                if name.strip():
                    aliases.add(name.strip())

    #	a name that is both a script and somebody's alias is a script
    aliases -= factories

    unknown = (tt | merged) - factories
    if unknown:
        raise SystemExit(
            'tagged names missing from the catalog: %s'
            % ', '.join(sorted(unknown)))

    #
    #	`check_script_catalog.py` reads the whole directory, so what it counts
    #	is a superset: the scripts declared in the files the build leaves out.
    #	Anything this tool found that it did not is a disagreement about the
    #	files they share, which is a bug in one of them.
    #
    scanned = set(sites[0][0] for sites in catalog.scan(root).values())
    drift = (factories | aliases) - scanned
    if drift:
        raise SystemExit(
            'disagrees with check_script_catalog.py on: %s'
            % ', '.join(sorted(drift)))

    unbuilt = scanned - (factories | aliases)

    return factories, tt, merged, aliases, unbuilt


def emit_array(handle, name, values):
    handle.write(u'const char * const %s[] = {\n' % name)
    for value in sorted(values, key=lambda text: text.lower()):
        if '"' in value or '\\' in value:
            raise SystemExit('script name is not a plain identifier: %s' % value)
        handle.write(u'\t"%s",\n' % value)
    handle.write(u'};\n\n')


def main(argv):
    root = argv[1] if len(argv) > 1 else os.path.join(
        os.path.dirname(ROOT), 'Code', 'Scripts')
    output = argv[2] if len(argv) > 2 else os.path.join(
        root, 'tests', 'ScriptCatalogManifest.h')

    factories, tt, merged, aliases, unbuilt = tagged(root)
    stock = factories - tt

    with io.open(output, 'w', encoding='utf-8', newline='\n') as handle:
        handle.write(u'''//
//\tGenerated by tools/tt484/script_catalog_manifest.py -- do not edit.
//
//\tWhat the source says the built-in script catalog contains.  The catalog
//\ttests compare this against what NativeScriptRegistry actually holds, which
//\tis the only way to notice a registration that stopped arriving.
//

#ifndef\tSCRIPTCATALOGMANIFEST_H
#define\tSCRIPTCATALOGMANIFEST_H

namespace ScriptCatalogManifest {

''')
        handle.write(u'const int ScriptCount = %d;\n' % len(factories))
        handle.write(u'const int StockCount = %d;\n' % len(stock))
        handle.write(u'const int TTCount = %d;\n' % len(tt))
        handle.write(u'const int MergedCount = %d;\n' % len(merged))
        handle.write(u'const int AliasCount = %d;\n\n' % len(aliases))

        emit_array(handle, 'StockNames', stock)
        emit_array(handle, 'TTNames', tt)
        emit_array(handle, 'MergedNames', merged)
        emit_array(handle, 'AliasNames', aliases)

        handle.write(u'}\t// namespace ScriptCatalogManifest\n\n')
        handle.write(u'#endif\t// SCRIPTCATALOGMANIFEST_H\n')

    sys.stdout.write(
        '%s: %d scripts (%d stock, of which %d merged; %d TT), %d aliases\n'
        % (os.path.basename(output), len(factories), len(stock), len(merged),
           len(tt), len(aliases)))

    if unbuilt:
        sys.stdout.write(
            '%d script names are declared in files SCRIPTS_SRC leaves out: %s\n'
            % (len(unbuilt), ', '.join(sorted(unbuilt))))

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
