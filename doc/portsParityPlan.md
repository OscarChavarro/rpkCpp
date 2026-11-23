# Port parity results and improvement plan

This plan records the image comparison performed on 2026-11-23 and the next
steps for Java, TypeScript and Turbo C++. It complements
[the VITRAL alignment plan](vitralAlignmentPlan.md); its source synchronization,
performance and reference-image rules continue to apply.

## Comparison with C++

The reference is the C++ output image for the same corpus scene. Each numeric
entry is the number of pixels that differ (ImageMagick AE); zero means exact
image equality. These are comparisons with the available C++ outputs, not
comparisons with the golden images or a claim that the ports are fully aligned.
The C++ and Java outputs were produced with the parallel corpus configured for
72 threads. Turbo C++ used its serial renderer. The working trees contained
uncommitted port changes during the comparison; the RPK and VITRAL checkout
HEADs were `733579d0` and `a71b1d27`, respectively. Those revisions alone do
not reproduce the uncommitted working trees.

| Corpus image | Java | TypeScript | Turbo C++ |
| --- | ---: | ---: | ---: |
| 01 cube Jacobi | 0 | 104 | 39 |
| 01 cube Gauss-Seidel | 0 | 104 † | 39 |
| 01 cube Southwell | 1 | 76 † | 31 |
| 02 corridor | 2 | OOM | 145 |
| 03 hospital | 0 | OOM | 168 |
| 04 office 1 | 0 | OOM | 261 |
| 05 office 2 | 0 | 582 | 132 |
| 06 office 3 | 0 | OOM | 122 |
| 07 classroom | 0 | OOM | 229 |
| 08 soda | 0 | OOM | Unavailable |
| 10 floor ray matting | 0 | 0 † | 3,453 |
| 11 floor ray casting | 0 | OOM | 98 |
| 12 floor bidirectional path tracing | 11 | 606 † | 69 |
| 13 floor stochastic Random Walk | 6 | No parallel route | 1,139,011 |
| 14 floor stochastic Jacobi | 1 | No parallel route | 41 |
| 15 cube Jacobi importance | 1 | 106 | 1,276,673 |
| 17 floor stochastic Jacobi importance | 3 | No parallel route | 30 |
| 21 tone mapping Ward | 0 | OOM | 99 |
| 22 tone mapping Tumblin-Rushmeier | 0 | OOM | 142 |

**OOM** means Node failed with a JavaScript heap allocation error during a
72-thread run, including isolated retries, and did not produce a valid final
image. **No parallel route** means the
TypeScript wrapper does not provide the requested parallel execution for that
method; attempts on 14 and 17 were stopped and their partial outputs were not
compared. **Unavailable** means soda is excluded from the 32-bit Turbo corpus:
the scene exceeds that process's address space.

† These TypeScript cases were inadvertently run serially because the parallel
wrapper only substitutes parallel Jacobi for supported Galerkin runs. Their
measurements are recorded for completeness; they do not validate a parallel
run. Future runs must explicitly verify which execution mode the wrapper selects.
Turbo's serial renderer was expressly permitted for this comparison. Independent
Turbo scenes were also run in separate processes; a redundant classroom process
was stopped after a separate serial run had completed. The Turbo corpus driver
itself therefore did not complete as one uninterrupted run.

The largest normalized RMSE values against the C++ output were:

| Comparison | Normalized RMSE | Interpretation |
| --- | ---: | --- |
| Turbo 15 cube Jacobi importance | 0.422904 | Major difference; investigate first. |
| Turbo 13 Random Walk | 0.0107872 | Major difference; compare matching serial execution modes and random streams before attributing it to a defect. |
| Turbo 10 ray matting | 0.00968248 | Investigate the differing ray and color paths. |
| Turbo 11 ray casting | 0.000920389 | Fewer pixels differ, but some differ substantially. |
| TypeScript 12 bidirectional path tracing † | 0.000632452 | Serial measurement; compare equivalent execution modes. |

Java matches C++ exactly in 12 of 19 images. The other seven differ in one to
11 pixels. For this stage, **accept Java provisionally and defer investigation
of those small differences**. Random sequences and floating-point arithmetic
are plausible contributors, but have not been proven to be the cause. The
parallel Random Walk images from Java and C++ differ in six pixels; this is
separate from their difference against the recorded golden image.

The image gates are not all passing: Java's full parallel corpus finished, but
its Random Walk golden comparison exceeded the recorded bound; several exact
Java comparisons also differ from port references, and references for 15 and
17 are missing. The TypeScript golden bound for 15 is missing. Java emits an
incubator-module build warning, and the Turbo host build emits C++98 warnings.
These facts remain open even though Java is accepted provisionally for this
cross-port comparison. Do not replace golden images or bounds without the
owner's decision.

## Improvement plan

### 1. Make TypeScript runs fit in memory and diagnose the failure

1. Use **fewer than 72 workers** for TypeScript while investigating memory.
   Start with a small count such as four, run scenes individually, and adjust
   the count based on measured peak RSS and available memory. This is a
   deliberate exception to the previous 72-thread testing preference for this
   port. Record the actual worker count, execution mode, elapsed time and peak
   RSS for every result. Do not launch the full corpus with 72 scene replicas
   per parallel run.
2. Reproduce the allocation failure on a representative affected scene, such
   as 11 ray casting or 02 corridor, then compare worker counts and an isolated
   run. Inspect Node/V8 heap limits, per-worker heaps and scene replicas, the
   lifetime of refinement data, allocation and garbage-collection behavior,
   and whether retained objects or an unexpectedly bounded heap cause the
   failure. Distinguish an actual leak or invalid allocation from legitimate
   working-set growth; the current evidence does not establish a root cause.
3. Fix the demonstrated cause while preserving rendering results. Prefer
   immutable shared scene data, compact transferable representations, bounded
   workspaces or reusable buffers where the runtime and ownership rules allow
   them. Measure the memory and performance effects, including per-worker
   costs, and check the impact on hot paths.
4. Run each supported parallel corpus scene at a documented safe worker count
   and compare its completed image against C++ and the existing golden gate.
   Retry soda only when the measured memory budget allows it. Record failures
   and unavailable references explicitly. Repeat a higher worker-count run
   only when measurements show that it fits.
5. Audit the wrapper and the available parallel paths for Random Walk,
   StochJacobi, Gauss-Seidel, Southwell and ray tracing. Report the actual mode
   for each scene; do not call a serial run a parallel test. Add or port a
   parallel implementation only where it is part of the intended design, and
   compare equivalent algorithms and random-stream behavior.

### 2. Investigate Turbo C++ differences with serial tests

1. Start with **15 cube Jacobi importance**. Reproduce the difference using
   serial Turbo and an equivalent serial C++ reference, then inspect typed
   visibility targets, face IDs, the `SoftIds` export, direct potential and
   importance accumulation. Compare intermediate visibility and potential
   buffers as well as the final image to locate the first divergence.
2. Investigate **13 Random Walk** by comparing serial Turbo with serial C++
   using equivalent seeds, random generators, path counts and render options.
   Determine how much of the current difference comes from serial versus
   parallel random streams before changing rendering logic.
3. Investigate **10 ray matting** and **11 ray casting** using matching serial
   runs. Compare ray hits, shading and output conversion at representative
   differing pixels; use the AE and RMSE together to prioritize the cases.
4. Check the remaining smaller differences after the major ones. Preserve Turbo
   C++ 3.0 compatibility, 8.3 mappings, and the no-`friend` rule. Record soda
   as unsupported by the current 32-bit build unless a suitable build and
   execution environment are established; do not treat it as a pass.

### 3. Finish cross-port alignment and verification

1. Complete the remaining C++ design counterparts in the ports, especially
   typed SGL visibility buffers and scene-owned face state, including direct
   potential and display colors where the port supports them. Verify that the
   already ported typed `KDTree`, `ScreenIterate`, option parser and constructors
   preserve their contracts in each runtime. Keep port-specific limitations
   explicit rather than calling partial implementations 1:1 parity.
2. For VITRAL subset changes, update the matching VITRAL port first, refresh
   each RenderPark subset, and run `vitralSubset.sh --check`. Run the relevant
   VITRAL and port contract tests. Do not introduce `friend` declarations.
3. After each slice, build and report warnings, run the applicable corpus in
   its documented execution mode, and compare images both against C++ and the
   recorded golden bounds. Investigate exact-reference failures as required by
   the governing plan. Keep missing bounds and references visible; do not
   silently regenerate them. Measure affected hot paths and explain any
   material performance or memory change.
4. Defer Java's small image differences for this stage. Revisit them after the
   TypeScript memory and major Turbo discrepancies are understood, retaining
   the provisional acceptance and the outstanding build and golden-gate facts
   above until independently resolved.
