# C++ Notes for rpkCpp

This document contains the C++-specific checklist and modernization notes.
For program usage, screenshots and run/build examples, see the main README:
[../README.md](../README.md).

## Modernized features from original ANSI C to current C++-11 code

This is a full rewrite with the following features:
- Project controlled by CMake instead of make.
- Code ported from a mix of K&C + ancient C++ to C++ 2011.
- Avoid usage of very old coding styles:
  - Avoid using union structs.
  - Avoid using goto in favor of structured programming.
  - Avoid using pointer arithmetic.
- Custom-made operations and data structures replaced in favor of standard ones:
  - Pools library for memory allocator replaced by standard ANSI C malloc (intermediate step).
  - ANSI C malloc/free operators replaced by C++ new/delete (final step).
  - New Java-style basic datastructures.
- Motif GUI removed in favor of command line only basic algorithms.
- Code syntax and format modernized.
- Code compiling without warnings on modern C++ compilers.
- Some clean code ideas applied:
  - Most smells from SonarLint/clang-tidy cleaned.
  - Variable names refactor to be self-explanatory.
  - Removing redundant comments from code that is self-explanatory.
  - Const parameter and methods used when possible to reinforce read-only / immutable elements.
  - Enums and classes organized on its own header modules, avoiding the inclusion of several structures on single module.
- Exotic C++ specific features unused (makes it easy to port code to other languages):
  - Vanilla printf style functions preferred over `<<` streams.
  - Avoid the use of iterators and STL library.
  - Avoid the use of operator overload in favor of old style plain class methods.
  - Avoid the use of pre-processor based (`#define`) macros for constants in favor of const typed variables.
  - Avoid the use of pre-processor based (`#define`) macros for functions in favor of inline methods.
  - Avoid the use of pre-processor based (`#define`) macros for generic code.
- Reorganization in to new packages structure for clarity at software architecture level.
- Memory leak free after valgrind analysis.
- Use of global variables set to a minimum possible, replaced by parameters.
- Lots of dead code removal.
- Lots of reworkings on moving `#define` macros to inline functions, functions to class methods,
  revisited in-code comments documentation, standard naming style and much more.
- References to papers and books on code has been downloaded to `../doc` folder to ease the algorithm study process.
- Ported to several architectures, including MacOSX, Windows, Linux on Intel/x86, ARM, RiscV, etc.
- Proper class hierarchies introduced where their application is evident.
- Object-oriented programming implemented using C++ features:
  - Using `static_cast` and `reinterpret_cast` instead of simple old C++-style casts.
  - Replacing pointer to callback functions with class hierarchies using inheritance overloaded methods.

## Modernized OOP design

Some clean code and SOLID object-oriented programming (OOP) principles and best practices has been applied:
- All functions and variables where organized in to classes (methods and attributes).
- Single class or enum per `.h`/`.cpp` module.
- Inspired in ports and adapters / hexagonal architecture:
  - Infrastructure code has been decoupled from business logic.
  - Basic operating system, filesystem and other operations where abstracted in a Java-like organization targeted to migrate this program to other languages easily.
  - There are no system `#include` classes in headers outside "java" abstraction. This means all classes are "pure" business logic.
- No friend classes or methods.
- Keyword `using` instead of old-fashioned `typedef`.

## Re-entrant code

Basic original code has been carefully converted to re-entrant code:
- All variables are class members (attributes).
- There are no global variables in the project, and no global variables on the module scope.
- There are no static variables.
- Immutable variables are annotated as const.
- Code does not depend on external libraries, so re-entrant issues and share resource issues are minimized.
- Basic non re-entrant functions as such `strtok` are not used.
- Methods are idempotent? (pending to check).

Note that operations does not use `mutex` or other sync methods, that responsibilities are on higher level in the using application.

Input/output package is not re-entrant.

## Using clang-tidy linter

Can check pending warnings/smells using:

```bash
clang-tidy -checks='-*,modernize-use-nullptr,google-readability-casting' src/**/*.cpp -- -std=c++17 -Isrc -I/usr/include/c++/ -I/usr/include/c++/11 -I/usr/include/x86_64-linux-gnu/c++/11
```
