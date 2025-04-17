# rpkCpp
Modernized version of RenderPark radiosity engine

Original software from 2001 is available on
[https://graphics.cs.kuleuven.be/renderpark/](https://graphics.cs.kuleuven.be/renderpark/).

This is a full rewrite with the following features:
- Project controlled by cmake instead of make.
- Code ported from a mix of K&C + ancient C++ to C++ 2011.
- Avoid usage of very old coding styles:
  - Avoid using union structs
  - Avoid using goto in favor of structured programming
  - Avoid using pointer arithmetic
- Custom-made operations and data structures replaced in favor of standard ones
  - Pools library for memory allocator replaced by standard Ansi C malloc (intermediate step).
  - Ansi C malloc/free operators replaced by C++ new/delete (final step).
  - New java-style basic datastructures.
- Motif GUI removed in favor of command line only basic algorithms.
- Code syntax and format modernized.
- Code compiling without warnings on modern C++ compilers.
- Some clean code ideas applied
  - Most smells from SonarLint/clang-tidy cleaned.
  - Variable names refactor to be self-explanatory.
  - Removing redundant comments from code that is self-explanatory.
  - Const parameter and methods used when possible to reinforce read-only / immutable elements.
  - Enums and classes organized on its own header modules, avoiding the inclusion of several structures on single module.
- Exotic C++ specific features unused (makes it easy to port code to other languages)
  - Vanilla printf style functions preferred over `<<` streams
  - Avoid the use of iterators and STL library
  - Avoid the use of operator overload in favor of old style plain class methods
  - Avoid the use of pre-processor based (#define) macros for constants in favor of const typed variables
  - Avoid the use of pre-processor based (#define) macros for functions in favor of inline methods
  - Avoid the use of pre-processor based (#define) macros for generic code! (big C issue)
- Reorganization in to new packages structure for clarity at software architecture level.
- Memory leak free after valgrind analysis.
- Use of global variables set to a minimum possible, replaced by parameters.
- Lots of dead code removal.
- Lots of reworkings on moving #define macros to inline functions, functions to
  class methods, revisited in-code comments documentation, standard naming style and
  much more.
- References to papers and books on code has been downloaded to ./doc folder to ease the
  algorithm study process.
- Ported to several architectures, including MacOSX, Windows, Linux on Intel/x86, ARM, RiscV, etc.
- Proper class hierarchies introduced where their application is evident.

## Install prerequisites

### On linux

```bash
apt-get install cmake build-essential libosmesa6-dev findimagedupes
```

### On MacOS

```bash
brew install mesa cmake
```

## Build program

```bash
mkdir build
cd build
cmake ..
make
cd ..
./scripts/runAll.sh
```

Generated images will be written at `./output` folder, reading models from `./etc`.

## Running RPK program from the command line

The following command will run all the samples located on the `./etc` folder and generate output
files on `./output` folder.

```
./scripts/runAll.sh
```

Note that for verification purposes, the output images can be compared against the copies on `./doc/testBaseImages`.
For checking if last run is successful, try

```
./scripts/testReviewResults.sh
```

## What RPK program does

Basically, this program has two use cases:
- Galerkin / geometric subdivision view independent solution.
- Raytracer solution: Check the use of `RAYTRACING_ENABLED` macro on how to remove this code from project.


