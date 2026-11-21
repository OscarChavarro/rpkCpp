#!/usr/bin/env python3
"""T0 duplicate-class audit of the Java port's class path.

Collects the top-level types (class, interface, enum, record) declared by the
sources of the Java modules that run together (the base library, the
RenderPark application and the ray corpus replay) and fails when one fully
qualified name is declared by two different files, or when a file's package
does not match its directory, except for the names listed in
classAudit.allow. javac rejects a duplicate inside one module, but two
modules on one class path may each declare the same class: the class loader
then takes whichever comes first, silently, which is the Java form of the C++
port's ODR violation. A file whose package does not match its directory
breaks the path mapping of vitralSubset.sh and vitralClosure.py.

The C++ audit's include guard check has no Java counterpart (Java has no
textual inclusion), and the same simple name in two packages is legal in
Java, so neither is checked here.

    classAudit.py

Usable by hand from any directory.
"""
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
SOURCE_ROOTS = [
    os.path.join(PROJECT_DIR, 'base', 'src', 'main'),
    os.path.join(PROJECT_DIR, 'testsuite', 'ApplicationCases', 'RenderparkApplication', 'src', 'main', 'java'),
    os.path.join(PROJECT_DIR, 'testsuite', 'ApplicationCases', 'RayCorpusReplay', 'src', 'main', 'java'),
]
COMMENT = re.compile(r'/\*.*?\*/|//[^\n]*', re.S)
STRING = re.compile(r'"(?:\\.|[^"\\])*"')
PACKAGE = re.compile(r'^\s*package\s+([\w.]+)\s*;', re.M)
TOP_LEVEL = re.compile(
    r'^(?:(?:public|final|abstract|sealed|non-sealed|strictfp)\s+)*'
    r'(?:class|interface|enum|record|@interface)\s+([A-Za-z_]\w*)', re.M)


def sources():
    for root in SOURCE_ROOTS:
        for directory, _, names in os.walk(root):
            for name in names:
                if name.endswith('.java'):
                    yield root, os.path.join(directory, name)


def main():
    definitions = {}
    misplaced = []
    for root, path in sources():
        with open(path, encoding='utf-8', errors='replace') as f:
            text = STRING.sub('""', COMMENT.sub(' ', f.read()))
        relative = os.path.relpath(path, PROJECT_DIR)
        match = PACKAGE.search(text)
        package = match.group(1) if match else ''
        expected = os.path.relpath(os.path.dirname(path), root).replace(os.sep, '.')
        if expected == '.':
            expected = ''
        if package != expected:
            misplaced.append((relative, package))
        for name in TOP_LEVEL.findall(text):
            qualified = package + '.' + name if package else name
            definitions.setdefault(qualified, set()).add(relative)
    allowed = set()
    allowFile = os.path.join(SCRIPT_DIR, 'classAudit.allow')
    if os.path.isfile(allowFile):
        with open(allowFile) as f:
            for line in f:
                line = line.split('#', 1)[0].strip()
                if line:
                    allowed.add(line)
    failures = 0
    for name in sorted(definitions):
        files = definitions[name]
        if len(files) < 2:
            continue
        if name in allowed:
            print('classAudit: known duplicate class %s: %s' % (name, ' '.join(sorted(files))))
            continue
        print('classAudit: ERROR class %s is declared by %s' % (name, ' '.join(sorted(files))))
        failures += 1
    for relative, package in misplaced:
        if relative in allowed:
            print('classAudit: known misplaced file %s (package %s)' % (relative, package or '<default>'))
            continue
        print('classAudit: ERROR %s declares package %s, not its directory' % (relative, package or '<default>'))
        failures += 1
    if failures:
        print('classAudit: %d duplicate class name(s) or misplaced file(s) on the class path' % failures)
        return 1
    print('classAudit: no new duplicate class names or misplaced files on the class path')
    return 0


if __name__ == '__main__':
    sys.exit(main())
