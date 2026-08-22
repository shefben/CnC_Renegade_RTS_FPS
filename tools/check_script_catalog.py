#!/usr/bin/env python3
"""Fail generation when the built-in script catalog is not one-name-one-script.

A built-in script name must resolve to exactly one implementation.  Two
registrations of the same name compile fine and then race during static
initialization, so the catalog is checked here, at configure time, rather than
being discovered at runtime.

Usage: check_script_catalog.py <source-dir> [<source-dir> ...]
"""
import os
import re
import sys

#	class Foo : public ScriptImpClass, declared through the DECLARE_SCRIPT
#	macro -- the class name is the registered name.
DECLARE = re.compile(r'\bDECLARE_SCRIPT\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,')

#	the same registration without a class body
REGISTER = re.compile(r'\bREGISTER_SCRIPT\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,')

#	the raw template form, where the string literal is the registered name and
#	need not equal the class name
REGISTRANT = re.compile(
    r'\bScriptRegistrant\s*<\s*[A-Za-z_][A-Za-z0-9_:]*\s*>\s*'
    r'[A-Za-z_][A-Za-z0-9_]*\s*\(\s*"([^"]*)"')


def blank_comments(text):
    """Replace comment bodies with spaces, keeping every newline in place.

    A commented-out script still contains its DECLARE_SCRIPT, and this catalog
    is full of them, so the scan has to see the code the compiler sees.  Line
    numbers are preserved so the report still points at the real line.
    """
    out = []
    index = 0
    length = len(text)

    while index < length:
        char = text[index]

        if char == '/' and index + 1 < length and text[index + 1] == '*':
            end = text.find('*/', index + 2)
            end = length if end < 0 else end + 2
            out.append(''.join(c if c == '\n' else ' ' for c in text[index:end]))
            index = end

        elif char == '/' and index + 1 < length and text[index + 1] == '/':
            end = text.find('\n', index)
            end = length if end < 0 else end
            out.append(' ' * (end - index))
            index = end

        elif char in ('"', "'"):
            quote = char
            end = index + 1
            while end < length:
                if text[end] == '\\':
                    end += 2
                    continue
                if text[end] == quote or text[end] == '\n':
                    end += 1
                    break
                end += 1
            out.append(text[index:end])
            index = end

        else:
            out.append(char)
            index += 1

    return ''.join(out)


IF_ZERO = re.compile(r'^\s*#\s*if\s+0\s*(?://.*)?$')
IF_ANY = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b')
ELSE_ANY = re.compile(r'^\s*#\s*(else|elif)\b')
ENDIF = re.compile(r'^\s*#\s*endif\b')


def blank_if_zero(text):
    """Blank out `#if 0` blocks, which is how this tree parks dead scripts.

    Not a preprocessor: only the literal `#if 0` form is understood, and only
    down to its matching `#else`, `#elif` or `#endif`.  Anything else is left
    alone, so a script behind a real conditional still counts as registered.
    """
    lines = text.split('\n')
    depth = 0
    zero_at = -1

    for index, line in enumerate(lines):
        if IF_ANY.match(line):
            depth += 1
            if zero_at < 0 and IF_ZERO.match(line):
                zero_at = depth
        elif ELSE_ANY.match(line):
            if zero_at == depth:
                zero_at = -1
        elif ENDIF.match(line):
            if zero_at == depth:
                zero_at = -1
            depth -= 1

        if zero_at > 0:
            lines[index] = ''

    return '\n'.join(lines)


def scan(root):
    found = {}
    for dirpath, _dirnames, filenames in os.walk(root):
        for name in filenames:
            if not name.lower().endswith(('.cpp', '.h')):
                continue

            path = os.path.join(dirpath, name)
            with open(path, 'r', encoding='utf-8', errors='replace') as handle:
                text = blank_if_zero(blank_comments(handle.read()))

            lines = text.split('\n')

            for pattern in (DECLARE, REGISTER, REGISTRANT):
                for match in pattern.finditer(text):
                    script = match.group(1)
                    line = text.count('\n', 0, match.start()) + 1

                    #	the macro definitions themselves are not registrations
                    if lines[line - 1].lstrip().startswith('#define'):
                        continue

                    found.setdefault(script.lower(), []).append(
                        (script, path, line))

    return found


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2

    catalog = {}
    for root in argv[1:]:
        for key, sites in scan(root).items():
            catalog.setdefault(key, []).extend(sites)

    duplicates = {k: v for k, v in catalog.items() if len(v) > 1}

    if duplicates:
        sys.stderr.write(
            'Duplicate built-in script names (%d):\n' % len(duplicates))
        for key in sorted(duplicates):
            sys.stderr.write('  %s\n' % duplicates[key][0][0])
            for script, path, line in duplicates[key]:
                sys.stderr.write('      %s:%d\n' % (path, line))
        return 1

    sys.stdout.write('%d built-in scripts, no duplicate names\n' % len(catalog))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
