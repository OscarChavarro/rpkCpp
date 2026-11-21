#!/usr/bin/env python3
"""Closure inventory of the VITRAL headers that a slice of the alignment needs.

Follows #include "..." (and the sibling .cpp/.txx of each header) from the
seed headers inside VITRAL's own Turbo C++ port (turboc/base/src/main of a
VITRAL checkout), and classifies every file of the closure against this
port's turboc/base/src/main:

    shared-identical   present in both trees with the same bytes
    shared-different   present in both trees with different bytes (must be
                       resolved to one pinned implementation first)
    vitral-only        enters RPK with the slice

and lists every class the closure defines that RPK already defines in
another header (a name collision that the D2 rename map must clear before
the header enters the tree).

    vitralClosure.py <VITRAL checkout> [seed header relative to src/main ...]

Without seeds it uses the first-slice seeds of doc/vitralAlignmentPlan.md.
Exit status 1 when a collision or a shared-different file remains.

Turbo C++ port note: the cpp port's script reads VITRAL's cpp/base/src/main;
this one reads VITRAL's turboc/base/src/main, the files this port takes
unchanged (scripts/vitralSubset.txt). Class definitions are found with the
same pattern: the Turbo C++ subset has no `final`, which the pattern allows.
"""
import os
import re
import sys
from collections import Counter

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
RPK_MAIN = os.path.join(os.path.dirname(SCRIPT_DIR), 'base', 'src', 'main')
FIRST_SLICE = [
    'vsdk/toolkit/environment/material/Material.h',
    'vsdk/toolkit/environment/geometry/surface/TriangleMesh.h',
    'vsdk/toolkit/environment/geometry/surface/QuadMesh.h',
    'vsdk/toolkit/environment/geometry/element/Triangle.h',
    'vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/PreRayHitElement.h',
    'vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/TriangleAccelerationStructure.h',
    'vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BoundingVolumeHierarchy.h',
    'vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BinnedSahHierarchy.h',
]
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
CLASS = re.compile(r'^(?:class|struct)\s+([A-Za-z_]\w*)(?:\s+final)?\s*(?::[^;{]*)?\{', re.M)


def read(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        return f.read()


def closure(vitralMain, seeds):
    seen = set()
    pending = list(seeds)
    while pending:
        name = pending.pop()
        if name in seen or not os.path.isfile(os.path.join(vitralMain, name)):
            continue
        seen.add(name)
        pending.extend(INCLUDE.findall(read(os.path.join(vitralMain, name))))
        stem = name.rsplit('.', 1)[0]
        for extension in ('.cpp', '.txx'):
            sibling = stem + extension
            if sibling != name and os.path.isfile(os.path.join(vitralMain, sibling)):
                pending.append(sibling)
    return seen


def rpkClasses():
    classes = {}
    for directory, _, names in os.walk(RPK_MAIN):
        for name in names:
            if name.endswith('.h'):
                path = os.path.join(directory, name)
                for c in CLASS.findall(read(path)):
                    classes.setdefault(c, []).append(os.path.relpath(path, RPK_MAIN))
    return classes


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    vitralMain = os.path.join(sys.argv[1], 'turboc', 'base', 'src', 'main')
    seeds = sys.argv[2:] or FIRST_SLICE
    files = sorted(closure(vitralMain, seeds))
    classes = rpkClasses()
    states = Counter()
    problems = 0
    for name in files:
        rpkPath = os.path.join(RPK_MAIN, name)
        if not os.path.isfile(rpkPath):
            state = 'vitral-only'
        elif read(rpkPath) == read(os.path.join(vitralMain, name)):
            state = 'shared-identical'
        else:
            state = 'shared-different'
        states[state] += 1
        collisions = []
        if name.endswith('.h'):
            for c in CLASS.findall(read(os.path.join(vitralMain, name))):
                collisions += ['%s (RPK %s)' % (c, p) for p in classes.get(c, []) if p != name]
        print('%-17s %s%s' % (state, name, ('  COLLIDES ' + ', '.join(collisions)) if collisions else ''))
        if collisions or state == 'shared-different':
            problems += 1
    print('%d files: %s; %d to resolve' % (len(files), ', '.join('%d %s' % (n, s) for s, n in sorted(states.items())), problems))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
