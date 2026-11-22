# GPU plan, alternative A: visibility offload

Read `gpuPlan.md` first: it holds the analysis, the groundwork steps G0 to G3 that this alternative assumes, the gates and the comparison with alternatives B and C.

## Idea

The CPU keeps the whole solver: element hierarchy, oracle, kernel arithmetic, subdivision, commit, light transport and push-pull. The GPU answers one question in bulk: for these candidate links, which node pairs see each other. That is the 77 % of the serial time spent in shadow rays, and it is the part for which RTX hardware exists.

After step G2 the solver already asks that question once per wave through the batched visibility query. Alternative A is a second and a third implementation of that interface.

## Data

Built once per scene (step G3) and kept on the device:

| Buffer | Content | Size for soda |
| --- | --- | --- |
| Acceleration structure | Ray query backend: one BLAS of all faces (a quadrilateral as two triangles sharing its face index) and a TLAS with a single instance. Software backend: `nodes` and `order` of `BinnedSahHierarchy`, converted as `flatBinaryTreeContract.md` prescribes | < 250 k triangles, tens of MB |
| Face table | Software backend: the data `PlanarFace::rayHit` reads (plane, vertices, bilinear terms). Both: twin index and cluster tour index | 121 k faces |
| Cluster table | Euler tour interval `[enter, exit)` of each cluster | 31 k clusters |

Uploaded per wave, in chunks sized to a fixed device budget:

| Buffer | Record | Bytes |
| --- | --- | --- |
| Node table | Positions of the cubature nodes of one element in one role, computed on the CPU | 12 per node |
| Candidates | Receiver node slot and count, source node slot and count, four excluded face indices, two excluded cluster indices | 32 |

Read back per wave: one bit per node pair, 64 bits per candidate for the default rules (42 pairs). The size follows the configured cubature degrees, up to 400 bits for the largest rules.

## Shader

One compute invocation per candidate, looping over its node pairs, or one per node pair when candidates are too few to fill the device. For each pair:

1. `direction = x − y`, `distance = |direction|`. Skip when `distance < EPSILON` (the CPU reports the "nodes too close" warning from the same test).
2. Trace from `y` towards `x` over `[EPSILON_FLOAT · d, d · (1 − EPSILON)]`, front faces only, any hit.
3. Reject a candidate hit whose face is one of the four excluded faces or whose tour index lies inside one of the two excluded cluster intervals.
4. Set the bit when nothing blocks.

The cosine tests of `evaluatePointsPairKernel` stay on the CPU, which ignores the bit of a pair whose cosines are not positive. Skipping those rays on the GPU as well is an optimisation that needs the two normals in the candidate record.

Ray query backend: `rayQueryInitializeEXT` with `gl_RayFlagsCullBackFacingTrianglesEXT`, geometry flagged non-opaque so that each candidate hit is inspected, `rayQueryProceedEXT` in a loop, `rayQueryConfirmIntersectionEXT` on the first candidate that is not excluded, then terminate. Winding must match RenderPark's front side, and a two-sided surface must be present as it is in the scene (twin patches, option `-dont-force-onesided`).

Software backend: the any-hit traversal of the flat tree with a GLSL port of `PlanarFace::rayHit`, every expression `precise`, 64-bit floats where the C++ uses `double`.

## What stays the same

Refinement decisions, element ids, link lists and sums are produced by the same CPU code from the same commit. With the software backend the visibility bits are expected to equal the CPU's, so the output is bit-identical to serial Jacobi and the gate is T1 structural. With the ray query backend a small fraction of bits differ, and the gate is ray agreement plus T3 bounded.

## Cost model

All figures are estimates until G0 replaces them.

- Rays: soda traces 1 528 M rays in total without a shadow cache. At 0.3 to 1 G rays per second that is 1.5 to 5 s for the ray query backend, and 8 to 30 s at 50 to 200 M rays per second for the software backend.
- Transfer: 32 bytes up and 8 bytes down per candidate, about 36 M candidates in total for soda, so under 2 GB over the whole run. Negligible on PCIe.
- CPU remainder: about 23 % of the serial work (oracle, kernel, bookkeeping, transport), which is about 275 s of single-thread work for soda and parallelises as it does today. Step G1 removes about a third of it.
- Waves: one device round trip per refinement level and chunk, a few tens per iteration.

Estimated soda total on the development machine: 25 to 40 s against 102 s for parallel Jacobi with 72 threads. On a machine with few cores the CPU remainder dominates, and that is the argument for alternative B.

## Steps

1. G0 to G3 from `gpuPlan.md`.
2. Vulkan device set-up as an optional component (`-DWITH_VULKAN`), absent from default builds and from the T0 audit of builds without it.
3. Software traversal backend. Gate: ray agreement with zero differences on the captured corpora, then T1 structural on cube, corridor and soda.
4. Ray query backend. Gate: ray agreement rate recorded, then T3 bounded.
5. T4 for both against parallel Jacobi; record the figures in this file.

## Risks

| Risk | Consequence | Response |
| --- | --- | --- |
| Hardware and exact face tests disagree more than expected | Visible differences in penumbra regions | G0 measures it first. If too high: software backend only, or re-test on the CPU the pairs of links whose bits are mixed and whose rays pass close to an edge |
| Division or square root not correctly rounded on some driver | Software backend not bit-identical there | Differential harness per driver; T1 structural reported as failed, never relaxed silently |
| Inspecting candidates (non-opaque) costs hardware throughput | Lower gain | Use opaque geometry and plain termination for candidates with no cluster exclusion when the measured agreement allows it |
| Waves too large for device memory | Allocation failure | Chunking by a fixed budget from the start |
| The CPU remainder dominates | Gain limited to the estimate above | G1, then alternative B |
