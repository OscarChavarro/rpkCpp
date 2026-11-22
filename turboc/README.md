# Turbo C++ Notes for rpkCpp

This is a legacy C++ port aimed to build on Borland Turbo C 3.0 so
project can build for DOS.

## File Renaming Workflow (Long Names vs 8.3)

Development in this repository is done with long, descriptive file names.
The Git repository keeps those long names, and CMake builds are expected to
run with those long names.

For DOS/Turbo C builds, 8.3 file names are required. Use the renaming
mapping and scripts in this folder to switch between both layouts:

- `mappings.csv`: canonical mapping between long paths and 8.3 paths
- `scripts/map_to_83.py`: rename from long names to 8.3 names
- `scripts/map_from_83.py`: restore from 8.3 names back to long names

Recommended flow:

1. Normal development and CMake builds: keep long names.
2. Before DOS/Turbo C compilation: apply 8.3 renaming.
3. After DOS/Turbo C work: restore long names before committing.

Examples:

```bash
# Preview changes only
python3 scripts/map_to_83.py --dry-run

# Apply 8.3 renaming for DOS build
python3 scripts/map_to_83.py

# Restore long names for regular development
python3 scripts/map_from_83.py
```

## C++ Feature Changes (cpp -> turboc)

Changes actually applied in this port are listed here. C++98/C++11 differences.

1. `enum class` (scoped enums) -> traditional `enum`
2. `nullptr` -> `NULL`
3. `override`/`final` removed from class hierarchies
4. `constexpr` replaced with C++98 alternatives (`static const` or macros)
5. C++ casts (`static_cast`, `reinterpret_cast`) -> C-style casts
6. Simplified C++11 initialization -> explicit C++98 forms
   - Uniform initialization: `Type obj{}` -> `Type obj = Type()`
   - Delegating constructor removed in `LookUpTable`
7. Template usage simplified in `HashMap`
   - Removed member-template hash overloads (`hashKeyValue<T>(...)`)
   - Kept a single class-template-aware helper: `hashKeyValue(const K&)`
   - Goal: reduce template complexity for older compilers
8. `try/catch` removed from binary model deserialization path
   - Replaced exception wrapper with explicit checks + `goto fail` cleanup flow
   - Goal: keep deterministic C++98-compatible control flow
9. Remove all uses of namespaces. Those were not still invented in early 1990s.
10. Variadic `java::String::format(...)` -> `LoggerFormat::format(...)`
   - The VITRAL turboc `String` only offers `formatCStringToJavaString(format, va_list)`;
     `vsdk/toolkit/common/logging/LoggerFormat` wraps it for `Logger::reportMessage` messages
11. `explicit operator ColorRgb()` in `ColorRgbMutable` -> implicit conversion operator
   - C++98 has no explicit conversion operators
12. RenderPark's ray class `Ray` -> `RpkRay` (`common/linealAlgebra/RpkRay.h`)
   - Without namespaces, VITRAL's `Ray` (`environment/geometry/element/Ray.h`,
     used by the spatial indexes) and RenderPark's ray need distinct names, as in the cpp port
13. Spatial index support code of the cpp port in the C++98 subset
   - `BinnedSahSpatialIndex`: VITRAL's turboc `BinnedSahHierarchy` has no `traverse`
     (the cpp port's stopping traversal), so the index runs that loop itself over the
     hierarchy's public nodes, primitive order and `RaySlab`
14. Ray capture and replay (`WITH_RAY_CAPTURE`, `WITH_RAY_REPLAY`,
    `testsuite/ApplicationCases/RayCorpusReplay`, `scripts/captureRays.sh`,
    `scripts/replayRays.sh`)
   - The `.vqc` corpus is the cpp port's format, written and read field by field as
     32-bit little-endian values instead of host-order `fwrite`/`fread` of `int` (16 bits
     on Turbo C++ 3.0), so a corpus of either port replays on the other
   - The cpp port's `long long` counters (and query numbers, limits and strides of the
     replay) are `double` values holding whole numbers: exact up to 2^53, printed with
     `%.0f` as the same digits as `%lld`. Turbo C++ 3.0 has no 64-bit integer and C++98
     has no `long long`
   - Corpus files are only read and written sequentially: no `fseek`/`ftell`, so no file
     offset is held in a `long`; the reader skips geometry lists by reading them. The
     CMake build compiles the recorder and the replay with `_FILE_OFFSET_BITS=64`, so the
     32-bit build opens files beyond 2 GB (a FAT16 DOS volume caps a file at 2 GB)
   - No threads: the replay's worker pass runs its workers one after another, each over
     its own interleaved share of the queries with its own workspaces; timings come from
     `System::currentTimeMillis`, and a chunk holds 512 queries under Turbo C++
     (64 KB allocations) instead of 2^20
   - `captureRays.sh` writes the accounting files to `turboc/doc/rayCorpus`, not to the
     cpp port's top-level `doc/rayCorpus`
15. Galerkin refinement stays serial: no parallel Jacobi
   - The cpp port's `-gr-iteration-method parallel-jacobi` (`PARALLEL_JACOBI`) and
     `-gr-threads` are not offered: Turbo C++ 3.0 under DOS has no threads
   - The serial structure of the cpp port is kept: light transport split into
     `receiverLightTransport` and `sourceLightTransport`, the four subdivisions calling
     `refineSubInteraction`, `RefinementWorkspace` passed down the refinement,
     `FormFactorWorkspace` telling where shadow rays are counted,
     `FormFactorStrategy::reportNodesTooClose`, `Interaction::countCreated` and
     `GalerkinState::isJacobi` (true for `JACOBI` only)
   - `RefinementWorkspace` keeps only the query, form factor and coefficient scratch.
     The parts used only by parallel Jacobi workers are left out, being null for the
     cpp serial solver: the record of events committed later in the serial order
     (`RefinementRecord`, `RefinementEvent`), the subdivision lock and the task
     scheduler (`RefinementTask`, `RefinementScheduler`), and with them
     `regularSubDivide`, `refineOrForkSubInteraction` and the store and light
     transport branches taken when a record is present,
     `Interaction::uncountedDuplicate`, the deferred "nodes too close" warnings of
     `FormFactorWorkspace` and the parallel refinement and commit functions

## VITRAL turboc shared subset

`base/src/main` follows the layout of VITRAL's own turboc port
(`base/src/main/java` and `base/src/main/vsdk/toolkit`). The files listed in
`scripts/vitralSubset.txt` are kept byte-identical to VITRAL's
`turboc/base/src/main` (pin and exceptions in that list):
`scripts/vitralSubset.sh --check|--refresh <VITRAL checkout>` compares or copies
them. The one exception is `vsdk/toolkit/io/wrapper/FileUncompressWrapper.cpp`,
which upstream still calls the removed `Logger::error`. The spatial index files
(`environment/geometry/spatialIndex`) were translated from the cpp port's VITRAL
subset, since VITRAL's turboc port does not carry them yet.
`scripts/vitralClosure.py <VITRAL checkout> [seeds]` lists the closure of VITRAL
headers a slice needs and the class names it would collide with, and
`scripts/classAudit.py` (build target `classAudit`, known debt in
`scripts/classAudit.allow`) fails the build on a class or include guard name
defined twice in the link closure.

## Golden image test and the 4 GB limit

This port is built as 32-bit code (`-m32`), as Turbo C targets do, so a process
can address at most 4 GB. Scene `08_soda` needs more than that for its
radiosity hierarchy: the render still finishes and writes an image, but the
solution is not the one the 64-bit ports converge to, so it does not match
`doc/testBaseImages/08_soda.ppm`. For that reason `scripts/testReviewResults.sh`
leaves `08_soda` out of the golden image test. `scripts/08_runSoda.sh` is kept
so the scene can still be rendered by hand.
