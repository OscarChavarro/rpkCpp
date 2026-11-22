# GPU plan for the Galerkin Jacobi solver

This document analyses how feasible it is to run the Galerkin radiosity solver, in its Jacobi form, on a GPU, and lists the alternatives for doing so. It covers only the radiosity computation: scene loading (`.mgf`, `.bin`), exports (`.bin`, `.wrl`) and image output are serial, cheap and out of scope.

The three alternatives each have their own file:

| Alternative | File | What moves to the GPU |
| --- | --- | --- |
| A | `gpuPlanVisibilityOffload.md` | Shadow rays only |
| B | `gpuPlanFormFactorKernel.md` | Shadow rays, kernel, form factors and, optionally, the refinement oracle |
| C | `gpuPlanResidentSolver.md` | The whole iteration: hierarchy, links, refinement, gathering and push-pull |

They are cumulative. A needs the groundwork of section 5, B is built on A, and C is built on B, so the alternatives are also stages and the real decision is where to stop.

Sections 1 to 3 describe the code as it stands at `a952f719`. The profile in section 2 was measured for this document. Every GPU throughput figure and every projected time is an estimate, marked as such, and step G0 exists to replace them with measurements.

## 1. What the solver computes

One Jacobi iteration (`GatheringClusteredStrategy::doGatheringIteration`) has three parts:

1. **Refinement and form factors** (`HierarchicalRefinementStrategy`). Every link is tested by the oracle (`hierarchicRefinementEvaluateInteraction`). A link that is not accurate enough is replaced by links between sub-elements; each new candidate link gets a form factor (`FormFactorStrategy::computeAreaToAreaFormFactorVisibility`) and is tested in turn, recursively and depth first.
2. **Light transport** (`receiverLightTransport`, `sourceLightTransport`): for each accurate link, `received[receiver] += K · radiance[source]`.
3. **Push-pull** (`GalerkinBasis::pushPullRadiance`): one traversal of the element hierarchy that turns received radiance into the radiance of the next iteration.

The form factor of a candidate link is where the rays are. For a receiver rule of R nodes and a source rule of S nodes (7 × 6 = 42 pairs with the default quadrilateral rules) the kernel `Gxy[r][s]` is evaluated for each pair (`evaluatePointsPairKernel`): two cosines, a squared distance and one any-hit shadow ray from `y[s]` to `x[r]`. The link coefficients `K`, the error estimate `deltaK` and the 0–255 `visibility` follow from that R × S matrix with a few dozen floating point operations.

With the default options (isotropic clustering, no shaft culling) every shadow ray is a query on the whole scene (`SceneQuery::intersect`, front faces only, any hit). A link known to be fully visible (`exactVisibility` and `visibility == 255`) passes no occluder list to its children, which then trace no rays.

Serial Jacobi and parallel Jacobi give bit-identical results. Parallel Jacobi (`refineInteractionsInParallel`) relies on the property that makes a GPU version possible at all: **during a Jacobi iteration, refinement reads only state frozen at the start of the iteration** (radiance, potential, reflectivity, thresholds). Workers refine independently and record events (`SUBDIVISION`, `LINK`, `NODES_TOO_CLOSE`, `FORK`); a serial commit then replays the events in the order of the serial algorithm, so element ids, link lists and every floating point sum are the serial ones. It falls back to serial refinement for non-isotropic clustering, shaft culling, multi-resolution visibility and ray capture.

## 2. Where the time goes

Corridor scene, 21 iterations, serial Jacobi, WALD2007 scene acceleration, `perf record` at 499 Hz, self cost grouped by subsystem (2026-11-22):

| Share | Subsystem | Main symbols |
| --- | --- | --- |
| ≈ 77 % | Shadow ray visibility | `traverseNodes<StoppingVisitor>` 42.0 %, `PlanarFace::rayHit` 9.6 %, `Patch::intersect` 8.4 %, `quadrilateralParameters` 4.3 %, `SceneFaceRayTest::test` 3.8 %, `QueryVisitor::visit` 3.2 %, `RpkRayHit()` 2.1 % |
| ≈ 11 % | Refinement oracle | `traverseAllLeafElements` 5.1 % and `MaximumRadianceVisitor::visit` 4.0 % (both from `ClusterTraversalStrategy::maxRadiance`), `hierarchicRefinementEvaluateInteraction` 0.9 % |
| ≈ 5.5 % | Kernel and form factor arithmetic | `evaluatePointsPairKernel` 3.1 %, `computeAreaToAreaFormFactorVisibility` 0.9 %, `computeInteractionFormFactor` 0.7 % |
| ≈ 1 % | Light transport | `receiverLightTransport`, `isotropicGatherRadiance` |
| ≈ 0.1 % | Push-pull | `pushPullRadianceRecursive` |
| ≈ 5 % | Recursion, lists, allocation, unresolved symbols | |

Problem sizes, from the reference fingerprints in `cpp/etc/goldenImagesForTests/fingerprints`:

| Scene | Elements | Links | Shadow rays (shadow cache hits) | Iterations |
| --- | --- | --- | --- | --- |
| 02 corridor | — | 1.9 M surface-to-surface | 237 M (49 M) | 21 |
| 08 soda | 320 k (289 k surface, 31 k clusters) | 21.9 M | 1 528 M (574 M) | 11 |

Soda starts with 120 910 surface patches, so its ray tracing geometry is under 250 000 triangles. Its rays are concentrated in the first iterations: 221 M, 778 M, 277 M, 151 M and 68 M in iterations 1 to 5, and under 2 % of the total afterwards. Iterations 6 to 11 are almost entirely oracle tests and transport over 22 M existing links.

Amdahl bounds for one thread, from the shares above:

| Moved off the CPU | Remaining share | Bound on speed-up |
| --- | --- | --- |
| Shadow rays | 23 % | 4.4 × |
| Rays and kernel arithmetic | 17 % | 5.9 × |
| The above, with `maxRadiance` computed once per cluster and iteration | 8 % | ≈ 13 × |
| Everything | — | Limited by the GPU |

Two things follow. Visibility is the only part worth offloading on its own. And once it is offloaded, the oracle's `maxRadiance` (a traversal of every leaf of the source cluster for each oracle call) becomes the largest cost; radiance is frozen during a Jacobi iteration, so one value per cluster and iteration gives the same result. That is a CPU change, independent of any GPU work, and worth about 9 % of today's serial time.

## 3. Feasibility

The measured reference is soda: 1206 s serial and 102 s with 72 threads on the development machine (2 sockets, 72 logical CPUs), where scaling is limited by memory. That machine has an RTX 3060 (12 GB) with Vulkan 1.4, `VK_KHR_acceleration_structure`, `VK_KHR_ray_query`, `VK_KHR_ray_tracing_pipeline`, `shaderFloat64`, and `glslc` installed. The CUDA toolkit is not installed (`/usr/local/cuda` holds only documentation, and there is no `nvcc`).

| Part | Fit for a GPU | Why |
| --- | --- | --- |
| Shadow rays | Very good | Independent any-hit queries on a static scene of at most a few hundred thousand triangles; this is the workload RTX hardware is built for |
| Kernel and form factors | Good | Small fixed-size dense arithmetic per candidate link, no data dependence between links |
| Oracle | Good | A few comparisons per link over frozen data, once the per-cluster maxima are a table |
| Light transport | Good | A sparse matrix–vector product; needs links grouped by receiver to avoid write conflicts |
| Push-pull | Fair | A tree traversal; must be done level by level; cheap anyway |
| Refinement control and subdivision | Poor as written | Recursive, depth first, allocates elements and links, and each decision needs the form factor of the candidate before the next can be made |

The last row is the obstacle. As written, the solver asks for 42 rays, waits for the answer, decides, and recurses. A GPU needs millions of rays per call. **The solver has to be restructured from depth-first recursion into breadth-first waves** before any GPU can help it (section 5, step G2). The frozen-state property and the event-and-commit design of parallel Jacobi make that restructuring legal and supply most of the machinery: a wave is the current design with every sub-interaction forked.

Other obstacles, in order of weight:

1. **Bit-identical results are not available from hardware ray tracing.** The solver's exact face test is `PlanarFace::rayHit`: quadrilaterals are tested through their bilinear parameters, with RenderPark's tolerances, partly in double precision. RTX hardware tests triangles in single precision with its own watertight rule. Rays that graze an edge or end near a surface will occasionally get a different answer, so some links get a different `visibility` and a few refinement decisions change. The AE = 0 gate (T1) cannot hold on that path; it needs a bounded-difference gate (section 6). A software traversal on the GPU with a port of the exact face test can be bit-identical (section 4).
2. **Exclusions.** Each query excludes up to four faces (receiver, source and their twins) and up to two cluster geometries. Hardware any-hit queries must therefore inspect candidates instead of terminating on the first one: a ray query loop that rejects a candidate by face index or by membership in a cluster. Cluster membership becomes an interval test when faces are numbered by an Euler tour of the cluster hierarchy.
3. **Statistics.** The shadow cache (`ShadowCache`) and the last-pair cache of `FormFactorWorkspace` have no purpose on a GPU. Neither changes a visibility answer, but both change the counters "shadow rays" and "shadow cache hits", which are part of the T1 fingerprint. The fingerprint must be split into a structural part (elements, clusters, links by type) that stays identical and ray counters that are specific to the backend.
4. **Double precision.** Form factors are accumulated in `double` and stored as `float`. The RTX 3060 supports 64-bit floats in shaders, at a fraction of its single precision speed; the kernel is small enough for that not to matter. WebGPU has no 64-bit floats.
5. **Memory.** 22 M links at about 52 bytes (two indices, nine coefficients for the linear basis, `deltaK`, visibility) are 1.1 GB, and a wave of 4 M candidates is 168 M rays. Rays must be generated on the GPU from compact candidate records, and waves must be cut into chunks. This fits in 12 GB with room to spare.
6. **Ports and the portable subset.** Rule 6 of `vitralAlignmentPlan.md` asks for a counterpart in Java, TypeScript and Turbo C++. Turbo C++ cannot have one and stays serial, as it did for parallel Jacobi. Java can reach Vulkan through bindings. TypeScript has WebGPU, which has compute shaders but neither ray queries nor 64-bit floats. A GPU backend is therefore an optional component behind an interface, and the CPU solver stays the reference in every port.

Verdict: feasible, with a certain gain for visibility and a restructuring cost that is paid once and also benefits the CPU solver. The gain on the 72-thread machine is moderate for A (estimated 3 to 4 times over parallel Jacobi) because the CPU already does well there; on an ordinary desktop of 8 cores the same GPU would give an estimated 5 to 10 times. Only C changes the order of magnitude, at the price of a second solver.

## 4. API choice

The choice of API is independent of the choice of alternative.

| | Vulkan, ray query in a compute shader | Vulkan, software traversal in a compute shader | CUDA with OptiX |
| --- | --- | --- | --- |
| Ray traversal | Hardware (RT cores) | Shader code over VITRAL's flat tree (`flatBinaryTreeContract.md`) | Hardware (RT cores) |
| Estimated any-hit throughput on the RTX 3060 | 0.3 – 1 G rays/s | 50 – 200 M rays/s | 0.3 – 1 G rays/s |
| Exact face test of RenderPark | No | Yes, ported to GLSL | No (yes with a custom primitive program, at software speed) |
| Bit-identical visibility | No | In practice, not by specification (below) | No |
| Vendors | Any GPU with `VK_KHR_ray_query` | Any Vulkan 1.1 GPU, and `llvmpipe` for tests without a GPU | NVIDIA only |
| Scan, sort and compaction for B and C | Hand-written compute shaders | Hand-written compute shaders | Thrust and CUB |
| Present on the development machine | Yes | Yes | No (toolkit to be installed) |
| Fit with VITRAL | Its contract document already names a Vulkan 1.4 compute path | Same, and reuses `BinnedSahHierarchy` output unchanged | New dependency |
| Path for the TypeScript port | None (WebGPU has no ray query) | Yes, in single precision | None |

On bit-identity of the software path: Vulkan requires correctly rounded addition, subtraction and multiplication when contraction is disabled (`precise` in GLSL), but allows 2.5 ULP of error in division and inherits it for square roots. `PlanarFace::rayHit` divides (`planeHitParameter`) and takes a square root (`solveQuadraticUnitInterval`). NVIDIA hardware rounds both correctly in practice, so the result is expected to match and must be verified with the differential harness; it is not guaranteed. CUDA does guarantee IEEE 754 for those operations with `-fmad=false`.

Note: `flatBinaryTreeContract.md` describes `VulkanComputeAccelerationStructure` and `computeAccelerationStructure.glsl`, but no Vulkan source or shader was found in the VITRAL checkout at `/home/jedilink/VITRAL/vitral`. Whether that code exists elsewhere decides how much of the software traversal has to be written.

**Recommendation: Vulkan, with both traversals behind one interface.** The software traversal is the reference backend (exact, testable anywhere, the only one a browser port could follow); the ray query traversal is the fast backend. CUDA becomes attractive only for alternative C, where Thrust saves real work, and it would be the first vendor-specific dependency of the project.

## 5. Common groundwork

These steps precede every alternative. G1 and G2 change no result and must pass T1 at 1, 2 and N threads.

**G0. Measure before building.** The capture build (`WITH_RAY_CAPTURE`, `scripts/captureRays.sh`) already records the solver's visibility queries with their answers. Write a standalone console program that loads a captured corpus and the scene's faces, traces the corpus on the GPU with each traversal, and reports rays per second and the number of answers that differ from the CPU. This gives the two numbers the rest of the plan depends on, throughput and disagreement rate, before the solver is touched. Stop and report if hardware disagreement exceeds what the gate of section 6 can absorb.

**G1. One `maxRadiance` per cluster and iteration.** Compute the table once after push-pull and read it in `hierarchicRefinementApproximationError`. Same values, so T1 holds. It removes the cost that would otherwise dominate after offloading, and B needs the table anyway.

**G2. Wavefront refinement on the CPU.** Replace the depth-first recursion of the parallel workers by level-synchronous waves:

- Wave 0: the oracle is evaluated for every existing link. No rays. Links to subdivide emit candidate records (receiver element, source element, parent task, child ordinal, "parent fully visible" flag).
- Wave k: for all candidates at once, (1) visibility of every node pair, (2) kernel, `K`, `deltaK` and `visibility`, (3) candidates with `visibility == 0` are dropped, (4) the oracle decides: accurate candidates become `LINK` events, the others subdivide and emit the candidates of wave k + 1.
- Commit: unchanged in principle. Each candidate is a forked task, and `commitRefinementTask` already replays a tree of `FORK` events in serial order, which keeps element ids, link list order and sum order.

Step (1) goes through a new interface, a batched visibility query: given node tables and candidate records, return one bit per node pair. Its first implementation is the CPU one, looping `SceneQuery::intersect` over the worker threads. The interface and that implementation are generic and belong in VITRAL, next to `SpatialIndex` (ground rule on thinning the RPK layer; VITRAL first, then `vitralSubset.sh --refresh`).

Cubature node positions are computed on the CPU, once per distinct element of a wave, and passed as a table, so ray end points are the same bits on every backend.

Expect a small CPU cost from the lost shadow cache and last-pair cache and from the larger working set; measure it with T4 and report it under the stop-and-ask rule, stating that it buys the GPU path.

**G3. Flat scene tables.** A face table for the GPU: vertices, plane, twin index, one- or two-sidedness, and the Euler tour interval of each cluster for the exclusion test. Built once per scene, read-only.

## 6. Verification

| Gate | Applies to | Criterion |
| --- | --- | --- |
| T1 as today | G1, G2 with the CPU batch backend | AE = 0 and identical fingerprint at 1, 2 and N threads |
| T1 structural | Software traversal backend (A and B) | AE = 0; identical elements, clusters and links by type; ray counters recorded separately |
| Ray agreement | Every GPU traversal, on the captured corpora | Software traversal: zero differing answers. Ray query: rate recorded per scene, bound set by the owner |
| T3 bounded | Ray query backend, and alternative C | RMSE against the golden image within a bound recorded per scene in `goldenBounds.txt`; difference image kept on failure. A new bound is an owner decision |
| T4 | Every step | Wall clock, GPU time, peak host and device memory for corridor and soda against parallel Jacobi at N threads |

The GPU path is an option beside the existing ones (for instance `-gr-visibility-backend cpu|vulkan-software|vulkan-rayquery`), never a replacement, and it falls back to the CPU in the cases where parallel Jacobi falls back to serial today.

## 7. Comparison and recommendation

Projected soda times are estimates built from the measured 1206 s and 102 s, the corridor shares of section 2 (soda's own profile has not been taken) and the throughput ranges of section 4.

| | A: visibility offload | B: form factor kernel | C: resident solver |
| --- | --- | --- | --- |
| On the GPU | Rays | Rays, kernel, `K`, optionally oracle | Everything in the iteration |
| On the CPU | Hierarchy, oracle, kernel, commit, transport, push-pull | Hierarchy, subdivision, commit, transport, push-pull | Scene set-up and read-back |
| Bit-identical to serial Jacobi | Yes, with the software traversal | Depends on double precision division in shaders; verify | No (sum order and ids differ); structural fingerprint can still match |
| New code | Small: one interface, one backend, shaders for traversal | Medium: kernel and oracle shaders, radiance tables | Large: a second solver, with scan, sort and allocation on the GPU |
| Share of today's serial time removed | ≈ 77 % | ≈ 83 %, ≈ 92 % with the oracle | ≈ 100 % |
| Estimated soda time (72 threads + RTX 3060) | 25 – 40 s | 10 – 25 s | 3 – 10 s |
| Usable by the other ports | Interface yes; backend Java only | Same | No |
| Main risk | Disagreement rate of hardware traversal | Double precision in shaders | Effort, and two solvers to keep in agreement |

Recommended order:

1. G0, G1, G2, G3. G1 and G2 are worth having without any GPU.
2. Alternative A with the software traversal, gated by T1 structural. This proves the pipeline with an exact result.
3. Alternative A with ray queries, gated by ray agreement and T3 bounded. Measure.
4. Decide on B from the measured remainder: if the CPU share left after step 3 is dominated by kernel arithmetic and mask handling, B is a contained addition.
5. Consider C only if, after B, the CPU remainder still dominates and the project accepts a second solver. Iterations 6 to 11 of soda, which are pure oracle and transport over 22 M links, are where C would show its gain.

## 8. Decisions for the owner

1. Accept a bounded-difference gate (T3 bounded) for the hardware ray tracing path, or require the software traversal only.
2. Split the T1 fingerprint into a structural part and backend-specific ray counters.
3. Vulkan as the API, with CUDA reconsidered only for C.
4. Place the batched visibility query and its Vulkan backends in VITRAL.
5. Whether G1 and G2 enter `vitralAlignmentPlan.md` as numbered steps or form a new plan.
