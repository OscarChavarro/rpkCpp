#!/usr/bin/env python3
"""T0 duplicate-class audit of the link closure.

Collects the headers reachable through #include "..." from the compiled
sources of the three targets (base library without render/opengl, the
vitral-opengl library and the RenderPark application) and fails when one
class name is defined by two different headers of that closure, or when two
headers of the closure open with the same include guard, except for the names
listed in classAudit.allow. Two definitions of one class linked together are
an ODR violation that the linker resolves silently; two headers sharing a guard
make the second one vanish from any unit that includes both.

    classAudit.py [--raytracing on|off]

Run by the build (target classAudit) and usable by hand from any directory.
"""
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
INCLUDE_ROOTS = [
    os.path.join(PROJECT_DIR, 'base', 'src', 'main'),
    os.path.join(PROJECT_DIR, 'opengl', 'src', 'main'),
]
SOURCE_ROOTS = [
    (os.path.join(PROJECT_DIR, 'base', 'src', 'main'), ['/render/opengl/']),
    (os.path.join(PROJECT_DIR, 'opengl', 'src'), []),
    (os.path.join(PROJECT_DIR, 'testsuite', 'ApplicationCases', 'RenderparkApplication', 'src'), []),
]
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
GUARD = re.compile(r'^\s*#\s*ifndef\s+([A-Za-z_]\w*)', re.M)
CLASS = re.compile(r'^(?:class|struct)\s+([A-Za-z_]\w*)(?:\s+final)?\s*(?::[^;{]*)?\{', re.M)


def compiledSources(raytracing):
    for root, excluded in SOURCE_ROOTS:
        for directory, _, names in os.walk(root):
            for name in names:
                path = os.path.join(directory, name)
                if not name.endswith('.cpp') or '/build/' in path:
                    continue
                if any(e in path for e in excluded):
                    continue
                if not raytracing and '/raycasting/' in path:
                    continue
                yield path


def resolve(include, fromFile):
    for root in [os.path.dirname(fromFile)] + INCLUDE_ROOTS:
        candidate = os.path.normpath(os.path.join(root, include))
        if os.path.isfile(candidate):
            return candidate
    return None


def closure(sources):
    seen = set()
    pending = list(sources)
    while pending:
        path = pending.pop()
        if path in seen:
            continue
        seen.add(path)
        with open(path, encoding='utf-8', errors='replace') as f:
            text = f.read()
        for include in INCLUDE.findall(text):
            target = resolve(include, path)
            if target is not None and target not in seen:
                pending.append(target)
    return seen


def main():
    raytracing = not ('--raytracing' in sys.argv and sys.argv[sys.argv.index('--raytracing') + 1] == 'off')
    definitions = {}
    guards = {}
    for path in closure(compiledSources(raytracing)):
        if not path.endswith(('.h', '.txx')):
            continue
        with open(path, encoding='utf-8', errors='replace') as f:
            text = f.read()
        relative = os.path.relpath(path, PROJECT_DIR)
        for name in CLASS.findall(text):
            definitions.setdefault(name, set()).add(relative)
        guard = GUARD.search(text)
        if path.endswith('.h') and guard is not None:
            guards.setdefault(guard.group(1), set()).add(relative)
    allowed = set()
    allowFile = os.path.join(SCRIPT_DIR, 'classAudit.allow')
    if os.path.isfile(allowFile):
        with open(allowFile) as f:
            for line in f:
                line = line.split('#', 1)[0].strip()
                if line:
                    allowed.add(line)
    failures = 0
    for kind, table in (('class', definitions), ('include guard', guards)):
        for name in sorted(table):
            headers = table[name]
            if len(headers) < 2:
                continue
            if name in allowed:
                print('classAudit: known duplicate %s %s: %s' % (kind, name, ' '.join(sorted(headers))))
                continue
            print('classAudit: ERROR %s %s is used by %s' % (kind, name, ' '.join(sorted(headers))))
            failures += 1
    if failures:
        print('classAudit: %d duplicate class or include guard name(s) in the link closure' % failures)
        return 1
    print('classAudit: no new duplicate class or include guard names in the link closure')
    return 0


if __name__ == '__main__':
    sys.exit(main())
