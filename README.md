# rpkCpp
Modernized version of RenderPark radiosity engine

Original software from 2001 is available on
[https://graphics.cs.kuleuven.be/renderpark/](https://graphics.cs.kuleuven.be/renderpark/).

This is a full rewrite with the following features:
- Code ported from a mix of K&C + ancient C++ to C++ 2011.
- Project controlled by cmake instead of make.
- Motif GUI removed in favor of basic command line only basic algorithms.
- Code syntax and format modernized.
- Code compiling without warning on modern C++ compilers.
- Reorganization in to new packages structure for clarity at software architecture level.
- Memory leak free after valgrind analysis.
- New java-style basic datastructures.
- Lots of dead code removal.
- Lots of reworkings on moving #define macros to inline functions, functions to
  class methods, revisited in-code comments documentation, standarized naming style and
  much more.
