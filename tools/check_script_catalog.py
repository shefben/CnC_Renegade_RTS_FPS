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
DECLARE = re.compile(r'\bDECLARE_SCRIPT(?:_MERGED|_TT)?\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,')

#	the same registration without a class body
REGISTER = re.compile(r'\bREGISTER_SCRIPT(?:_MERGED|_TT)?\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,')

#	a 4.8.4 script whose registered name is not its class name -- the name is
#	the second argument, and the class name is not a catalog entry at all
NAMED = re.compile(
    r'\b(?:DECLARE|REGISTER)_SCRIPT_TT_NAMED\s*\(\s*[A-Za-z_][A-Za-z0-9_]*\s*,'
    r'\s*"([^"]*)"')

#	a merged script the 4.8.4 package also registered under names of its own.
#	Every one has to be unique across the catalog, so all are collected: the
#	class name here, and the semicolon-separated alias list just below.
ALIAS = re.compile(
    r'\b(?:DECLARE|REGISTER)_SCRIPT_MERGED_ALIAS\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,')

#	The alias list is the third argument, and the second argument may be a
#	macro rather than a string literal, so the call is split into arguments
#	rather than matched whole.
ALIAS_CALL = re.compile(r'\b(?:DECLARE|REGISTER)_SCRIPT_MERGED_ALIAS\s*\(')

#	A macro defined in this tree whose body registers a script is itself a
#	registration: every call of it registers the script it is handed.
MACRO_DEFINE = re.compile(r'^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(')


def split_arguments(text, open_paren):
    """Split one macro call into its top-level arguments.

    `open_paren` indexes the `(`.  Commas inside nested parentheses or inside
    string literals do not separate arguments -- a script parameter string is
    very little but commas.  Returns None if the call is never closed.
    """
    args = []
    current = []
    depth = 0
    index = open_paren
    length = len(text)

    while index < length:
        char = text[index]

        if char == '"':
            end = index + 1
            while end < length and text[end] != '"':
                end += 2 if text[end] == '\\' else 1
            current.append(text[index:end + 1])
            index = end + 1
            continue

        if char == '(':
            depth += 1
            if depth == 1:
                index += 1
                continue

        elif char == ')':
            depth -= 1
            if depth == 0:
                args.append(''.join(current).strip())
                return args

        elif char == ',' and depth == 1:
            args.append(''.join(current).strip())
            current = []
            index += 1
            continue

        current.append(char)
        index += 1

    return None


def literal_names(argument):
    """The names in an alias argument: one string literal, semicolons inside."""
    if argument is None or not argument.startswith('"') or not argument.endswith('"'):
        return []

    return [name.strip() for name in argument[1:-1].split(';')]


def registration_macros(text):
    """Names of macros defined here whose body expands to a registration."""
    names = set()
    lines = text.split('\n')

    for index, line in enumerate(lines):
        match = MACRO_DEFINE.match(line)
        if match is None:
            continue

        #	the body is this line and every continuation of it
        body = [line]
        cursor = index
        while lines[cursor].rstrip().endswith('\\') and cursor + 1 < len(lines):
            cursor += 1
            body.append(lines[cursor])

        name = match.group(1)

        #	the registration primitives are matched by their own patterns
        #	above; counting their calls again here would double every
        #	script in the tree.
        if name.startswith(('DECLARE_SCRIPT', 'REGISTER_SCRIPT')):
            continue

        if 'REGISTER_SCRIPT' in '\n'.join(body[1:] or body):
            names.add(name)

    return names

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


def in_macro(lines, line):
    """True when this line is part of a `#define` body.

    The registration macros expand a script name, so their own text reads
    exactly like a registration of a script called `x`.  A continuation line
    is not a `#define` line itself, so walk back over the trailing
    backslashes to find out whether one started it.
    """
    index = line - 1
    if lines[index].lstrip().startswith('#define'):
        return True

    while index > 0 and lines[index - 1].rstrip().endswith('\\'):
        index -= 1
        if lines[index].lstrip().startswith('#define'):
            return True

    return False

def scan(root):
    sources = []
    for dirpath, _dirnames, filenames in os.walk(root):
        for name in filenames:
            if not name.lower().endswith(('.cpp', '.h')):
                continue

            path = os.path.join(dirpath, name)
            with open(path, 'r', encoding='utf-8', errors='replace') as handle:
                sources.append(
                    (path, blank_if_zero(blank_comments(handle.read()))))

    #	A macro of this tree that expands to a registration registers whatever
    #	it is handed, so its calls count too.  It may be defined in one file
    #	and called from another, so all of them are collected first.
    macros = set()
    for _path, body in sources:
        macros |= registration_macros(body)

    macro_call = None
    if macros:
        macro_call = re.compile(
            r'\b(?:' + '|'.join(sorted(macros)) + r')\s*\(\s*'
            r'([A-Za-z_][A-Za-z0-9_]*)\s*,')

    found = {}

    def record(script, path, line):
        script = script.strip()
        if script:
            found.setdefault(script.lower(), []).append((script, path, line))

    for path, body in sources:
        lines = body.split('\n')

        patterns = [DECLARE, REGISTER, REGISTRANT, NAMED, ALIAS]
        if macro_call is not None:
            patterns.append(macro_call)

        for pattern in patterns:
            for match in pattern.finditer(body):
                line = body.count('\n', 0, match.start()) + 1

                #	the macro definitions themselves are not registrations
                if in_macro(lines, line):
                    continue

                record(match.group(1), path, line)

        #	the alias list, which needs the call split into arguments
        for match in ALIAS_CALL.finditer(body):
            line = body.count('\n', 0, match.start()) + 1
            if in_macro(lines, line):
                continue

            args = split_arguments(body, match.end() - 1)
            if args is None or len(args) < 3:
                continue

            for script in literal_names(args[2]):
                record(script, path, line)

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
