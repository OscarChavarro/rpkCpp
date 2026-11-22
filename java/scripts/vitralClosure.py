#!/usr/bin/env python3
"""Closure inventory of the VITRAL Java classes that a slice of the alignment needs.

Follows the dependencies of the seed files inside VITRAL's java/base/src/main
(imports, wildcard imports, fully qualified vsdk.* names and the classes of
the same package that a file names without an import, unless the file
declares a nested type of that name) and classifies every
file of the closure against this port's VITRAL copy, java/base/src/main:

    shared-identical   present in both trees with the same bytes
    shared-different   present in both trees with different bytes (must be
                       resolved to one pinned implementation first)
    vitral-only        enters the Java port with the slice
    rpk-path           absent from base/ but present at the same path in
                       rpkBase/ or rpkOpenGL1/ (RenderPark's own copy)

and lists, for every class of the closure, the classes of the Java port
(base/, rpkBase/ or rpkOpenGL1/) with
the same simple name in another package. Java packages keep such classes
apart, so they are not errors as in the C++ port (where the D2 rename map
must clear them): they are listed because a file that imports both needs a
qualified name, and because the two ports' class names must be aligned
before a shared file can use either.

    vitralClosure.py <VITRAL checkout> [seed file relative to src/main ...]

Without seeds it uses the first-slice seeds of doc/vitralAlignmentPlan.md.
Exit status 1 when a shared-different or rpk-path file remains.
"""
import os
import re
import sys
from collections import Counter

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JAVA_DIR = os.path.dirname(SCRIPT_DIR)
RPK_MAIN = os.path.join(JAVA_DIR, 'base', 'src', 'main')
RPK_OWN_MAINS = [
    os.path.join(JAVA_DIR, 'rpkBase', 'src', 'main'),
    os.path.join(JAVA_DIR, 'rpkOpenGL1', 'src', 'main'),
]
FIRST_SLICE = [
    'vsdk/toolkit/environment/material/Material.java',
    'vsdk/toolkit/environment/geometry/surface/TriangleMesh.java',
    'vsdk/toolkit/environment/geometry/surface/QuadMesh.java',
    'vsdk/toolkit/environment/geometry/element/Triangle.java',
    'vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/PreRayHitElement.java',
    'vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/TriangleAccelerationStructure.java',
    'vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BoundingVolumeHierarchy.java',
    'vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BinnedSahHierarchy.java',
]
IMPORT = re.compile(r'^\s*import\s+(static\s+)?([\w.]+)(\.\*)?\s*;', re.M)
QUALIFIED = re.compile(r'\b((?:vsdk|java|javax)\.(?:\w+\.)+[A-Z]\w*)')
IDENTIFIER = re.compile(r'\b_?[A-Z]\w*\b')
DECLARATION = re.compile(r'\b(?:class|interface|enum|record)\s+(_?[A-Z]\w*)')
COMMENT = re.compile(r'/\*.*?\*/|//[^\n]*', re.S)
STRING = re.compile(r'"(?:\\.|[^"\\])*"')


def read(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        return f.read()


def code(text):
    return STRING.sub('""', COMMENT.sub(' ', text))


def packageFiles(main, package):
    directory = os.path.join(main, package.replace('.', os.sep))
    if not os.path.isdir(directory):
        return []
    return sorted(os.path.join(package.replace('.', '/'), name)
                  for name in os.listdir(directory) if name.endswith('.java'))


def typeFile(main, qualifiedName):
    """The file of a type name, trying a.b.C.java, then a.b.C (nested D) for a.b.C.D"""
    parts = qualifiedName.split('.')
    while len(parts) > 1:
        candidate = '/'.join(parts) + '.java'
        if os.path.isfile(os.path.join(main, candidate)):
            return candidate
        parts = parts[:-1]
    return None


def dependencies(main, name):
    text = code(read(os.path.join(main, name)))
    found = set()
    for static, target, wildcard in IMPORT.findall(text):
        if wildcard and not static:
            found.update(packageFiles(main, target))
        else:
            file = typeFile(main, target)
            if file is not None:
                found.add(file)
    for target in QUALIFIED.findall(text):
        file = typeFile(main, target)
        if file is not None:
            found.add(file)
    package = os.path.dirname(name).replace('/', '.')
    used = set(IDENTIFIER.findall(text)) - set(DECLARATION.findall(text))
    for sibling in packageFiles(main, package):
        if os.path.basename(sibling)[:-5] in used:
            found.add(sibling)
    found.discard(name)
    return found


def closure(vitralMain, seeds):
    seen = set()
    pending = list(seeds)
    while pending:
        name = pending.pop()
        if name in seen or not os.path.isfile(os.path.join(vitralMain, name)):
            continue
        seen.add(name)
        pending.extend(dependencies(vitralMain, name))
    return seen


def rpkClasses():
    classes = {}
    for main in [RPK_MAIN] + RPK_OWN_MAINS:
        for directory, _, names in os.walk(main):
            for name in names:
                if name.endswith('.java'):
                    relative = os.path.relpath(os.path.join(directory, name), main)
                    if main != RPK_MAIN:
                        relative = os.path.join(os.path.basename(os.path.dirname(os.path.dirname(main))), relative)
                    classes.setdefault(name[:-5], []).append(relative)
    return classes


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    vitralMain = os.path.join(sys.argv[1], 'java', 'base', 'src', 'main')
    if not os.path.isdir(vitralMain):
        print('No VITRAL Java sources at %s' % vitralMain)
        return 2
    seeds = sys.argv[2:] or FIRST_SLICE
    files = sorted(closure(vitralMain, seeds))
    classes = rpkClasses()
    states = Counter()
    problems = 0
    sameNames = 0
    for name in files:
        rpkPath = os.path.join(RPK_MAIN, name)
        if not os.path.isfile(rpkPath):
            if any(os.path.isfile(os.path.join(m, name)) for m in RPK_OWN_MAINS):
                state = 'rpk-path'
            else:
                state = 'vitral-only'
        elif read(rpkPath) == read(os.path.join(vitralMain, name)):
            state = 'shared-identical'
        else:
            state = 'shared-different'
        states[state] += 1
        others = [p for p in classes.get(os.path.basename(name)[:-5], []) if p != name]
        if others:
            sameNames += 1
        print('%-17s %s%s' % (state, name, ('  SAME NAME AS ' + ', '.join(sorted(others))) if others else ''))
        if state in ('shared-different', 'rpk-path'):
            problems += 1
    print('%d files: %s; %d to resolve; %d share a simple name with a class of another package'
          % (len(files), ', '.join('%d %s' % (n, s) for s, n in sorted(states.items())), problems, sameNames))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
