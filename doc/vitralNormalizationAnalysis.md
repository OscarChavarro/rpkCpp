# Comparative analysis of the core model: rpkCpp and VITRAL

This comparison uses RenderPark C++ at `c8bbe766` and VITRAL C++ at
`ec5e54aa`. RenderPark's `cpp/base/` is a source subset of VITRAL's
`cpp/base/`. Its engine and viewer live in `cpp/rpkBase/` and
`cpp/rpkOpenGL1/`. The subset check found 130 byte-identical files.
The [alignment plan](vitralAlignmentPlan.md) governs the remaining work
and its verification.

## 1. Linear algebra

RenderPark uses the shared `Vector2Dd`, `Vector3Dd`, `Vector4Dd`,
`Matrix4x4d` and `Matrix2x2`, all in double precision. `PlanarFace` computes
its geometry in double, as the spatial indices traverse double-precision
bounds, so faces prepared from VITRAL meshes need no precision boundary. The
float variants (`Vector3Df` and the others) remain in VITRAL for other
clients.

## 2. Ray model

RenderPark uses VITRAL's `Ray` under `environment/geometry/element/`
(its former `RpkRay` was removed). `Ray` encapsulates double-vector origin
and direction and a ray parameter `t`; RenderPark builds it with
`fromAffine` or the setters, which keep the direction as given. Distances
travel separately in its query contract. A radiosity visibility interface needs
an explicit distance interval and documented direction and distance
conventions. Existing VITRAL raytracer entry points can keep their API.

## 3. Hit record

`RpkRayHit` is a reusable, encapsulated hit record with a borrowed
`const Patch *`, a shading frame and flags that track which results are
valid. Updating UV invalidates the derived texture coordinate, normal
and frame. VITRAL's `RayHit` uses surface detail flags and can carry the
information used by the VITRAL raytracer, including its material.
Galerkin shadow rays primarily need an occlusion answer and, for blocker
reuse, the face identity and distance. A face hit contract can keep
those fields independent of either renderer's full shading record.

## 4. Intersection operation model

`SceneQuery` selects `BinnedSahSpatialIndex`, `GridSpatialIndex` or
`LinearSpatialIndex` over the scene's face table. Its const query takes
a distance interval, exclusions, hit flags, a reusable hit record and
a caller-owned workspace. `Patch::intersect` performs the exact test
using `PlanarFace`; an any-hit query may return different blockers at
the same visibility result. Closest-hit ties can also depend on index
traversal order. VITRAL's `Geometry::doIntersectionFirstHit` is the
closest-surface contract used by its raytracing path. The radiosity
module can reuse the spatial index contract and define its face query
with the policies it needs.

## 5. Geometry model

`Patch` owns an immutable VITRAL `PlanarFace` and binds it to vertices,
material, an optional twin and two identifiers. The scene-local
`getFaceIndex()` addresses the face table. `getId()` serves legacy ID
output and serialization. Bounds, direct potential, view visibility,
display color and top-level radiance-element bindings now live outside
`Patch`, in scene-owned tables.

VITRAL's `Geometry` represents intersectable shapes, including meshes and
analytic surfaces. RenderPark's `RpkGeometry`, `MeshSurface`, `Compound`
and `PatchSet` organize patches, including for clustering and shaft
culling. The radiosity input can expose a prepared face table to the
solver. It can keep clusters and shaft candidates in solver state and
scratch while leaving mesh and placement with VITRAL.

## 6. Scene model and transforms

RenderPark's MGF loader produces world-space geometry. `Scene` owns the
flat patch list and cluster hierarchy and builds a dense face table and
`SceneQuery`. VITRAL's `SimpleScene` organizes bodies. A
`SimpleSceneSnapshot` exposes bodies, lights, camera and background, and
its body collection contains pointers to `SimpleBody`. The adapter
should copy the planar face positions after applying body placement and
establish read-only material bindings for the solve. The snapshot alone
provides no guarantee that the bodies stay immutable.

## 7. Camera

RenderPark's `Camera` supplies float projection and view matrices, frustum
planes and the eye frame used by importance, face ID rendering and the
viewer. VITRAL's `CameraSnapshot` is the view input for raytracing.
Use a snapshot at the VITRAL display boundary. The base radiosity
solution can be evaluated from several views, and direct potential and
view visibility can be recomputed for each importance pass.

## 8. Material model

VITRAL's `PhongMaterial`, `PhongLobe` and `ScatteringFrame` supply the
integrated reflectance and emission used by Galerkin and the directional
operations used by the RPK ray methods. `RpkMaterial` owns the shared
material and presents EDF and BSDF forwarding views. VITRAL's existing
shaders also use `SimpleMaterial` and `MicroFacetedMaterial`. The scene
adapter needs an explicit radiometric binding or conversion for scenes
authored with the viewer's materials. In particular it must specify
diffuse reflectance, emissive surfaces and sidedness.

## 9. Renderer configuration

RenderPark's `RendererConfiguration` carries display and debug options
such as outline and cluster display, smoothing and culling.
VITRAL's class of the same name defines shading modes and viewer
options. Pass the options each solve or render actually consumes at the
boundary. When a VITRAL header enters the RenderPark build, check for
colliding class names and include guards before combining them.

## 10. Bounding information

RenderPark fills `FaceBoundsTable` from planar faces and keeps
`RpkAxisAlignedBoundingBox` for geometry and clusters. VITRAL already
provides `GeometryBounds` for geometry-local bounds and a separate
`SpatialIndex` family for traversal. Reuse the shared index. Check the
padding used by each backend against the face intersection tolerance
when building the new prepared scene.

## 11. Statistics

RenderPark divides counters into reader, radiance, ray tracer, shadow and
potential groups. Its `Statistics::instance()` still exposes global
state. VITRAL's `RaytraceStatistics` exposes a static recording API.
The extracted solver needs per-solve counters, with per-worker collection
and aggregation where needed. Keep the iteration counts and ray
fingerprints used to compare against RenderPark.

## 12. Remaining subsystem boundaries

The Galerkin solver, its clustering and refinement, the SGL 3D scan
conversion, the MGF loader, tone mapping and the Niederreiter sequences
live in RenderPark. VITRAL already has its own raytracer and a
`Rasterizer2D` that draws and fills polygons into an `Image`. The
radiosity milestone needs a VITRAL input adapter, a reusable solve and a
way to display the result. Importers, other RPK radiance methods and
GPU execution can evolve separately.

## 13. SGL and typed raster outputs

### Current data flow

`SglContext` owns three separate arrays with the same pixel coordinates
and an optional depth array. It allocates all three outputs and clears
all three over the current viewport. `sglSetPatch` and
`sglSetGalerkinElement` change the current value and the output
mode for the context. `Poly::scanlineFlat` and `Poly::scanlineZ` check the
mode inside the pixel loop. In the depth path, a covered sample wins on
`z <= storedDepth`, so the last submitted polygon wins an equal-depth tie.

| Mode | Value per sample | Current C++ consumer |
| --- | --- | --- |
| `PIXEL` | Packed `SGL_PIXEL` value | The constructor defaults to this mode. The code search found no packed-color draw caller or reader of `frameBuffer`. `SoftIds::softRenderPixels` uses its own RGBA array. |
| `PATCH_POINTER` | One borrowed `Patch *`, or null for background | `SoftIdsWrapper` looks up a visible patch for ray casting and image export. `Potential` uses the buffer for direct visibility and an exported patch ID array for direct importance. |
| `ELEMENT_POINTER` | One borrowed `Element *`, or null for background | The Galerkin scratch pass projects a cluster, reads source radiance, counts covered samples and updates per-element coverage counters. All entries it submits are `GalerkinElement` objects. |

The patch list supplies polygons in draw order. Each output cell records
only the winning polygon's patch or element. The scan loops cast away
constness to store the pointers, and the element readers downcast the
generic `Element *`. During importance calculation a patch pointer
becomes a legacy ID, is masked to 24 bits and is used with a table
sized by the largest patch ID. The dense face table gives that pass a
scene-local address already. A migration should check any changes in
legacy ID semantics and statistics summation order separately.

### Design options

| Design | Benefit | Cost or constraint |
| --- | --- | --- |
| Concrete typed targets with a shared rasterizer | A face pass exposes face indices and an element pass exposes element references or indices. Each target owns one payload plane. Invalid combinations fail at the API boundary. | Keep transforms, clipping, depth comparison and scan conversion in shared code. Use typed entry points with a common kernel, and keep any dispatch outside the per-pixel loop. |
| A generic primitive ID target | SGL writes a compact integer. Pass-owned lookup tables resolve it to a face or element. The raster core stays independent of solver classes and the IDs can also be used by an integer GPU attachment. | Raw integers allow face and element indices to mix. Expose distinct typed maps or index wrappers at the consumer boundary and define the lookup lifetime and background value. |
| A rasterizer that emits visible spans or fragments to a sink | The rasterizer stays independent of the storage format. Color, IDs and specialized consumers supply the storage operation. | Dispatch once per span or use a statically typed sink. Consumers that need the final winner still need storage or must handle replacement when depth changes. |
| A target tagged once per pass | A context can choose one concrete target dynamically and allocate only that target. | Runtime type checking remains at the boundary. A tag per cell or multiple simultaneous payload planes add storage and allow combinations that neither current visibility pass needs. |

### Proposed first change

Give the two active passes concrete APIs, for example
`FaceVisibilityBuffer` and `ScratchVisibilityBuffer`, using
one rasterizer and a common depth component. Submit the polygon and its
typed value together. Each pass selects, allocates and clears its own target.
The target dimensions and viewport are fixed or validated at the draw
boundary. Use a distinct color target only if a real color consumer
requires one.

- **Face pass.** Use a typed index into the prepared scene's dense face
  table, with an explicit background sentinel such as `-1`. Face zero is
  valid. Treat the map as valid only while that table remains in use.
  Resolve to `const Patch *` at the RPK boundary where an existing caller
  needs a patch. Keep the legacy ID export as a conversion while migrating
  its consumers. `FaceVisibilityTable` remains the per-face summary of
  the per-sample result.
- **Element pass.** Use a pass-scoped `const GalerkinElement *` initially,
  with null for background. Keep coverage accumulation in solver state,
  with explicit mutable access there. A solver-owned element index can
  replace the reference when its stability through refinement is defined.
  Clear or invalidate the scratch result when its element hierarchy or
  projection changes. Move the mutable cached bounding box in
  `ScratchVisibilityStrategy` into solve-owned scratch when making this
  path reentrant.
- **Rasterizer.** Preserve the shared clipping, viewport mapping, scan
  conversion and depth quantization. Reuse a depth plane within a pass
  with a clear depth convention. VITRAL's `ZBuffer` stores float depth and
  `Rasterizer2D` uses `Image` and color fills. Any reuse needs an explicit
  depth and coordinate compatibility check against SGL's integer
  comparisons, depth stepping and ties.

## 14. Remaining alignment work

The [alignment plan](vitralAlignmentPlan.md) sequences the typed visibility
outputs, the VITRAL mesh and material adapter, extraction of a Galerkin
solver with instance-owned state, and a VITRAL offline example. The main
solver work is the removal of dependencies on application ownership,
including static `GalerkinState`, ID counters, shaft pools and scratch
caches. The current CPU Jacobi implementations provide the serial and
parallel reference for that extraction.
