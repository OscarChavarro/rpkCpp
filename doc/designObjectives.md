# Design Objectives

A faithful C++ modernization of RenderPark (KU Leuven, 2001), a physically-based
rendering system that combines a **Galerkin / hierarchical radiosity engine**
(view-independent solution over patch subdivisions) with a **family of
raytracers** (ray casting, ray matting, stochastic raytracing / random walk,
bidirectional path tracing, photon map). This document reviews the current
state of the code under `cpp/` against the project's stated objectives.

The transversal direction that binds everything — convergence with the
[VITRAL library](https://github.com/OscarChavarro/vitral) — is documented
separately in `doc/vitralRenderparkParallelLearningExperience.md` and
`doc/vitralNormalizationAnalysis.md`.

## Project objectives

1. **Academic study base** — serve as a clear, well-organized reference for
   learning global illumination: radiosity (Galerkin, hierarchical refinement,
   clustering, shaft culling) and Monte Carlo raytracing, with key equations in
   code annotated against the original papers archived in `doc/references/`.
2. **C++11 alignment** — deliberately stay on C++11 (no newer standards), with
   no STL containers, iterators, operator overloading or preprocessor macro
   tricks, to keep the implementation easy to migrate to other languages and
   old toolchains.
3. **Multi-port 1:1 parity** — maintain four ports of the same program:
   - `cpp/` — the C++11 reference port (this is where changes land first).
   - `java/` — Java 17 / Gradle port (optional JOGL visualization).
   - `typescript/` — Node/TypeScript port.
   - `turboc/` — Borland Turbo C 3.0 (DOS) port, with an 8.3-filename mapping
     workflow (`turboc/mappings.csv`).
   Parity is verified by comparing rendered output images across ports against
   the golden references in `doc/testBaseImages` (see also
   `typescript/parity-report.json` and `cpp/scripts/delta/`).
4. **Parallel/concurrent readiness** — keep the pure (no external dependency)
   CPU engine compatible with serial and future concurrent multi-thread
   implementations: re-entrant code, no global variables, mutable state owned
   by instances.

## Observed structure

- **`cpp/base/src/main/vsdk/`** (~62k LOC, 333 class headers): the whole engine
  as a `vsdk` toolkit, organized in packages:
  - `toolkit/common` — color, data structures, linear algebra
    (`Vector3D`/`Vector3Dd`, `Matrix4x4`/`Matrix4x4d`, `Numeric`,
    `CoordinateSystem`, `Jacobian`), logging, memory management, per-domain
    statistics, command-line options.
  - `toolkit/environment/geometry/elements` — the patch-based geometric model:
    `Vertex`, `Patch`, `PatchSet`, `Element` (radiance data hook), `RayHit`.
  - `toolkit/skin` — the geometry aggregation layer: `Geometry`, `Compound`,
    `MeshSurface`, `AxisAlignedBoundingBox`, `MinMaxBox`.
  - `toolkit/scene` — `Scene`, `Camera`, `Background`, `VoxelGrid` (regular
    grid ray acceleration), `PatchClusterOctreeNode`, `RadianceMethod`.
  - `toolkit/material` — physically-based Phong EDF/BRDF/BTDF/BSDF model,
    `Material`, `Texture`, `RefractionIndex`, `RendererConfiguration`.
  - `toolkit/galerkin` — the Galerkin radiosity engine (`GalerkinRadianceMethod`,
    `GalerkinElement`, `GalerkinBasis`, `Interaction`, `Shaft` culling,
    clustering strategies, Jacobi/Gauss-Seidel/Southwell iteration).
  - `toolkit/raycasting` — the raytracer family: `simple` (RayCaster,
    RayMatter), `stochasticRaytracing`, `bidirectionalRaytracing`, `photonMap`,
    plus shared `common` machinery.
  - `toolkit/numericalAnalysis` — quasi-Monte-Carlo (Niederreiter 31/63-bit).
  - `toolkit/io` — MGF scene reader, image writers, VRML export, binary
    readers/writers.
  - `toolkit/render` — `ScreenBuffer`, `Canvas`, software rasterizer (`sgl`),
    optional OpenGL debug visualization.
  - `toolkit/tonemap` — tone-mapping operators (Ward, TumblinRushmeier and
    revised variant, Ferwerda, Lightness, Identity).
- **`cpp/base/src/main/java/`** — a C++ mirror of the JDK (`java::String`,
  `java::Math`, `java::ArrayList`, `java::HashMap`, `java::io`-style classes),
  kept as a sibling of `vsdk/` (matching VITRAL and the Java/TypeScript ports),
  that makes the Java port nearly mechanical.
- **`cpp/testsuite/ApplicationCases/RenderparkApplication/`**: the application
  shell (`Main`, `RpkApplication`, `SceneBuilder`, `Radiance`, `Raytrace`,
  `Batch`) — the engine itself is a library.
- **Golden scene corpus**: `cpp/scripts/01..08_*` (Galerkin: cube with three
  iteration methods, corridor, hospital, office1–3, salón, soda), `10..14_*`
  (raytracers) and `21..22_*` (tone mapping), rendered from the MGF models in
  `etc/` and compared against `doc/testBaseImages`.

## Strengths

| # | Aspect | Evidence | Why it matters (objective) |
|---|--------|----------|----------------------------|
| P1 | **Package-per-concern architecture**, one class or enum per `.h`/`.cpp` module | `cpp/README.md` checklist; `base/src/main/vsdk/toolkit/*` layout | #1 didactic; #3 file-per-class maps 1:1 to Java/TypeScript |
| P2 | **Annotated math against archived references** | `doc/references/`, `doc/annotatedEquationsInCode.png` | #1: the code doubles as a guided reading of the radiosity/MC literature |
| P3 | **JDK mirror already built and consumed** (`java::*`) | `base/src/main/java/**`; Java/TS ports exist | #3: three ports already derived from it |
| P4 | **Golden-image corpus across engines** — Galerkin (8 scenes × iteration methods), 5 raytracer modes, tone-mapping cases | `cpp/scripts/*.sh`, `doc/testBaseImages`, `cpp/scripts/delta/imageDiff` | #3 the parity oracle; #1 reproducible experiments |
| P5 | **Disciplined C++11, no exotic features** — no STL/iterators/operator overloading/`#define` function macros; vanilla printf | `cpp/base/CMakeLists.txt` (`-std=c++11 -Wall -Wextra -pedantic`), `cpp/README.md` | #2 clean migration; #3 the same code shape compiles under Turbo C 3.0 discipline |
| P6 | **Four live ports with parity fixes flowing between them** | `java/`, `typescript/`, `turboc/`; parity bug-fix history (shaft-culling omit bug, energy conservation, BPT RNG parity) | #3 fulfilled as an ongoing discipline, not an aspiration |
| P7 | **Physically-based material model already encapsulated** — `Material`, Phong EDF/BRDF/BTDF/BSDF, `RefractionIndex` with private attributes, constructors and const getters | `toolkit/material/*` | #1 readable; #4 read-mostly materials are thread-shareable |
| P8 | **`RayHit` already encapsulated with a fill-flags contract** — private fields, `init()`, lazily-computed shading frame gated by `RayHitFlag` masks | `elements/RayHit.h` | #4; precedent for the encapsulation migration (M1) |
| P9 | **Per-domain statistics as instance classes** — `Statistics`, `RadianceStatistics`, `RayTracerStatistics`, `ShadowStatistics`, `PotentialStatistics`, `ReaderStatistics` | `toolkit/common/statistics/*` | #4 instance-owned counters aggregate across threads; #1 measurable behavior |
| P10 | **Acceleration structures isolated as classes** — `VoxelGrid` regular grid, patch cluster octree, per-patch bounding boxes | `toolkit/scene/VoxelGrid.*`, `PatchClusterOctreeNode.*` | #1; #4 traversal state is parameterized, not global |
| P11 | **Optional subsystems cleanly severable** — OpenGL behind `OPEN_GL_ENABLED`, raytracers behind `RAYTRACING_ENABLED` | root `README.md`, `RendererConfiguration.h` comment | #2/#3: the Turbo C port trims what DOS cannot host |
| P12 | **Valgrind-verified manual memory management, no smart pointers by design** | `cpp/README.md` ("Memory leak free after valgrind analysis") | #2 raw `new`/`delete` only; #1 explicit, traceable ownership |
| P13 | **Performance work is planned and measured, not ad hoc** | `doc/performanceImprovementPlan.md` (baselines over `02_runCorridor.sh`), `doc/patchBoundingBoxDecoupling.md` | #1/#4: refactors are gated on identical golden output and timed baselines |

## Areas to improve

| # | Aspect | Evidence / risk | Objective | Suggestion |
|---|--------|-----------------|-----------|------------|
| M1 | **Public-attribute data classes across the core model** — `Vector3D`, `Vector3Dd`, `Ray`, `Camera`, `Vertex`, `Element`, `Patch` (partially), `Geometry` expose raw fields; encapsulation is inconsistent with the already-private `Material`/`RayHit`/`RendererConfiguration`/`Scene` | `linealAlgebra/Vector3D.h`, `Ray.h`, `scene/Camera.h`, `elements/Vertex.h`, `elements/Element.h`, `skin/Geometry.h` (`public: // Will become protected`) | #1, #4, VITRAL convergence | Migrate to private attributes with getters and full-state constructors, **keeping the classes mutable** (setters/mutators in the VITRAL style) so hot loops keep in-place updates; gate every step on golden images + the timing baselines of `performanceImprovementPlan.md` |
| M2 | **Residual static mutable state** — `Patch::patchId` counter and `Patch::excludedPatches[]`, `Geometry::nextGeometryId` / `excludedGeometry1` / `excludedGeometry2`, `Vertex::currentComparisonFlags` | `elements/Patch.h`, `skin/Geometry.h`, `elements/Vertex.h` | **#4** | These contradict the re-entrance claim in `cpp/README.md`; move exclusion sets and ID generation into per-render context objects before any threading work |
| M3 | **Name/semantic collisions with VITRAL** — `Geometry`, `Ray`, `RayHit`, `Camera`, `Material`, `RendererConfiguration`, `Vector3Dd` exist in both trees with different contracts | see `doc/vitralNormalizationAnalysis.md` §1–§9 | VITRAL convergence | Reconcile class by class following the analysis document; never let both meanings coexist in one translation unit |
| M4 | **`float` core vs VITRAL's `double` model** — the radiosity pipeline is deliberately `float` (`Vector3D`, `Ray`, patch geometry) for memory/bandwidth; VITRAL's shared primitives are `double` (`Vector3Dd`, `Ray`) | `Vector3D.h`, `Ray.h` vs VITRAL `Vector3Dd.h`, `Ray.h` | #3, VITRAL convergence | Decide the precision strategy per layer before merging classes: keep `float` storage for patch/radiosity data, adopt shared `double` types where VITRAL primitives are taken as-is |
| M5 | **No unit-test scaffolding** — only whole-image golden tests | no unit-test directory in `cpp/` | #1, #3, #4 | Golden images do not localize regressions and give no cross-port contract for `Vector3D*`/`Matrix4x4*`/solvers; unit tests would pin behavior for all four ports |

## Reading by objective

- **#1 Academic base** — Strong (P1, P2, P4, P13): package-per-concern layout,
  literature-annotated equations, and a reproducible scene corpus. The main
  detractors are M1 (an inconsistent encapsulation story confuses the reader
  about which fields are contracts) and M5 (no unit tests documenting
  per-class contracts).
- **#2 C++11 migratable** — Solid (P5, P11, P12). The no-STL/no-exotics
  discipline is what made three additional ports possible, including a DOS
  compiler from 1991.
- **#3 Multi-port parity** — Fulfilled as a discipline (P3, P4, P6) and unique
  to this project: parity is not "a Java port could be built" but "four ports
  are kept image-equivalent". M4 (float/double) and M5 (no shared unit
  contracts) are the standing risks to parity.
- **#4 Parallel/concurrent readiness** — Partially prepared (P7–P10 give
  instance-owned state and read-mostly shared data), but **not yet honest**:
  M2's static mutable members must be eliminated before any pthreads work, and
  the IO package is documented as non-re-entrant.
- **VITRAL convergence (transversal)** — The direction of travel for all of the
  above: M1 adopts VITRAL's encapsulation style, M3/M4 are the class-by-class
  reconciliation detailed in `doc/vitralNormalizationAnalysis.md`. The unique
  assets this project can eventually contribute upstream are the Galerkin
  radiosity engine, the raytracer family and the tone-mapping operators —
  none of which VITRAL has today.
