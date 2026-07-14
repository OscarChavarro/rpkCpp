# VITRAL / rpkCpp parallel learning experience

## Statement of intent

This document is a **declaration of intent**, not a work plan with dates or
phases. It records why these source trees are being developed side by side,
what has already converged between them, and the direction future changes
should take to keep reconciling them. Use it as a compass for individual
changes (commits, small refactors), not as a checklist to execute in order.

It is the rpkCpp analogue of
`povCpp/doc/vitralPovrayParallelLearningExperience.md`, and it deliberately
inherits decisions already settled by that sibling effort instead of
re-opening them.

## Three trees, one direction

Three source trees are being developed in parallel:

1. [VITRAL](https://github.com/OscarChavarro/vitral) — the target
   architecture: a general-purpose toolkit (`vsdk`) meant to centralize
   rendering primitives across several client applications and language ports
   (C++ and Java).
2. `povCpp` — a faithful C++ adaptation of POV-Ray 1.0 (1991/1992), a
   recursive raytracer over analytic solids with CSG. Its convergence effort
   is documented in its own `doc/` folder.
3. `rpkCpp` (this repository) — a faithful C++ modernization of RenderPark
   (2001): a Galerkin/hierarchical radiosity engine plus a family of Monte
   Carlo raytracers over patch-based scenes, whose design objectives are
   recorded in `doc/designObjectives.md`.

All trees are studied continuously, and whenever a concept is understood well
enough in one of them, it feeds the others:

- What is learned studying VITRAL improves rpkCpp.
- What is learned studying rpkCpp improves VITRAL.
- Decisions already negotiated between povCpp and VITRAL (bounding contract,
  statistics direction, `RendererConfiguration` derivation, the
  `CameraSnapshot` boundary) are adopted here rather than re-decided — three
  trees renegotiating every contract pairwise would never converge.

Once a concept is understood completely and at good quality, it is meant to
be integrated into VITRAL — the architecture that centralizes shared
components — and that same integrated version is mirrored back into the
client trees. The end state is rpkCpp shrinking down to an application (the
RenderPark scene pipeline, the Galerkin driver, the raytracer drivers) that
depends on VITRAL as a library, with no duplicated implementation of shared
primitives left in this repository.

What rpkCpp uniquely brings to that partnership — capabilities VITRAL does
not have today — is the **Galerkin radiosity engine**, the **Monte Carlo
raytracer family** (stochastic random walk, bidirectional path tracing,
photon map, ray matting), the **tone-mapping operators** and the
**quasi-Monte-Carlo sequences**. The analogue of what povCpp brings (CSG,
all-crossings traversal, nested media).

## The four-port parity constraint

rpkCpp has a property neither VITRAL nor povCpp has: the same program exists
as **four ports kept in verified 1:1 parity** — `cpp/` (the reference),
`java/` (Java 17), `typescript/` and `turboc/` (Borland Turbo C 3.0 / DOS).
Parity is checked by rendering the golden scene corpus and comparing images
across ports.

This constrains how convergence work is done:

- Every convergence change lands in the **C++ port first**, gated on
  unchanged golden images (and on the timing baselines in
  `doc/performanceImprovementPlan.md` when hot paths are touched).
- Once stable, the same change is migrated 1:1 to `java/`, `typescript/` and
  `turboc/` **before the next convergence step begins**, so the ports never
  drift structurally. (Earlier plans such as
  `doc/patchBoundingBoxDecoupling.md` were written as C++-only; under this
  intent, structural changes are now expected to propagate.)
- The Turbo C port adds a mechanical constraint the other trees never face:
  8.3 filenames via `turboc/mappings.csv`. That port keeps its own flat
  directory layout, so it is unaffected by structural changes in the C++
  port's `base/`; any change that does touch turboc's own file names must
  still update that mapping.
- Language-specific idioms stay out of the shared design: this is the same
  force that already keeps the C++ port free of STL, iterators and operator
  overloading, and it is why VITRAL's encapsulation style (private
  attributes, getters, constructors, plain methods) ports so well — it is
  expressible identically in all four languages.

## The `base/` folder

Each port's `base/` holds the `vsdk` toolkit that the RenderPark application
is built on. The intent — mirroring povCpp's `base/` — is that this layer
converges with VITRAL's `vsdk` until it *is* VITRAL, kept in manual sync with
the upstream repository.

The C++ port's `base/` already follows VITRAL's directory and namespace
layout — `base/src/main/vsdk/toolkit/...`, with the `java::` JDK mirror kept
as a sibling root (`base/src/main/java/`) rather than nested inside the
toolkit — matching the convention the Java and TypeScript ports of rpkCpp
already use. File-level diffs between rpkCpp classes and VITRAL classes are
therefore directly comparable, which is the working mode the povCpp effort
already enjoys.

Layers conceptually shared already (same role, divergent shape, see
`doc/vitralNormalizationAnalysis.md` for the class-by-class detail):

1. The `java::` JDK-mirror layer (`String`, `Math`, `ArrayList`, `HashMap`,
   IO classes) — closest to VITRAL in content, only misplaced.
2. The linear-algebra layer (`common/linealAlgebra`) — same package name,
   colliding class names (`Vector3Dd`, `Matrix4x4d`) with different styles.
3. The nearest-hit intersection model (`Ray`, `RayHit`,
   `discretizationIntersect` ↔ `doIntersectionFirstHit`) — same primitive,
   different signatures; unlike povCpp, no new traversal primitive needs to
   be invented for VITRAL on rpkCpp's behalf.

## Next phase: encapsulation, then naming convergence

The next unification targets, in order:

1. **Retire static mutable state** (`Patch::excludedPatches`,
   `Geometry::excludedGeometry1/2`, ID counters) into per-render context —
   both a VITRAL-compatibility and a parallel-readiness requirement.
2. **Encapsulation migration**: convert the public-attribute core model
   (`Vector3D`, `Vector3Dd`, `Ray`, `Camera`, `Vertex`, `Element`, `Patch`,
   `Geometry`) to VITRAL's style — private attributes, getters, full-state
   constructors — **without giving up mutability**: hot loops keep their
   in-place mutators as inline members, so the abstraction costs nothing.
   The repository already contains both styles side by side (`Material`,
   `RayHit`, `Scene`, `RendererConfiguration` are private;
   the geometry/algebra core is public), so this migration makes the tree
   self-consistent, not just VITRAL-consistent. Innermost types first,
   one class at a time, each step measured.
3. **Naming and signature convergence** with VITRAL on the shared primitives
   (vector naming scheme, ray `origin`, detail flags, the nearest-hit
   signature) — negotiated once, renamed once, in both trees.

`doc/vitralNormalizationAnalysis.md` section 14 ("Required alignment work")
is the authoritative, itemized and ordered list; this document only
summarizes the direction.

The Galerkin engine, the raytracer family, tone mapping, the MGF reader and
the render/`sgl` layer are considered rpkCpp application-specific for now and
are **not** planned to move to VITRAL until the primitives they stand on have
converged — moving them earlier would drag divergent types upstream.

## Worked example: the intersection contract

The statistics model was povCpp's worked example; rpkCpp's clearest one is
the nearest-hit contract, because here the two trees *almost* agree already:

- **VITRAL**: `bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)`,
  double precision, one `requiredDetailMask` that both requests and reports
  detail.
- **rpkCpp**: `RayHit* discretizationIntersect(Ray* ray, float minimumDistance,
  float* maximumDistance, int hitFlags, RayHit* hitStore)`, float distances,
  a request mask (`hitFlags`) separate from the filled-fields mask
  (`RayHit::flags`), a caller-owned hit store that doubles as the
  closest-so-far accumulator across lists and the voxel grid, and an
  in-place shrinking `maximumDistance` that gives shadow rays a natural
  any-hit early out.

Neither is a subset of the other: VITRAL's is simpler and
allocation-conscious; rpkCpp's expresses interval-capped queries, any-hit
shadow tests and request-vs-filled detail tracking that VITRAL cannot state
today. The intended resolution shape — the same three-step template povCpp
recorded for statistics — is:

1. Agree the shared contract as a superset (interval + flags + caller store
   folded into VITRAL's signature family).
2. Decide the precision boundary once (float storage in patch data, double in
   shared primitives), and make it the shared answer.
3. Keep whatever is already common (the nearest-hit semantics itself, the
   lazy-detail idea) untouched while the signatures converge.

## Divergences discovered along the way

- **Patch-based vs analytic geometry.** RenderPark bakes MGF scenes into
  world-space patches at load time: no runtime transforms, no analytic
  solids, no CSG. VITRAL and povCpp own placement at scene-body level. This
  is an intentional, domain-driven divergence — radiosity needs finite
  elements, not implicit surfaces — and it means rpkCpp converges with VITRAL
  on *primitives* (vectors, rays, hits, bounding volumes) while contributing
  a different *geometry family* (patch sets, meshes, clusters) than povCpp
  does.
- **Float radiosity core.** rpkCpp's patch pipeline is deliberately
  single-precision for memory/bandwidth; VITRAL's shared primitives are
  double. The precision boundary is a real design decision to make jointly,
  not an accident to normalize away.
- **Radiance-method hooks stamped on structure.** `Geometry::radianceData`,
  `Patch::radianceData`, `Vertex::radianceData` let the active radiance
  method annotate the scene in O(1) — a pattern VITRAL has no counterpart
  for, and a per-algorithm-coupling concern already flagged by
  `doc/patchBoundingBoxDecoupling.md`. Whether this becomes a shared
  "algorithm attachment" concept in VITRAL or stays application-side is an
  open question for the geometry-convergence phase.
- **Acceleration structures.** rpkCpp ships a voxel grid and a patch-cluster
  octree; povCpp recorded that neither VITRAL nor POV-Ray has BVHs. Scene
  acceleration remains future shared work, independent of this
  reconciliation.
- **The DOS port as a design regulator.** Keeping Turbo C 3.0 viable forbids
  header-only template growth, long filenames without mapping updates and
  memory-hungry designs (the `08_soda` scene already exceeds what a 32-bit
  build can address). This constraint has repeatedly pushed the design toward
  the same simplicity VITRAL's multi-language parity requires — the two
  forces agree.
