# Comparative analysis of the core model: rpkCpp <-> VITRAL

Status snapshot comparing the current `cpp/` port of rpkCpp (RenderPark
modernization) with the current
[VITRAL library](https://github.com/OscarChavarro/vitral) (`vitral/cpp`).

The focus is the required alignment surface between both codebases: linear
algebra, rays, hit records, the intersection operation model, geometry and
scene models, cameras, materials, renderer configuration, bounding
information and statistics. It closes with the authoritative, ordered list of
alignment work (§14).

---

Directory and namespace layout are already shared between the two trees:
both keep the toolkit under `base/src/main/vsdk/toolkit/...`, with the
`java::` JDK-mirror layer as a sibling root (`base/src/main/java/{io,lang,util}`),
matching the Java and TypeScript ports of this same project.

## 1. Linear algebra

### 1.1. rpkCpp

`toolkit/common/linealAlgebra/` carries a **two-precision** model:

- `Vector3D` — `float x, y, z`, **public attributes**, a rich mutating API
  (`combine`, `sumScaled`, `crossProduct(a, b)` writing into `this`, etc.).
  This is the workhorse of the whole patch/radiosity pipeline; `float` is a
  deliberate memory/bandwidth choice.
- `Vector3Dd` — `double x, y, z`, public attributes, a minimal API
  (`dotProduct`, `crossProduct`, `normalizeAndGivePreviousNorm`), used where
  double precision is numerically required.
- `Vector2D`, `Vector2Dd`, `Vector4D`, `Matrix2x2`, `Matrix4x4` (float),
  `Matrix4x4d`, `Numeric`, `CoordinateSystem`, `CoordinateAxis`, `Jacobian`.

### 1.2. VITRAL

`common/linealAlgebra/` is **double-first with float twins**: `Vector3Dd` /
`Vector3Df`, `Vector2Dd/f`, `Vector4Dd/f`, `Matrix4x4d/f`, `MatrixNxM`,
`Quaterniond/f`, `Complex`. VITRAL `Vector3Dd` has **private** `x_, y_, z_`,
inline getters, and an immutable-style API returning new values
(`multiply`, `crossProduct(other)`, `midpoint`), plus `equals`/`hashCode`/
`toString` in the JDK idiom.

### 1.3. Alignment status

- The package name is already shared (`common/linealAlgebra`, same spelling).
- The **class-name collision is real**: both trees define `Vector3Dd` and
  `Matrix4x4d` with different contracts (public mutable vs private
  immutable-style).
- rpkCpp's float `Vector3D` corresponds to VITRAL's `Vector3Df` in precision,
  but not in name or API shape.
- VITRAL has no `CoordinateSystem`/`Jacobian`/`Numeric` equivalents; those are
  rpkCpp candidates to contribute upstream, in VITRAL style.

Required alignment item: converge on VITRAL's naming scheme
(`Vector3Df` = float, `Vector3Dd` = double) and encapsulation style, while
preserving rpkCpp's mutating hot-path operations as members of the same
classes (VITRAL classes are already mutable-friendly: private fields do not
force immutability). The float/double split itself is **kept**: patch and
radiosity storage stays `float`; shared VITRAL primitives stay `double`.

## 2. Ray model

### 2.1. rpkCpp

```cpp
class Ray {
  public:
    Vector3D position;
    Vector3D direction; // Direction should be normalized
};
```

Float precision, public attributes, no parametric distance on the ray (the
`t`/distance travels separately as `minimumDistance`/`maximumDistance`
parameters of the intersection calls). Lives in `common/linealAlgebra/`.

### 2.2. VITRAL

```cpp
class Ray {  // environment/geometry/element/Ray.h
private:
    Vector3Dd origin;
    Vector3Dd direction;
    double t;
public:
    Ray(const Vector3Dd& origin, const Vector3Dd& direction, double t);
    Ray withOrigin(...) const; Ray withDirection(...) const; Ray withT(...) const;
    const Vector3Dd& getOrigin() const; ...setters...;
    bool equals(...); int hashCode(); java::String toString();
};
```

Double precision, private attributes, getters/setters plus `with*` builders,
normalization enforced at the boundary. Lives in
`environment/geometry/element/`.

### 2.3. Alignment status

Divergent on: package (linealAlgebra vs environment/geometry/element), field
name (`position` vs `origin`), precision (float vs double), carried state
(`t` on the VITRAL ray vs distance-interval parameters in rpkCpp), and
encapsulation.

Required alignment item: adopt VITRAL's `origin` naming and encapsulation for
the rpkCpp ray, and decide precision at the same time as §1. povCpp resolved
the analogous problem by **extending** VITRAL `Ray`
(`RayWithTracingState : public Ray`) rather than replacing it; the same
pattern fits here if rpkCpp needs to keep a float fast path or per-ray
traversal caches (e.g. `VoxelGrid` reciprocals).

## 3. Hit record

### 3.1. rpkCpp `RayHit`

Already encapsulated (private fields, `init()`, getters/setters):

- `point`, `geometricNormal`, `texCoord`, `uv` (`Vector2Dd`), `patch`
  (hit `Patch*`), `material`, `shadingFrame` (`CoordinateSystem`, Z = shading
  normal), `flags`.
- `flags` (values in `RayHitFlag.h`) records **which fields have been filled
  in**, and the `hitFlags` parameter of the intersection entry points requests
  what must be computed — a request/fill contract.
- Raytracing-only members (`getNormal`, shading frame accessors) are gated by
  `RAYTRACING_ENABLED`.
- A header comment acknowledges the layering debt: `class Patch; // TODO:
  this is coupling RayHit with skin level classes :(`.

### 3.2. VITRAL `RayHit`

- Public `p`, `n`, `t` (tangent), `u`, `v`, `material`, `texture`,
  `normalMap`; private consumed `Ray`, `hitDistance`, and
  `requiredDetailMask_` with `DETAIL_NONE/POINT/NORMAL/UV/TANGENT/ALL`
  constants and `needsPoint()/needsNormal()/...` queries.

### 3.3. Alignment status

The two records agree on intent more than povCpp's did: both are
**nearest-hit** records with a lazy-detail contract. The differences:

| VITRAL `RayHit` | rpkCpp `RayHit` | Status |
|---|---|---|
| `p` | `point` (float `Vector3D`) | aligned concept, name/precision differ |
| `n` | `geometricNormal` + `shadingFrame.getZ()` | rpkCpp splits geometric vs shading normal — richer |
| `t` tangent | `shadingFrame.getX()` | representable via the frame |
| `u`, `v` | `uv` (`Vector2Dd`) + `texCoord` (`Vector3D`) | aligned concept |
| `hitDistance` | not stored; `maximumDistance` parameter is updated in place | divergent |
| `requiredDetailMask` (request) | `hitFlags` argument (request) + `flags` (filled) | same contract, split differently: rpkCpp separates "requested" from "filled" |
| `material`, `texture`, `normalMap` | `material`; texture lives on `Material` | aligned role, different material model |
| — | `patch` (hit primitive back-pointer) | rpkCpp-specific: radiosity needs the patch identity, not just surface data |

Required alignment item: map `RayHitFlag` values onto VITRAL's `DETAIL_*`
constants (or a superset), and keep rpkCpp's separation of
"requested mask" vs "filled mask" as the proposed improvement to VITRAL —
VITRAL currently overloads one mask for both meanings. The `Patch*`
back-pointer stays rpkCpp-specific (radiosity attribution), like povCpp's
`hitGeometry`/`hitBody` chain.

## 4. Intersection operation model

### 4.1. VITRAL: nearest-hit primitive

```cpp
virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) = 0;
virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outHit);
virtual int computeQuantitativeInvisibility(const Vector3Dd& origin, const Vector3Dd& p);
virtual double* getMinMax() = 0;
virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);
```

### 4.2. rpkCpp: nearest-hit with interval and flags

```cpp
virtual RayHit *discretizationIntersect(
    Ray *ray, float minimumDistance, float *maximumDistance,
    int hitFlags, RayHit *hitStore) const;              // Geometry
RayHit *intersect(...same shape...);                     // Patch
static RayHit *listDiscretizationIntersect(...);         // over geometry lists
RayHit *VoxelGrid::gridIntersect(...);                   // accelerated
```

Semantics: return the nearest hit in `(minimumDistance, *maximumDistance)`,
writing into the caller-provided `hitStore` and shrinking
`*maximumDistance` — the caller-owned store doubles as the "closest so far"
accumulator across list/grid traversal. Shadow tests pass `hitFlags` that
allow any-hit early out.

### 4.3. Alignment status

The alignment is about naming and types:

- `discretizationIntersect` ↔ `doIntersectionFirstHit`: same role. The rpkCpp
  extras — distance interval, in-place `maximumDistance` shrinking,
  caller-provided store, any-hit flags for shadows — are genuinely useful and
  are the candidate *upstream* improvements (VITRAL's signature cannot express
  a shadow any-hit or a capped interval today).
- rpkCpp has no `doContainmentTest` / `computeQuantitativeInvisibility`; not
  needed by current algorithms, nothing to do.
- Precision: rpkCpp intersects in `float` distances; VITRAL in `double`.

Required alignment item: agree the shared nearest-hit signature. Proposed
direction: VITRAL adopts an interval+flags overload (superset of its current
contract); rpkCpp adopts the `doIntersectionFirstHit` name once signatures
match. Renaming before that agreement would create a false cognate.

## 5. Geometry model

### 5.1. VITRAL

`environment/geometry/Geometry.h` is a small **abstract interface** (see
§4.1); concrete shapes live in `curve/`, `surface/`, `volume/` and are
analytic (sphere, box, ...). Placement is owned by `SimpleBody` at scene
level; geometry is transform-free.

### 5.2. rpkCpp

`toolkit/skin/` is a **concrete aggregation hierarchy** over patches:

- `Geometry` (base): `id`, `boundingBox`, `radianceData` (`Element*` hook for
  the radiance method), `bounded`/`shaftCullGeometry`/`omit` flags,
  `className` discriminator, plus static list operations
  (`listDiscretizationIntersect`, `listBounds`). It also carries **static
  mutable state**: `nextGeometryId`, `excludedGeometry1/2` (the
  "don't self-intersect" mechanism), and the header admits the debt:
  `public: // Will become protected`.
- `Compound` — a list of child geometries (hierarchical scenes, clusters).
- `MeshSurface` — a material + vertex/patch list.
- `PatchSet` — a bare patch list.
- The actual surface primitive is `Patch` (triangles/quads with vertices,
  normal, plane constant, area, jacobian, `radianceData`), in
  `environment/geometry/elements/`.

There are **no analytic solids and no transforms**: the MGF reader bakes
everything into world-space patches at load time.

### 5.3. Alignment status

The two `Geometry` classes share a name but not a level of abstraction:
VITRAL's is "an intersectable analytic surface"; rpkCpp's is "a node in a
patch-aggregation scene tree". The honest mapping is:

| VITRAL | rpkCpp | Notes |
|---|---|---|
| `Geometry` (abstract intersectable) | `Patch` + `Geometry::discretizationIntersect` | the intersectable unit is the patch |
| `SimpleBody` / `SimpleBodyGroup` | `MeshSurface` / `Compound` | body-with-material / grouping |
| `SimpleScene` | `Scene` | §6 |
| analytic `surface/`, `volume/` shapes | — (patches only) | rpkCpp has no analytic solids |
| — | `PatchSet`, cluster octree, `radianceData` hooks | radiosity-specific, stays application-side |

Required alignment items:

- Do **not** force-rename either `Geometry` yet; first split rpkCpp's
  `Geometry` into a geometric core vs radiosity annotations (`radianceData`,
  `shaftCullGeometry`, `omit` are algorithm state stamped onto scene
  structure — the same concern `doc/patchBoundingBoxDecoupling.md` records
  for `Patch`/`BoundingBox`).
- Replace the static `excludedGeometry1/2` / `Patch::excludedPatches`
  mechanism with per-render context state (also required by design
  objective #4).
- rpkCpp's patch/mesh model is itself the candidate upstream contribution:
  VITRAL has meshes for painting but no patch-set intersectable with
  radiosity hooks.

## 6. Scene model and transforms

- **rpkCpp `Scene`** owns: the geometry list, the flat patch list, the
  clustered hierarchy (`PatchClusterOctreeNode`), the `VoxelGrid`, `Camera`
  and `Background`. Already encapsulated (private helpers, accessors).
- **VITRAL `SimpleScene`** owns `SimpleBody`/`SimpleBodyGroup` collections and
  snapshots (`SimpleSceneSnapshot`).
- Transforms: VITRAL `SimpleBody` owns decomposed position/scale/rotation
  (+quaternions, fast-path flags). rpkCpp has **no runtime transforms at
  all** — MGF geometry is baked to world space at parse time. Nothing to
  converge until an application needs instancing; record the difference and
  move on.

Alignment status: concepts map (`Scene` ↔ `SimpleScene`), contents differ by
domain (acceleration structures and radiosity clustering vs body grouping and
snapshots). No forced convergence; shared vocabulary can emerge when geometry
classes converge (§5).

## 7. Camera

- **rpkCpp `Camera`** (`toolkit/scene/`): public float attributes — eye/look/up,
  FOV pair, near/far, resolution, the eye frame `X/Y/Z`, background color,
  derived per-pixel state (`pixelWidth`, tangents) and the four view planes
  used for frustum culling. Mutable, self-completing (`complete()` recomputes
  derived state).
- **VITRAL `Camera`** (`environment/camera/`): encapsulated, exports an
  immutable `CameraSnapshot` (eye, direction, up/right with scale, projection
  mode) that the raytracer consumes; povCpp already renders from
  `CameraSnapshot`.

Required alignment item: same recipe as povCpp — keep the application camera
(rpkCpp `Camera` carries radiosity-view concerns like frustum planes and
`changed` tracking) but produce a VITRAL `CameraSnapshot` at the
render boundary, so the raytracer family reads the shared record. Encapsulate
rpkCpp `Camera` per M1 along the way.

## 8. Material model

- **rpkCpp**: `Material` (private: `edf`, `bsdf`, `sided`, `name`) over a
  physically-based Phong family — `PhongEmittanceDistributionFunction`,
  `PhongBidirectionalReflectanceDistributionFunction`, `...Transmittance...`,
  combined by `PhongBidirectionalScatteringDistributionFunction`, plus
  `RefractionIndex`, `Texture`, and BSDF component/sampling-mode flags.
  Already in the target encapsulation style.
- **VITRAL**: `Material` base with `SimpleMaterial` (the raytracer's
  Phong-ish surface: colors, texture, normal map) and `MicroFacetedMaterial`.

Alignment status: different physical models at different maturity. rpkCpp's
EDF/BSDF decomposition is the more principled radiometric model and is a
strong upstream candidate once VITRAL needs physically-based rendering; in
the meantime the two coexist, exactly as povCpp's `PovRayMaterial` stays
application-side.

Required alignment item: none immediate. When VITRAL gains a
physically-based material interface, rpkCpp's Phong EDF/BSDF family is the
proposed seed.

## 9. Renderer configuration

Both trees define `RendererConfiguration` — same name, **disjoint
vocabularies**:

- rpkCpp (`toolkit/material/RendererConfiguration.h`): display/debug options
  (outlines, bounding boxes, clusters, smooth shading, backface culling,
  frustum culling, freeze-raytraced-image), private with getters.
- VITRAL (`environment/material/RendererConfiguration.h`): `SHADING_TYPE_*`
  vocabulary (NOLIGHT/FLAT/GOURAUD/PHONG/COOK_TERRANCE) plus viewer options
  (wires, bounding volume, texture, bump map, LOD hint...).

povCpp precedent: `PovRayRendererConfiguration` **derives from** the VITRAL
base, reusing the shading-type vocabulary and keeping app flags separate.

Required alignment item: same move — make rpkCpp's configuration derive from
(or embed) the VITRAL base, mapping `smoothShading`/`noShading` onto
`SHADING_TYPE_*`, and keep the radiosity-specific display flags
application-side.

## 10. Bounding information

- rpkCpp: `skin/AxisAlignedBoundingBox` (+ `MinMaxBox`,
  `BoundingBoxCoordinateIndex`), stored by value on `Geometry`, plus
  per-patch boxes (decoupling plan already written in
  `doc/patchBoundingBoxDecoupling.md`).
- VITRAL: `virtual double* getMinMax() = 0` on `Geometry`.
- povCpp: proposes `BoundingVolumeHierarchy` / `AabbBoundingVolume` /
  `AxisAlignedBoundingBox` as the shared winner (its
  `vitralNormalizationAnalysis.md` §10/§14.4).

Required alignment item: this is now a **three-tree decision**, and povCpp's
proposal is inherited: a polymorphic bounding-volume contract whose naive
implementation wraps a vptr-free AABB value. rpkCpp's
`AxisAlignedBoundingBox` is already the value type of that shape; when the
shared contract lands in VITRAL, rpkCpp adopts it instead of growing its own.
Until then, finish `patchBoundingBoxDecoupling.md` first — it moves bounds out
of `Patch` into a dedicated store, which is compatible with any winner.

## 11. Statistics

- rpkCpp (`common/statistics/`): per-domain **instance** counter classes —
  `Statistics`, `RadianceStatistics`, `RayTracerStatistics`,
  `ShadowStatistics`, `PotentialStatistics`, `ReaderStatistics`.
- VITRAL: static event recorder `RaytraceStatistics`
  (+ `GeometryStatistics`/`SolidTextureStatistics` colocated by the povCpp
  effort).

The povCpp learning-experience document already works this example through:
one shared event taxonomy, instance ownership for thread aggregation
(povCpp's parts-summing constructor), shared sub-models kept as-is. rpkCpp's
instance-based design is on the winning side of that direction; its
contribution is the per-domain split (reader/shadow/potential) as taxonomy
input.

Required alignment item: participate in the same taxonomy discussion; no
unilateral rename.

## 12. rpkCpp-only subsystems (future upstream contributions)

No VITRAL counterpart exists for any of these; they are what rpkCpp brings to
the partnership (the analogue of povCpp bringing CSG and all-crossings):

1. **Galerkin radiosity** (`toolkit/galerkin/`): hierarchical Galerkin with
   higher-order bases, clustering, shaft culling, Jacobi/Gauss-Seidel/
   Southwell iteration — the flagship.
2. **Raytracer family** (`toolkit/raycasting/`): stochastic raytracing /
   random walk, bidirectional path tracing, photon map, ray matting.
3. **Tone mapping** (`toolkit/tonemap/`): Ward, TumblinRushmeier (+revised),
   Ferwerda, Lightness operators behind a `ToneMap` base.
4. **Quasi-Monte-Carlo** (`numericalAnalysis/quasiMonteCarlo/`): Niederreiter
   31/63-bit sequences.
5. **MGF scene format** (`io/mgf/`) and the software rasterizer
   (`render/sgl/`).

These migrate to VITRAL only after their supporting models (§1–§4, §8) have
converged — never before, or they would drag the divergent types with them.

## 13. Current alignment matrix

| Area | Current rpkCpp `cpp/` | Current VITRAL | Alignment status |
|---|---|---|---|
| Vector types | `Vector3D` float public + minimal `Vector3Dd` | `Vector3Df`/`Vector3Dd` encapsulated | name collision, style divergent |
| Ray | float, public `position`/`direction`, no `t` | double, private `origin`/`direction`/`t`, builders | divergent |
| Hit record | `RayHit` encapsulated, patch back-pointer, request/fill flag split | `RayHit` public surface fields, one detail mask | same intent, different shape; rpkCpp split is the proposed improvement |
| Intersection primitive | nearest-hit, interval + hitFlags + caller store | nearest-hit `doIntersectionFirstHit` | same primitive — naming/signature convergence only |
| Geometry | concrete patch-aggregation tree + radiosity hooks + static excluded state | small abstract intersectable | same name, different abstraction level |
| Scene | `Scene` + VoxelGrid + cluster octree | `SimpleScene`/`SimpleBody` | concept maps, contents domain-specific |
| Transforms | none (world-space baked at MGF load) | decomposed on `SimpleBody` | intentional divergence, documented |
| Camera | public float camera + frustum planes | `Camera` + `CameraSnapshot` | adopt snapshot at render boundary |
| Material | Phong EDF/BSDF family, encapsulated | `SimpleMaterial`/`MicroFacetedMaterial` | different models; rpkCpp is upstream seed |
| RendererConfiguration | display/debug flags, private | shading-type vocabulary + viewer flags | name collision; derive-from-base per povCpp precedent |
| Bounding | `AxisAlignedBoundingBox` value on `Geometry`/`Patch` | `getMinMax()` | three-tree decision; povCpp `BoundingVolumeHierarchy` proposal inherited |
| Statistics | per-domain instance counters | static recorder | povCpp worked example governs |
| Radiosity / tonemap / QMC / MGF | full subsystems | — | rpkCpp-only, future contributions |
| Encapsulation style | mixed (Material/RayHit/Scene private; Vector/Ray/Camera/Patch/Element/Vertex public) | private + getters + constructors, mutable | migration pending (M1) |

## 14. Required alignment work

Ordered; each step is gated on an unchanged golden-image corpus
(`cpp/scripts/*.sh` vs `doc/testBaseImages`) and, where hot paths are
touched, on the timing baselines of `doc/performanceImprovementPlan.md`.
Every completed step is then propagated 1:1 to the `java/`, `typescript/` and
`turboc/` ports before the next step starts.

1. **Kill static mutable state**: move `Patch::excludedPatches`,
   `Geometry::excludedGeometry1/2` and the ID counters into per-render
   context objects. Prerequisite for objective #4 and for sharing any class
   with VITRAL. (§5)
2. **Encapsulation migration** of the public-attribute core — `Vector3D`,
   `Vector3Dd`, `Ray`, `Camera`, `Vertex`, `Element`, `Patch`, `Geometry` —
   to private attributes with getters and full-state constructors, **keeping
   the classes mutable** (setters/in-place mutators preserved) so hot loops
   lose nothing; inline accessors keep the abstraction cost at zero. Do it
   class by class, innermost types first (`Vector3D` → `Ray` → `Vertex`/
   `Patch` → `Element` → `Geometry` → `Camera`), measuring each. (M1)
3. **Naming convergence on the shared primitives**: `Vector3Df`/`Vector3Dd`
   scheme, ray `origin`, `DETAIL_*`-compatible hit flags — only after step
   2, and jointly with VITRAL so both trees rename once. (§1–§3)
4. **Shared nearest-hit signature**: negotiate the interval+flags+store
   extensions into VITRAL's `doIntersectionFirstHit`, then adopt the shared
   name in rpkCpp. (§4)
5. **`RendererConfiguration` derives from the VITRAL base**, mapping shading
   flags onto `SHADING_TYPE_*`. (§9)
6. **Bounding contract**: adopt the povCpp-proposed `BoundingVolumeHierarchy`
   family when it lands in VITRAL; finish `patchBoundingBoxDecoupling.md`
   first. (§10)
7. **Statistics taxonomy**: join the povCpp/VITRAL shared-taxonomy design
   with rpkCpp's per-domain split as input. (§11)
8. **Upstream contributions** (Galerkin, raytracer family, tone mapping,
   QMC, MGF) — only after their supporting types are shared. (§12)
