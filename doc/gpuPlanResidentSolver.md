# GPU plan, alternative C: solver resident on the GPU

Read `gpuPlan.md` first. This alternative reuses the traversal shaders of `gpuPlanVisibilityOffload.md` and the kernel and oracle shaders of `gpuPlanFormFactorKernel.md`, and replaces everything that those leave on the CPU.

## Idea

The hierarchy and the links live in device buffers for the whole solve. The CPU builds the initial tables, issues a fixed sequence of dispatches per iteration and reads back the final radiance. Nothing crosses the bus per wave, and no part of the iteration is serial.

This is a second solver, not a backend of the existing one. The CPU solver remains the reference and the fallback.

## Data layout

Elements, as parallel arrays indexed by element number:

| Array | Content |
| --- | --- |
| `parent`, `firstChild`, `childCount` | Topology. The four regular children of a surface element are consecutive; the irregular children of a cluster are a range of a child index array |
| `kind`, `face`, `childNumber`, `level` | Cluster or surface, top-level face, position in the parent, depth |
| `upTransform` | The 2D affine transform to the top-level face (`Element::topTransform`), composed at subdivision |
| `area`, `reflectivity`, `emittance` | Constants of the element |
| `radiance`, `received` | Coefficients per basis function |
| `potential`, `receivedPotential` | Only for importance-driven runs |
| `maxRadiance` | Per cluster, the table of step G1 |

Links: `receiver`, `source`, `K` (nine floats for the linear basis), `deltaK`, `visibility`. Soda needs about 1.1 GB for 22 M links; the capacity is fixed in advance and the solve stops with an error when it is exceeded.

## One iteration

1. **Oracle over all links** (wave 0). One dispatch; each link gets an evaluation code.
2. **Compaction.** A prefix sum separates accurate links, which stay, from links to subdivide, which leave the link array and become the first set of subdivision requests.
3. **Subdivision.** Several links may ask for the subdivision of the same element in the same wave. Mark the element with an atomic flag, compact the marked elements, allocate four children for each from a bump counter, fill them (geometry from the parent transform, radiance by the push filter, which reads only frozen parent data), then let each request look up the children.
4. **Candidate emission.** Each request emits its children pairs, with the facing tests of the cluster cases, by a prefix sum over child counts.
5. **Form factors and oracle** for the candidates, as in alternative B stage B2, with node positions computed in the shader from `face` and `upTransform`.
6. **Compaction** again: candidates with zero visibility disappear, accurate ones are appended to the link array, the rest are the requests of the next wave. Repeat from step 3 until no requests remain.
7. **Gather.** Group links by receiver with a counting sort (count per receiver, prefix sum, scatter), then one thread per receiver adds `K · radiance[source]` over its links. No two threads write the same element, so no atomics are needed. Source-side effects (`basisUsed`, received potential) use the same scheme grouped by source.
8. **Push-pull.** Top-down by level: push received radiance to the children. At the leaves: multiply by reflectivity and add emittance. Bottom-up by level: pull the children's radiance. About twice the hierarchy depth in dispatches.
9. **Cluster maxima** for the next iteration, bottom-up.

Initial state: the cluster hierarchy and top-level surface elements come from the CPU (`ClusterCreationStrategy`), and the link array starts with the single self-link of the top cluster (`LinkingClusteredStrategy::createInitialLinks`).

## Results and gates

Under Jacobi the set of links an iteration produces does not depend on the order in which candidates are processed, because every decision reads frozen data. What changes relative to the serial solver:

- **Element numbering.** Children are numbered by the bump counter, not in serial order. Ids are visible only in warnings and debug output.
- **Order of sums** in the gather, hence the last bits of received radiance, hence, rarely, an oracle decision in a later iteration.

The gate is therefore: structural fingerprint (elements, clusters and links by type per iteration) equal to the CPU's in the first iteration and recorded afterwards, and T3 bounded for the image. With the software traversal and 64-bit kernel arithmetic the differences should be at rounding level; the bound is set from measurement and is an owner decision.

A serial-order gather is possible (give each link a key that encodes its path in the serial recursion, sort each receiver's links by it) and would restore identical sums at the price of a sort per iteration. Decide after measuring how far the plain version drifts.

## API

Steps 2, 3, 4, 6 and 7 are prefix sums, compactions and a counting sort. In Vulkan each is a hand-written compute shader with its own tests. In CUDA they are library calls (Thrust, CUB), and OptiX supplies the rays. This is the one alternative for which CUDA would save substantial work, against a vendor-specific dependency, a toolkit that is not installed on the development machine, and no reuse of VITRAL's Vulkan direction. If the project stays on Vulkan, budget the primitives as a small library in VITRAL with CPU reference implementations.

## Cost model

Estimates. Per iteration of soda at full size: 22 M oracle evaluations, a gather over 22 M links and a push-pull over 320 k elements are tens of milliseconds each on the RTX 3060. Ray time is that of alternative A: 1.5 to 5 s for the whole run with ray queries. Estimated soda total: 3 to 10 s, against 102 s for parallel Jacobi with 72 threads and the 10 to 25 s estimated for alternative B.

Device memory for soda: about 1.1 GB of links, under 100 MB of elements, the acceleration structure, and the transient buffers of the largest wave, which must be chunked as in alternative A.

## Cost of having it

| Item | Weight |
| --- | --- |
| A second implementation of refinement, subdivision, gathering and push-pull | Every later change to the solver is made and verified twice |
| Outside the portable subset | No counterpart in Java, TypeScript or Turbo C++; rule 6 of `vitralAlignmentPlan.md` needs an explicit exception |
| Options not covered | Non-isotropic clustering, shaft culling, multi-resolution visibility, Gauss-Seidel and Southwell stay on the CPU |
| Debugging | Device-side allocation and compaction errors are harder to localise than CPU ones; each primitive needs a CPU twin and a comparison test |

## Steps

1. Alternatives A and B complete, including stage B2.
2. Element and link arrays as a CPU data structure first: a flat-array wavefront solver on the CPU whose structural fingerprint matches the existing one. This separates the change of data layout from the change of device.
3. Compaction, prefix sum and counting sort primitives with CPU reference implementations and comparison tests.
4. Oracle and gather on the device with links resident; subdivision still on the CPU. Measure: this alone removes the cost of late iterations.
5. Subdivision and candidate emission on the device.
6. Push-pull and cluster maxima on the device.
7. T3 bounded on the corpus; T4; record the figures in this file.

Step 4 is a natural stopping point: it covers the iterations that alternative B cannot speed up and leaves the hardest part, device-side subdivision, undone.

## Risks

| Risk | Consequence | Response |
| --- | --- | --- |
| The two solvers drift apart | Results differ for reasons no gate explains | Structural fingerprint comparison in the test corpus for every commit that touches either |
| Link capacity exceeded | Solve aborts | Capacity from a dry estimate or a growth step between iterations; clear error message |
| Sum-order drift larger than expected | Bound hard to set | Serial-order gather key, at the cost of a sort |
| Effort far above A and B | Delays other plan work | Enter only on measured evidence that B's CPU remainder dominates |
