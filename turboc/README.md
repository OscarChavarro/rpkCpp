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
   - It is VITRAL's turboc port's (`base/`): `SpatialQueryMode` is a plain enum nested
     in a class (`SpatialQueryMode::ANY_HIT`, of type `SpatialQueryMode::Value`), and
     `GridSpatialIndex::queryWith` is `query` (no member templates)
14. Ray capture (`WITH_RAY_CAPTURE`, `scripts/captureRays.sh`)
   - The `.vqc` corpus is the cpp port's format, written field by field as
     32-bit little-endian values instead of host-order `fwrite` of `int` (16 bits
     on Turbo C++ 3.0), so both ports write the same file
   - The cpp port's `long long` counters are `double` values holding whole numbers:
     exact up to 2^53, printed with `%.0f` as the same digits as `%lld`. Turbo C++ 3.0
     has no 64-bit integer and C++98 has no `long long`
   - Corpus files are only written sequentially: no `fseek`/`ftell`, so no file
     offset is held in a `long`. The CMake build compiles the recorder with
     `_FILE_OFFSET_BITS=64`, so the 32-bit build writes files beyond 2 GB (a FAT16
     DOS volume caps a file at 2 GB)
   - `captureRays.sh` writes the accounting files to `turboc/doc/rayCorpus`, not to the
     cpp port's top-level `doc/rayCorpus`
   - The ray corpus replay that consumed the corpora
     (`testsuite/ApplicationCases/RayCorpusReplay`, `scripts/replayRays.sh`,
     `WITH_RAY_REPLAY`, `SceneConsumer`) was removed, as in the cpp port
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

16. Lineal algebra from VITRAL's turboc port
   - RenderPark's mutable `Vector2D`, `Vector3D`, `Vector4D` and `Matrix4x4` were removed,
     as in the cpp port: the port uses VITRAL's immutable `Vector2Df`, `Vector3Df`,
     `Vector4Df`, `Matrix4x4f` and `Matrix2x2`, `Numeric::EPSILON` and
     `Numeric::HUGE_FLOAT_VALUE` became `VSDK::EPSILON` and `Float::MAX_VALUE`, and
     `Camera::lookAtMatrix`/`perspectiveMatrix`, `Numeric::vectorsEqual` and
     `VrmlWriter::recoverRotationParameters` hold what RenderPark keeps of its own
   - Kept, unlike the cpp port, because this port has not taken the steps that retired
     them there: `Patch` keeps its own face data (`normal`, `midPoint`, the `Jacobian`
     class) since it does not hold a VITRAL `PlanarFace` yet, and `MinMaxBox` stays
     for the legacy voxel grid (`scene/VoxelGrid`), which this port still offers

## Library layout

The build (`CMakeLists.txt` of this folder; `Makefile` compiles the same three
source trees into one program) stacks three targets:

| Folder | Target | Contents |
| --- | --- | --- |
| `base/` | `libvitral-turboc.so` | Only the VITRAL files RenderPark takes, byte-identical to `turboc/base/src/main` of a VITRAL checkout (list in `scripts/vitralSubset.txt`) |
| `rpkBase/` | `librpkBase-turboc.so` | RenderPark's own classes: Galerkin, stochastic relaxation and random walk radiosity, raytracers and raycasters, scene model, MGF reader, tone mapping, support classes |
| `testsuite/ApplicationCases/RenderparkApplication/` | `rpk` | The program: command line, options and the rendering driver |

`rpkBase` keeps the `vsdk/toolkit/...` include paths its files had inside `base/`.
This port has no OpenGL viewer, so there is no `rpkOpenGL1/` (the cpp port's third
library).

## VITRAL turboc shared subset

`base/src/main` holds exactly the files listed in `scripts/vitralSubset.txt`
(pin in that list), byte-identical to VITRAL's own turboc port
(`turboc/base/src/main` of a VITRAL checkout):
`scripts/vitralSubset.sh --check <VITRAL checkout>` compares them byte for byte
and fails on any file of `base/src/main` that is not in the list, and `--refresh`
copies them. A VITRAL file changes in VITRAL first and comes in with `--refresh`.
The spatial index files (`environment/geometry/spatialIndex`) and
`io/wrapper/FileUncompressWrapper.cpp`, which this port used to keep as its own
translations, now come from VITRAL's turboc port like the rest.
`scripts/vitralClosure.py <VITRAL checkout> [seeds]` lists the closure of VITRAL
headers a slice needs and the class names it would collide with (in `base/` or
`rpkBase/`), and `scripts/classAudit.py` (build target `classAudit`, known debt in
`scripts/classAudit.allow`) fails the build on a class or include guard name
defined twice in the link closure.

## Golden image test and the 4 GB limit

This port is built as 32-bit code (`-m32`), as Turbo C targets do, so a process
can address at most 4 GB. Scene `08_soda` needs more than that for its
radiosity hierarchy: the render still finishes and writes an image, but the
solution is not the one the 64-bit ports converge to, so it does not match
the golden `cpp/etc/goldenImagesForTests/08_soda.ppm`. For that reason `scripts/testReviewResults.sh`
leaves `08_soda` out of the golden image test. `scripts/08_runSoda.sh` is kept
so the scene can still be rendered by hand.
