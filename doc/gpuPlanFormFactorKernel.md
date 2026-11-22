# GPU plan, alternative B: form factor kernel on the GPU

Read `gpuPlan.md` first, then `gpuPlanVisibilityOffload.md`: this alternative extends alternative A and reuses its buffers, shaders and gates.

## Idea

In alternative A the GPU returns visibility bits and the CPU turns them into a link: it evaluates the kernel for each node pair, integrates it against the basis functions, estimates the error and runs the oracle. That work is independent per candidate, has a fixed small size and reads only frozen data, so it can follow the rays onto the GPU. The device then returns finished link records, or, in the second stage, only the decision for each candidate.

The CPU keeps what is hard to do on a GPU and cheap on a CPU: the element hierarchy, subdivision, the serial-order commit, light transport and push-pull.

## Stage B1: links computed on the GPU

Per candidate the shader computes what `computeAreaToAreaFormFactorVisibility` computes:

1. `Gxy[r][s] = cosθx · cosθy / (π d²) · visible`, with the cluster convention of 0.25 for a cosine, the two back-facing early exits and the "nodes too close" case.
2. `K[α][β]`, by the two weighted sums of `computeInteractionFormFactor`, or the constant case of `doConstantAreaToAreaFormFactor` (including its `G ≤ 1` clamp).
3. `deltaK`, from `gMin`, `gMax` and the radiance-weighted residual of `computeInteractionError`, and the isotropic cluster override `maximumKernelValue · sourceArea`.
4. `visibility`, from the count of non-zero kernel values, with the `exactVisibility` adjustment from 255 to 254.

The two early cases (overlapping bounding boxes for clusters, an element linked to itself) are decided on the CPU when the candidate is emitted and never reach the device.

Additional device data:

| Buffer | Content | Update |
| --- | --- | --- |
| Element record | Normal, area, cluster flag, basis size | Per wave, for the elements the wave references |
| Rule tables | Weights of each cubature rule and basis function values at its nodes (`φα(u_k, v_k)`), which depend only on rule and basis | Once |
| Source radiance | Radiance coefficients of each source element (needed by `deltaK`) | Per wave, for the sources referenced; the values are frozen for the iteration |

Result per candidate: `K` (nine floats for the linear basis, one for constant), `deltaK`, `visibility` and a flag for "nodes too close": about 48 bytes instead of the 8 of alternative A.

## Stage B2: the oracle on the GPU

`hierarchicRefinementEvaluateInteraction` needs, beside the link, the reflectivity of the receiver, the source radiance or the cluster maximum (the table of step G1), the receiver area, the potential when importance is used, and four thresholds that are fixed for the iteration (`prepareRefinementIterationState`). With those as tables the shader returns the evaluation code per candidate. The CPU then reads back only the candidates that became links, with their coefficients, and a code for the rest; it performs the subdivisions and emits the next wave.

For isotropic clustering the receiver area of a cluster is a per-element value. The other clustering strategies (`ORIENTED`, `Z_VISIBILITY`) need per-link traversals or the scratch renderer and stay on the CPU fallback, as they do for parallel Jacobi.

Wave 0 of each iteration, the oracle over every existing link, becomes one dispatch over a link array if the links are kept in a device buffer between iterations. That is the first piece of alternative C and can be postponed.

## Precision

The C++ accumulates the kernel and the sums in `double` and stores `K` and `deltaK` as `float`. Two options:

- **64-bit floats in the shader** (`shaderFloat64`, present on the RTX 3060). With `precise` on every expression the additions and multiplications are correctly rounded by specification; the division by `π d²` and the square root in `d` are correctly rounded on NVIDIA hardware in practice but not by specification. The result is expected to be bit-identical and is gated by T1 structural; a failure on some driver is reported, not absorbed.
- **32-bit floats.** Faster and portable to WebGPU, but `K` changes in its last bits, a few oracle decisions change with it, and the gate becomes T3 bounded even with exact visibility.

Use 64-bit floats first. The arithmetic per candidate is a few thousand operations, small beside its 42 rays, so the slow double precision of consumer GPUs does not decide the total.

A differential test precedes integration, as G0 does for rays: feed recorded candidates with their CPU results to the shader and compare `K`, `deltaK` and `visibility` bit by bit.

## Cost model

Estimates, from the corridor shares of `gpuPlan.md` section 2.

| | Share of serial time left on the CPU | Estimated soda time, 72 threads + RTX 3060 |
| --- | --- | --- |
| Alternative A | ≈ 23 % (≈ 14 % with G1) | 25 – 40 s |
| B1 | ≈ 17 % (≈ 8 % with G1) | 15 – 30 s |
| B2 | ≈ 6 %: subdivision, commit, transport, push-pull | 10 – 25 s |

The gain over A is modest on the 72-thread machine and larger where cores are few, because what B removes is CPU work that A leaves to the threads. After B2 the serial-order commit and the light transport over all links are what remains, and they are the limit of this alternative: iterations 6 to 11 of soda do little else.

## Steps

1. Alternative A complete, with G1.
2. Differential test program for the kernel shader on recorded candidates.
3. B1 with 64-bit floats. Gate: T1 structural with the software traversal, T3 bounded with ray queries.
4. T4. Stop here if the CPU remainder is already dominated by commit and transport.
5. B2. Same gates.
6. T4; record the figures in this file.

## Risks

| Risk | Consequence | Response |
| --- | --- | --- |
| Division or square root not correctly rounded on some driver | `K` differs in the last bit, T1 structural fails | Differential test per driver; keep A's CPU kernel as the fallback path |
| A second implementation of the form factor mathematics | The two drift apart when one is changed | The differential test runs in the unit test stage; the CPU code remains the definition |
| Larger read-back (48 bytes per candidate) | About 1.7 GB over the soda run | Negligible; B2 reduces it to the candidates that become links |
| Little gain over A on many-core hosts | Effort not repaid there | Decide from the T4 figures of A (step 4 of the order in `gpuPlan.md` section 7) |
