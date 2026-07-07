# Galerkin Radiosity — Performance Improvement Plan

Status: PROPOSED (2026-07-07)

Scope: single-process, single-thread optimizations only. Parallelization is explicitly
out of scope for this plan.

Reference workload: `scripts/02_runCorridor.sh` (corridor scene, clustered hierarchical
Galerkin, Jacobi, 21 iterations, linear basis, exact visibility, shaft culling for
refinement — all defaults from `GalerkinState.h`).

---

## 1. Baseline measurements

Measured on the development machine (Linux 6.2, 72 cores — only 1 used), build flags
`-O3 -ffast-math` (`base/CMakeLists.txt:15`):

| Metric (corridor, 21 iterations)          | Value        |
|-------------------------------------------|--------------|
| Wall clock, full script                   | 4 min 39 s   |
| User CPU time                             | 276.7 s      |
| Solver CPU time (app counter, 21 iters)   | ~256 s       |
| Peak resident memory                      | 1.13 GB      |
| Elements created                          | 118 574+     |
| Interactions alive at iteration 6         | 3 517 504    |
| Shadow rays traced (6 iterations)         | 191.6 M      |
| Shadow rays answered by shadow cache      | 50.8 M (28%) |

Per-iteration solver cost (cumulative CPU seconds from the app's own counter):

| Iteration | 1    | 2     | 3     | 4     | 5     | 6     | 7..21      |
|-----------|------|-------|-------|-------|-------|-------|------------|
| CPU (s)   | 21.5 | 142.8 | 188.4 | 211.3 | 220.8 | 225.1 | +2.3 s each|

Iterations 1–6 are ~97 % of the solver cost. The cost of an iteration is dominated by
*creation and refinement of new interactions* (form factors + visibility rays); once the
link hierarchy stabilizes (iteration ~7 on), an iteration costs ~2.3 s (pure transport +
push/pull + oracle re-evaluation).

### 1.1 CPU profile (perf, 6 iterations, cycles, self cost)

| Self % | Function |
|-------:|----------|
| 23.9 % | `Geometry::discretizationIntersectPreTest` (bbox pre-test per geometry per ray) |
|  9.7 % | `Patch::intersect` |
|  6.8 % | `Patch::quadUv` (point-in-quad test inside `hitInPatch`) |
|  5.5 % | `PatchSet::discretizationIntersect` |
|  5.5 % | `Shaft::shaftPatchTest` |
|  5.2 % | `Geometry::listDiscretizationIntersect` |
|  4.7 % | `Geometry::getRayIntersectionBox` (rebuilds a `MinMaxBox` copy per call) |
|  3.7 % | `Shaft::boundingBoxTest` |
|  2.6 % | `Patch::hitInPatch` |
|  2.3 % | `AxisAlignedBoundingBox::copyFrom` |
|  2.3 % | `FormFactorStrategy::evaluatePointsPairKernel` |
|  2.1 % | `Compound::discretizationIntersect` |
|  1.7 % | `ClusterTraversalStrategy::traverseAllLeafElements` |
|  1.4 % | `RayHit::RayHit()` |
|  1.3 % | `Shaft::cullPatches` |
|  1.2 % | `MaximumRadianceVisitor::visit` |
| ~1.7 % | `malloc` / `free` (visible part only) |

### 1.2 Inclusive cost by responsibility

| Inclusive % | Subsystem |
|------------:|-----------|
| 95.5 % | `HierarchicalRefinementStrategy::refineInteractions` (whole solver) |
| 75.9 % | `FormFactorStrategy::computeAreaToAreaFormFactorVisibility` |
| 71.7 % | `FormFactorStrategy::shadowTestDiscretization` (**shadow-ray visibility**) |
| 13.6 % | `HierarchicalRefinementStrategy::hierarchicRefinementCull` (shaft culling) |
|  6.1 % | `ShadowCache::cacheHit` |
|  4.1 % | refinement oracle (`hierarchicRefinementEvaluateInteraction`) |
|  3.4 % | `ClusterTraversalStrategy::maxRadiance` (leaf traversal per oracle call) |

Conclusion: **~2/3 of all CPU goes into occlusion testing of the 7×6 = 42 cubature
node-pair rays traced for every candidate link**, and ~11 % into shaft culling that
prepares the occluder candidate lists for those rays. Everything else is second order.

### 1.3 Exact call counts (callgrind, partial dump inside iterations 1–2)

| Calls        | Function | Notes |
|-------------:|----------|-------|
|      584 583 | `computeAreaToAreaFormFactorVisibility` | one per candidate (sub-)link |
|    1 168 528 | `determineNodes` | exactly 2× the form factors → node cache is dead (item 1.3) |
|   26 899 380 | `evaluatePointsPairKernel` | ≈46 per form factor |
|   17 969 166 | `shadowTestDiscretization` | 1 per non-trivially-rejected kernel eval |
|   17 969 166 | `ShadowCache::cacheHit` | every shadow ray probes the 5-entry cache |
|  423 788 559 | `Geometry::discretizationIntersectPreTest` | **~24 geometry bbox tests per shadow ray** |
|  415 006 071 | `Geometry::getRayIntersectionBox` | ≈1 bbox copy per pre-test (item 1.1) |
|  426 282 440 | `AxisAlignedBoundingBox::copyFrom` | mostly from the line above |
|  380 173 896 | `PatchSet::discretizationIntersect` | list unwrapping per ray |
|  144 972 221 | `Patch::intersect` | ~8 patch plane tests per shadow ray |
|   66 023 072 | `Patch::hitInPatch` / `quadUv` | plane test passed, point-in-polygon done |
|    8 083 329 | `Shaft::cullGeometry` (incl. recursion) | shaft culling volume |
|    7 953 126 | `Shaft::shaftPatchTest` | exact patch-vs-shaft tests |
|   18 394 705 | `Shaft::boundingBoxTest` | box-vs-shaft tests |
|      538 099 | `hierarchicRefinementEvaluateInteraction` | oracle invocations |
|      399 233 | `Interaction::interactionDuplicate` | stored links (heap `new` each) |
|      108 309 | `GalerkinElement::regularSubDivide` | |

---

## 2. Flow analysis (what happens per iteration)

Defaults put the corridor run on this path:

1. `GalerkinRadianceMethod::doStep` → `GatheringClusteredStrategy::doGatheringIteration`
   (`clustered = true`, Jacobi).
2. Iteration 1 creates a single self-link `topCluster ↔ topCluster`
   (`LinkingClusteredStrategy::createInitialLinks`).
3. `HierarchicalRefinementStrategy::refineInteractions` walks the whole element
   hierarchy bottom-up and re-runs the refinement oracle on **every stored interaction,
   every iteration**:
   - `hierarchicRefinementEvaluateInteraction` estimates the transport error
     (`deltaK * srcRad`); for cluster sources this calls
     `ClusterTraversalStrategy::maxRadiance`, a full traversal of the cluster's leaves.
   - If the error exceeds the threshold, the larger end is subdivided
     (`hierarchicRefinementRegularSubdivideSource/Receiver`,
     `hierarchicRefinementSubdivideSource/ReceiverCluster`). Each subdivision:
     - shaft-culls the occluder candidate list (`hierarchicRefinementCull` →
       `Shaft::doCulling` / `Shaft::cullGeometry`),
     - creates up to 4 (or #children) sub-links, and for each computes a full
       area-to-area form factor: 42 kernel evaluations, each potentially one shadow ray
       (`evaluatePointsPairKernel` → `shadowTestDiscretization`),
     - recurses.
   - Accurate-enough links transport radiance
     (`hierarchicRefinementComputeLightTransport`).
4. `GalerkinBasis::pushPullRadiance` makes the multilevel representation consistent,
   then patch colors are recomputed.

Radiance grows monotonically during the first iterations, so the oracle keeps finding
links to refine until iteration ~6; afterwards the hierarchy is stable and iterations
are cheap.

---

## 3. Improvement plan by phases

Ordering rationale: each phase groups changes with a similar risk profile, starting with
changes that must be numerically bit-exact (cheap to validate) and ending with changes
that can alter results within the tolerance of the golden-image gate (expensive to
validate). Within a phase, items are ordered by expected payoff.

Every phase ends with the full validation protocol of section 4. Do not mix items from
different phases in one commit; keep one commit per item so a gate regression bisects
trivially.

### Phase 0 — Measurement harness (prerequisite, ~half a day)

Goal: make regressions and wins observable in minutes, since the full golden-image gate
costs ~1 hour.

0.1 Add `scripts/perfBaseline.sh`: runs `02_runCorridor.sh` under `/usr/bin/time -v`,
    extracts wall/CPU/peak-RSS plus the app's own counters (elements, interactions,
    shadow rays, cached hits, CPU time per iteration) into a small text report saved
    under `output/perf/`. Keep the reports of every optimization step.

0.2 Add a *fast statistical gate*: the counters printed by `getStats()`
    (`GalerkinRadianceMethod.cpp:496`) — number of elements, clusters, interactions per
    type — are a fingerprint of the refinement decisions. For any change claimed to be
    *bit-exact* (Phase 1) these counters must be **identical** to baseline for the
    corridor run; that check takes ~5 minutes instead of the 1-hour image gate.
    Recommended: extend the per-iteration printout (or dump to a file with a hidden
    option) so the comparison is `diff`-able.

0.3 Document in this file, per item, the measured delta (before/after CPU seconds).

### 0.4 Measurement log

| Date | Scope | Report | Wall | User CPU | Solver CPU | Notes |
|------|-------|--------|------|----------|------------|-------|
| 2026-07-08 | Phase 1 partial workspace state: 1.1, 1.2, 1.4, 1.5, 1.6, 1.7 applied; 1.3 still disabled; 1.8 not started | `cpp/output/perf/corridor_20260708_001941.txt` | 4:07.66 (**-11.23 %** vs 4:39) | 245.53 s (**-11.26 %** vs 276.7 s) | 230.012 s (**-10.15 %** vs ~256 s) | Iteration-6 fingerprint remains identical to baseline: 118 574 elements, 3 517 504 interactions, 191 609 431 shadow rays. |
| 2026-07-08 | Phase 1 item 1.3 applied: cubature-node/form-factor pair cache re-enabled | `cpp/output/perf/corridor_20260708_003220.txt` | 4:10.14 (**+1.00 %** vs previous 4:07.66) | 247.45 s (**+0.78 %** vs previous 245.53 s) | 232.041 s (**+0.88 %** vs previous 230.012 s) | Iteration-6 fingerprint remains identical: 118 574 elements, 3 517 504 interactions, 191 609 431 shadow rays. Single-run timing shows no measurable win; may be noise or the node-cache payoff is below run variance. |
| 2026-07-08 | Phase 1 item 1.8 applied: inline `Interaction::k0`/`deltaK` (no heap alloc for the common 1×1-basis link; `deltaK` was always a single coefficient, made a plain `float`) | `cpp/output/perf/corridor_20260708_020831.txt` | 4:05.94 (**-1.68 %** vs previous 4:10.14) | 242.91 s (**-1.83 %** vs previous 247.45 s) | 227.716 s (**-1.86 %** vs previous 232.041 s) | Fingerprint identical at every iteration (e.g. iteration 21: 118 982 elements, 3 540 397 interactions, 191 974 102 shadow rays, 50 891 720 cached — matches previous run exactly). Peak RSS 991 224 KB, down from the 1.13 GB baseline. Phase 1 (1.1–1.8) is now complete. |

Per-iteration cumulative solver CPU, current Phase 1 partial workspace vs baseline:

| Iteration | Baseline CPU (s) | Current CPU (s) | Change |
|-----------|------------------|-----------------|--------|
| 1 | 21.5 | 19.428 | -9.64 % |
| 2 | 142.8 | 127.197 | -10.93 % |
| 3 | 188.4 | 167.041 | -11.34 % |
| 4 | 211.3 | 187.825 | -11.11 % |
| 5 | 220.8 | 196.323 | -11.09 % |
| 6 | 225.1 | 200.140 | -11.09 % |

Per-iteration cumulative solver CPU after 1.3 vs the previous Phase 1 partial
workspace measurement:

| Iteration | Previous CPU (s) | After 1.3 CPU (s) | Change |
|-----------|------------------|-------------------|--------|
| 1 | 19.428 | 19.4916 | +0.33 % |
| 2 | 127.197 | 128.307 | +0.87 % |
| 3 | 167.041 | 168.674 | +0.98 % |
| 4 | 187.825 | 189.577 | +0.93 % |
| 5 | 196.323 | 198.382 | +1.05 % |
| 6 | 200.140 | 202.384 | +1.12 % |

### Phase 1 — Bit-exact mechanical optimizations (expected ~15–25 % solver CPU)

These change no arithmetic and no evaluation order of floating-point accumulations; the
fast statistical gate must show identical counters, and images must be byte-identical
(a plain `cmp` against a pre-change render is a valid check here, stronger and cheaper
than the perceptual gate).

1.1 **Stop rebuilding `MinMaxBox` on every bbox pre-test.**
    `Geometry::getRayIntersectionBox()` (`skin/Geometry.cpp:70`) copies the geometry
    bounding box into a lazily heap-allocated `MinMaxBox` on *every call*, and it is
    called once per bounded geometry per shadow ray
    (`discretizationIntersectPreTest`, 23.9 % self + 4.7 % + 2.3 % `copyFrom`).
    The `MinMaxBox` adds no information — it wraps the same `boundingBox`. Give
    `AxisAlignedBoundingBox` (or a free function) the slab test and call it directly on
    `this->boundingBox`; delete the lazy copy/update. Also applies to
    `geometryMultiResolutionVisibility` (`FormFactorClusteredStrategy.cpp:115`).
    Expected: several percent of total CPU.

1.2 **Replace `dynamic_cast` with `static_cast` in the ray-intersection dispatch.**
    `Geometry::discretizationIntersect` (`skin/Geometry.cpp:225-231`) and
    `Compound::discretizationIntersect` dispatch on `className` and then perform an
    RTTI `dynamic_cast` per geometry per ray. The class is already proven by
    `className`; use `static_cast`. Same in `Geometry::patchListReference` and
    `Shaft::keep`/`shaftCullOpen` (hot during culling).

1.3 **Re-enable the cubature-node cache in the form-factor code.**
    `FormFactorStrategy::computeAreaToAreaFormFactorVisibility`
    (`FormFactorStrategy.cpp:496-497`) sets `formFactorLastReceived/Source = nullptr`
    on entry, which makes the `receiverElement != formFactorLastReceived` checks always
    true, so `determineNodes` (cubature rule + node positions, incl. `uniformPoint` per
    node) is recomputed on every call. The refinement loops call this 4+ times in a row
    with the same receiver (subdivide-source) or same source (subdivide-receiver), so
    the cache exists precisely for this pattern but is disabled. Restore it:
    - remove the two nullptr assignments; set the statics at the end (already done);
    - keep correctness: `Gxy`, `maximumKernelValue` and `visibilityCount` must also
      become `static` (they are only valid when *both* elements repeat — which is the
      case the original code guarded with the combined check at line 574);
    - long term (see 2.6) move all of these statics into a small POD member of
      `GalerkinState` to remove the re-entrancy hazard noted in the TODO at line 485.

1.4 **Hoist loop-invariant work out of the refinement oracle.**
    `hierarchicRefinementLinkErrorThreshold` (`HierarchicalRefinementStrategy.cpp:121`)
    recomputes `Statistics::instance()` lookups, `maximumComponent()` and (for
    POWER_ERROR) a division per evaluated interaction, i.e. millions of times per
    iteration; without importance the radiance-error threshold and the constant factor
    of the power-error threshold are invariant during a whole run, and `minimumArea`
    (`...:322`) likewise. Compute once per iteration into `GalerkinState`.

1.5 **Remove allocation churn in the per-element refinement loop.**
    `refineInteractions` (`HierarchicalRefinementStrategy.cpp:879`) allocates a
    `java::ArrayList<Interaction*>` (which pre-allocates a 100-slot buffer,
    `ArrayList.txx:6-11`) for *every element visit of every iteration* (~100 k elements
    × 21 iterations), almost always ending up empty. Replace the collect-then-remove
    pattern with a single order-preserving compaction pass over
    `parentElement->interactions` (write-index compaction): O(n) instead of
    O(n²) (`ArrayList::remove(T)` does a linear search plus a linear shift per removed
    link), zero allocations, and identical resulting order (bit-exact).

1.6 **Fix `ArrayList` growth policy.** `ArrayList::add` grows by a fixed chunk
    (default +100, `ArrayList.txx:53-71`); large lists (interaction lists, culled
    candidate lists, patch lists) pay O(n²/chunk) copying. Use geometric growth (×2
    or ×1.5) and allocate the backing array lazily on first `add` (the default
    constructor currently allocates 100 slots even for lists that stay empty — e.g.
    `GalerkinElement::interactions` of the ~40 k leaf elements that never receive
    links). Order of elements is unaffected → bit-exact.

1.7 **Avoid needless work on the ISOTROPIC clustering path (the default).**
    `ClusterTraversalStrategy::gatherRadiance` (`ClusterTraversalStrategy.cpp:239`)
    computes `sourceElement->midPoint()` (4–8 `uniformPoint` calls) and heap-allocates
    an `OrientedGathererVisitor` *before* the switch, then uses neither in the
    ISOTROPIC branch. Move both into the branches that need them, and stack-allocate
    all the small visitors (`PowerAccumulatorVisitor`, `MaximumRadianceVisitor`,
    `ProjectedAreaAccumulatorVisitor`, `DepthVisibilityGathererVisitor` — all created
    with `new`/`delete` per call).

1.8 **Inline the 1×1 coefficient storage in `Interaction`.**
    Every stored link heap-allocates `K = new float[1]` and `deltaK = new float[1]`
    (`Interaction.cpp:43-57`), and `computeInteractionError`
    (`FormFactorStrategy.cpp:357-360`) even does `delete[] deltaK; deltaK = new
    float[1]` per form-factor evaluation. With 3.5 M live links that is ≥7 M tiny
    allocations plus pointer-chasing on every transport. Give `Interaction` an inline
    `float k0` / `float deltaK0` (or a small fixed array of
    `MAX_BASIS_SIZE²` floats only when basis > 1) and use the pointer only for the
    higher-order case. Reduces allocator traffic and memory (also helps the 1.13 GB
    peak RSS).

### Phase 2 — Result-identical caching & memory layout (expected ~10–15 % solver CPU)

Values computed are identical, but memory layout / cache-content changes may perturb
malloc addresses etc. Validate with the fast statistical gate (counters must still be
identical) + one full golden-image run at phase end.

2.1 **Memoize `ClusterTraversalStrategy::maxRadiance` per cluster per iteration**
    (3.4 % inclusive). The oracle calls it for every interaction whose source is a
    cluster (~950 k surface-to-cluster links by iteration 6), each call traversing all
    leaves of the source cluster. Radiance only changes in `pushPullRadiance`, i.e.
    *between* refinement passes. Store `(iterationNumber, maxRadiance)` in
    `GalerkinElement` and recompute lazily once per iteration.

2.2 **Precompute basis-function tables at cubature nodes.**
    `computeInteractionFormFactor` (`FormFactorStrategy.cpp:252-295`) evaluates
    `basis->function[alpha](u_k, v_k)` through function pointers for every link. The
    values depend only on (basis, cubature rule) — four combinations in practice.
    Tabulate `phi[alpha][k]` once at startup next to the cubature rules in
    `GalerkinState`, and drop the per-call zero-initialization of the
    `receiverPhi[10][20]` array (initialize only the used sub-range).

2.3 **Arena-allocate `Interaction` objects.** Stored links are created via
    `interactionDuplicate` (`Interaction.cpp:111`) with global `new`, live in
    millions, and are freed in bulk between refinements. Use the existing
    `MemoryPool` (or a dedicated fixed-size free-list, since `free()` of the current
    pool is LIFO-only) so links of one element sit contiguously → fewer allocator
    cycles, better locality for the transport loop. Combine with 1.8.

2.4 **Cheaper `RayHit` handling in `Patch::intersect`** (`Patch.cpp:838`): the local
    `RayHit hit;` construction (1.4 % self) initializes the full record for every
    tested patch even though >90 % of tests miss. Defer constructing the record until
    the plane test and `hitInPatch` have passed (work directly with flags/locals).

2.5 **Shadow-cache tuning (measured, guarded by counters).** The 5-entry cache
    answers 28 % of the 191 M rays. Cheap experiments, each validated by the shadow-ray
    counters and gate: (a) test cache entries most-recently-added first;
    (b) MAX_CACHE 5 → 8/16; (c) keep a per-link "last blocker" and test it before the
    cache. Note: a cache change can alter *which* occluder is found (not *whether* —
    occlusion is boolean), so refinement counters stay identical but cached-hit
    counters will move; images must stay byte-identical.

2.6 **Move the form-factor statics into `GalerkinState`** (follow-up of 1.3): removes
    hidden global state (`formFactorLastReceived/Source`, static cubature/node
    arrays), no behavioral change, prepares the ground for any future re-entrancy.

### Phase 3 — Visibility-algorithm improvements (expected 20–40 % solver CPU; results may move within gate tolerance)

These change the amount or order of floating-point work in visibility, so images can
differ at the last-ulp level. `-ffast-math` is already on, so the project has accepted
this class of tolerance. Validate each item with the full golden-image gate.

3.1 **Early-exit on ANY-hit in the voxel grid.**
    `VoxelGrid::voxelIntersect` / `gridIntersect` (`scene/VoxelGrid.cpp:433-507`)
    keep testing items and walking voxels after a hit even when the caller passed
    `RayHitFlag::ANY` (all Galerkin shadow rays do). The list-based path already
    early-exits (`Geometry.cpp:250`); mirror that behavior in the grid path.
    Occlusion answer is unchanged (boolean), but the *reported* blocker may differ →
    shadow-cache contents differ; refinement decisions do not.

3.2 **Precompute the ray's inverse direction for slab tests.** Every
    box test divides by `direction.{x,y,z}` (`MinMaxBox.h:56`), i.e. 3 divisions per
    geometry per ray. Extend the slab test to take precomputed `1/direction`
    (computed once per shadow ray in `shadowTestDiscretization` /
    `evaluatePointsPairKernel`). Multiplication vs. division can differ in the last
    ulp → gate-validated.

3.3 **Flatten the per-link occluder list once, trace 42 rays against the flat list.**
    Today each of the 42 rays of one form factor re-walks the candidate `Geometry`
    list (`Compound` recursion, `PatchSet` unwrapping, per-geometry pre-test). Build,
    once per link (in `computeAreaToAreaFormFactorVisibility`), a flat array of
    `(bbox, Patch* run)` entries and loop over it per ray with the ANY early-out.
    Removes most of `listDiscretizationIntersect`/`PatchSet::discretizationIntersect`
    overhead (~11 % self combined) and improves locality of the dominant loop.

3.4 **Propagate proven full visibility down the refinement recursion.**
    `refineInteraction` (`HierarchicalRefinementStrategy.cpp:815`) already passes a
    null occluder list when `exactVisibility && visibility == 255` — but only for the
    stored top link. When a *sub*-link is computed with the polygon-exact shaft and
    ends with `visibility == 254/255` (see the 254 marking at
    `FormFactorStrategy.cpp:649-652`), its own children can skip occluder testing the
    same way. Plumb the parent's visibility through the `subdivide*` helpers.
    Big saver in open scenes where most links are unoccluded (the corridor: the
    average candidate list is small precisely because most shafts are empty — those
    links should pay ~0 rays instead of 42 bbox walks).

3.5 **(Optional, bigger win / bigger risk) Adaptive cubature degree.** Rays scale with
    `receiverNodes × sourceNodes` (7 × 6). For links whose unrefined error estimate is
    far below threshold, or with `visibility == 255`, a degree-3 (4-node) or even
    2×2 rule gives the same refinement decisions at a fraction of the rays. This
    changes K values within their own error bounds; requires the full gate on all 8
    scenes and a careful look at scenes 03/05 (largest). Only attempt after 3.1–3.4
    are banked and if their combined win is insufficient.

### Phase 4 — Build-level changes (no source edits; expected 5–15 % wall clock)

Each is a one-line build change, trivially reversible, gate-validated once:

4.1 Link-time optimization: `-flto` for `vitral` + `rpk` (the hot path crosses the
    shared-library boundary through many small functions — `clipAxisSlab`,
    `ColorRgbMutable` ops — that LTO can inline). Consider
    `-fvisibility-inlines-hidden` as a first cheaper step.
4.2 `-march=native` (or a fixed `-march=x86-64-v3`) to vectorize the kernel loops.
4.3 PGO: `-fprofile-generate` → run corridor → `-fprofile-use`. The workload is highly
    branch-structured (bbox tests, oracle switches) and benefits from layout.
4.4 Alternative allocator via `LD_PRELOAD` (jemalloc/tcmalloc) in the run scripts:
    zero code change; measures the true cost of the allocation churn addressed in
    Phase 1/2 (if Phases 1–2 land first, this may become negligible — measure both).

Note: with `-ffast-math` active, 4.1–4.3 may reorder FP operations → images can move
within tolerance; run the full gate once per flag.

---

## 4. Validation protocol

The regression gate is `scripts/testReviewResults.sh`: perceptual comparison
(`findimagedupes -t 100`) of the 8 scene renders against `doc/testBaseImages`.
Rendering all scenes takes ~1 hour on this machine, so it is a *phase* gate, not a
*commit* gate.

Per commit (fast, ~5 min):
1. Rebuild, run `scripts/02_runCorridor.sh`.
2. Compare the statistics fingerprint (elements / clusters / interactions per type
   per iteration) against the recorded baseline:
   - Phase 1 and 2 changes: fingerprint must be **identical**, and the corridor PPM
     should be **byte-identical** (`cmp`) — a stronger and cheaper check than the gate.
   - Phase 3 and 4 changes: fingerprint may move slightly; record the new one.
3. Record wall/CPU/RSS deltas in `output/perf/`.

Per phase (slow, ~1 h):
1. Run all scene scripts (`scripts/runAll.sh` or the 01–06 subset covered by the gate).
2. Run `scripts/testReviewResults.sh` — all images must MATCH.
3. Never `--refresh-baseline` to make a failing phase pass; a MISMATCH means the
   change is reverted or demoted to "needs investigation".

Suggested acceptance target: corridor solver CPU from ~256 s to ≤ 130 s after Phases
1–3, without any golden-image mismatch.

---

## 5. Latent defects noticed during the analysis (fix separately, not for speed)

These were found while reading the hot paths. They are *not* performance items, but
they touch the same code and should be fixed (or at least ticketed) before heavy
refactoring, in separate commits:

- `LinkingSimpleStrategy::createInitialLink` (`LinkingSimpleStrategy.cpp:72-74`):
  `shaft.doCulling(oldCandidateList, *candidateList, ...)` culls into the *source*
  list (`*candidateList` still aliases `oldCandidateList`) and then installs the empty
  `arr` as the candidate list. Only reachable in non-clustered mode with
  `exactVisibility`/always-cull; the default (clustered) path never runs it.
- `ClusterTraversalStrategy::surfaceProjectedAreaToSamplePoint`
  (`ClusterTraversalStrategy.cpp:128-147`): local `samplePoint` is read uninitialized
  (UB). Only reachable with ORIENTED clustering (non-default).
- `Shaft::cullGeometry` (`Shaft.cpp:763-780`): `reinterpret_cast<Patch *>(geometry)`
  on a `Geometry` — the TODO comments already flag it; the id used for the omit test
  is garbage. Currently harmless only because omit-testing then fails to match.
- `GalerkinElement::reAllocCoefficients` frees arrays allocated with `new[]` using
  `delete` (`GalerkinElement.cpp:321,339,347`) — UB, works for PODs in practice.
- The form-factor cubature cache being disabled (`FormFactorStrategy.cpp:496-497`,
  item 1.3) looks like an unintended regression of the C→C++ port rather than a
  deliberate choice; the TODO above it suggests the author was aware of the hazard.

---

## 6. How the profiles were captured (reproducibility)

```bash
# Instrumented copy (same -O3 -ffast-math, plus symbols), built out of tree:
cmake -S cpp -B <scratch>/superbuild \
  -DRPK_BASE_BINARY_DIR=<scratch>/base-build \
  -DRPK_OPENGL_BINARY_DIR=<scratch>/opengl-build \
  -DRPK_RENDERPARK_BINARY_DIR=<scratch>/rpk-build \
  -DCMAKE_CXX_FLAGS="-g -fno-omit-frame-pointer"
cmake --build <scratch>/superbuild -j

# Sampling profile (6 iterations ≈ 97 % of the 21-iteration cost):
LD_LIBRARY_PATH=<scratch>/base-build:<scratch>/opengl-build \
perf record -F 997 -g --call-graph fp -o corridor.perf.data -- \
  <scratch>/rpk-build/rpk ../etc/corridor.mgf -raytracing-method none \
  -nqcdivs 18 -iterations 6 -radiance-method Galerkin \
  -eyepoint -3.66 -5.52 7.2 -center 0.2 3.47 5.11 -dont-force-onesided
perf report --stdio --no-children -g none | c++filt

# Exact call counts (2 iterations, slow — use for call-count questions):
valgrind --tool=callgrind ... -iterations 2 ...
```

Note on gprof: the binaries would need `-pg`, and gprof does not attribute samples or
call arcs inside shared libraries — virtually all Galerkin code lives in
`libvitral.so`, so `scripts/makeCallGraph.sh` produces an empty-looking graph unless
`vitral` is built statically. perf + callgrind avoid the problem entirely.
