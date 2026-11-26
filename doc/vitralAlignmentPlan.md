# Aligning RenderPark C++ with VITRAL

RenderPark C++ already runs Galerkin over VITRAL face geometry and material
operations. The next milestone is a Galerkin solver that a VITRAL application
can call on a VITRAL scene. The immediate task is to give the SGL visibility
passes typed outputs. [The normalization analysis](vitralNormalizationAnalysis.md)
describes the current contracts and compares the SGL designs.

## Completed C++ alignment

The former steps 0–7 are available with
`git show c8bbe766:doc/vitralAlignmentPlan.md`. That revision records the
measurements, precision decisions, replay results and accepted image changes.

- The image gates, ray capture, class audit and VITRAL source subset are in
  place. The standalone ray replay used during the migration was retired.
- RPK computes in double precision. It uses VITRAL's double vectors and
  matrices, its `Ray`, `PlanarFace` and the integrated and directional
  operations of `PhongMaterial`, all in double precision. The RPK EDF and BSDF classes
  that remain are forwarding views. Both C++ builds use strict IEEE arithmetic
  with GCC and Clang.
- The scene assigns dense face indices and stores bounds, direct potential,
  visibility, display color and radiance-element bindings outside `Patch`.
- Former step 5 integrated the selectable indices at both entry points.
  RenderPark's `SceneQuery` supports `wald2007`, `voxel` and `bruteforce`.
  The index prunes candidates and `Patch::intersect` decides the exact hit.
  WALD2007 is the default, shaft culling defaults to `never`, and the old C++
  voxel grid has been retired. VITRAL's `RaytracingOfflineExample` can exercise
  the same index choices for triangle meshes.
- `parallel-jacobi` is implemented. The parallel refinement fast path requires
  isotropic clustering, shaft culling disabled, multiresolution visibility
  disabled and no ray recorder. It uses worker-owned scratch and commits in
  serial traversal order. The stochastic raytracer can also parallelize
  its final image pass in the supported Random Walk configuration.

## Ground rules

1. **Measure performance and justify costs.** Compare CPU time, wall time and
   peak RSS on the corridor for solver changes and measure the affected pass for
   rasterizer changes. A slowdown beyond the measured spread requires the
   owner's approval and a demonstrated gain in decoupling, architecture,
   parallel or GPU readiness, or reusable code moved upstream. Reuse
   caller-owned records and workspaces in hot loops.
2. **Encapsulate state.** New and migrated classes use private attributes,
   inline getters and constructors that establish valid state. No `friend`
   classes or functions and no class-level or function-local mutable statics
   in that code.
3. **Prefer const queries.** Make writes to a caller-owned output or workspace
   explicit. Shared query inputs stay read-only during a solve.
4. **Prefer immutable shared objects.** Scene faces, materials and indices
   can be completed by builders before workers start.
5. **Prefer one role per class.** Keep shared input, builders, worker scratch
   and mutable solver state distinct.
6. **Keep the portable subset.** Use C++11 without STL, lambdas or new
   operator overloads. Visitors are classes. Give the shared design direct
   counterparts in Java 17, TypeScript and Turbo C++ 3.0. Record Turbo C++
   8.3 file names in `turboc/mappings.csv`.

### Trees and ports

- Implement the next slice in `cpp/` first. `cpp/base/` contains the
  byte-identical VITRAL subset. `cpp/rpkBase/` contains the RenderPark engine
  and adapters. `cpp/rpkOpenGL1/` holds its viewer and debug tools.
- Change a shared file in VITRAL first, including its build inputs and contract
  tests. Run VITRAL C++ `./scripts/compile.sh`, then refresh and check the
  subset with `cpp/scripts/vitralSubset.sh --refresh` and `--check` against
  the VITRAL checkout. Record the tested VITRAL revision with the references.
  Synchronize each RPK port from its matching VITRAL port.
- Check parity as each new slice propagates. Grid classes already exist in
  all four VITRAL ports. RPK still has port-specific differences, including
  residual legacy acceleration in the other ports and direct potential stored
  on patches in Java and Turbo C++. Track those as port work where they affect
  the slice.

### Verification tiers

| Tier | Check | When |
| --- | --- | --- |
| T0 | Build with and without `RAYTRACING_ENABLED` where supported, with `-Wall -Wextra -pedantic` and the class audit | Each code change |
| T1 | Exact image comparison (AE = 0) against port references (and, outside C++, unchanged statistics fingerprints) | Result-preserving changes |
| T2 | Focused contract tests for the changed interface | When a contract changes |
| T3 | Full corpus within the recorded RMSE bounds against the golden images in `cpp/etc/goldenImagesForTests` | End of a stage |
| T4 | CPU time, wall time and peak RSS against the recorded baseline | End of a stage and changes to hot paths |
| T5 | Concurrent queries or solves under ThreadSanitizer, with results compared to the serial run | When shared state or concurrency changes |

Run the existing corpus and `scripts/testReviewResults.sh` from the port
directory. In C++, T1 and T3 are one check since 2026-11-26: every output must
equal its golden image in `cpp/etc/goldenImagesForTests` (AE = 0, scene 13 with
parallel tracing within its RMSE bound at 50% size); that folder holds only the
images, and T4 compares against the timings of a previous run kept by hand. The
other ports keep `--exact` (with fingerprints) and `--golden`. [The README](../README.md#rebuilding-the-c-golden-images)
gives the capture command. Changing a golden image or its bound requires the
owner's decision. Preserve floating-point precision and operation order in
result-preserving changes. Stop and report if an exact check fails, a legacy
answer disagrees with the reference implementation, a measured slowdown
exceeds the baseline spread, or work requires a new cross-tree rename or a
non-additive change to a shared API.

## 1. Give SGL typed outputs

Use one shared polygon rasterizer and one typed target per pass. Begin with
the visible-face pass (`SoftIds`, `SoftIdsWrapper`, `Potential` and
the image and ray-casting consumers) and the intra-cluster element pass
(`ScratchVisibilityStrategy` and `ScratchRendererVisitor`). Keep the
current depth representation and arithmetic, viewport stride, clipping,
pixel centers, row orientation and last-writer behavior on equal depth.
Check the active callers of the packed-color path before deciding whether
to carry a third target. [The SGL analysis](vitralNormalizationAnalysis.md#13-sgl-and-typed-raster-outputs)
records the proposed types and migration options.

Compare the face map and the final visibility and importance tables on
`15_cubeJacobiImportance` and `17_floorStochasticJacobiImportance`. Also
compare a ray-cast export and exercise the element scratch path directly,
including its covered-pixel count and per-element counts. The default corpus
uses isotropic clustering, so its ordinary image comparisons cannot verify
the `Z_VISIBILITY` branch. Include overlapping polygons with equal depth,
background, a sub-viewport and the flat scan path if retained. Measure
allocation, clearing and rasterization time on the two active passes.

## 2. Adapt a VITRAL scene to radiosity input

Define a read-only set of world-space planar faces and material bindings,
with dense scene-local face indices. Start with `TriangleMesh` and planar
`QuadMesh` geometry from VITRAL bodies. Apply the body placement once and
define the handling of transformed winding, degenerate or nonplanar faces,
and single- or double-sided materials. Specify the conversion to
`PlanarFace`'s float coordinates and the lifetime of the face table.

Reuse `PhongMaterial` for diffuse reflectance and emission. Provide an
explicit material binding or a documented conversion for the viewer's
`SimpleMaterial`. Start with constant untextured diffuse surfaces and
emissive geometry. Build the same prepared input from an existing RPK scene
to compare faces, materials and visibility before moving the solver. Defer
tessellation of analytic geometry and CSG until the mesh adapter works.

## 3. Extract a Galerkin solver with instance-owned state

Keep the prepared faces, materials and acceleration read-only. Give each
solve its own clusters, elements, interactions, coefficients, refinement
state, counters and pools. Use a caller-owned visibility workspace for
queries and separate per-worker form-factor scratch. Reuse VITRAL's spatial
indices with a face test that preserves the interval, sidedness, face
exclusions and any-hit behavior used for shadow rays.

Port the existing hierarchical Galerkin CPU path in slices, starting with
serial Jacobi and isotropic clustering, and keep RPK as a comparison caller.
First compare iteration statistics and radiance coefficients on the
same prepared Cornell box. Then attach the existing parallel Jacobi path
and compare one and many workers. The RPK CLI, MGF loader, OpenGL debug
tools and other radiance methods can remain at their current entry points
while that module takes shape.

## 4. Publish and render the solved radiance

Expose the solution by scene face and surface coordinates so it can be
sampled from multiple cameras. Preserve the Galerkin basis coefficients
through refinement and evaluation. Add an offline VITRAL example that
constructs a Cornell box from VITRAL meshes, solves it and writes an image.
Use `CameraSnapshot` and VITRAL image output at this boundary. Verify a
second camera can display the same solution, then compare the corridor for
cost and the existing image corpus for RPK parity. Add view-dependent
importance and other clustering modes as follow-up slices.
