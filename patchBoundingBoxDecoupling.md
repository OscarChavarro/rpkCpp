# Patch Bounding Box Decoupling Plan for the C++ Codebase

## Goal

Decouple `Patch` from direct ownership of `BoundingBox` without losing rendering correctness or runtime efficiency.

The intended design is:

- `Patch` remains the geometric surface primitive.
- Per-patch spatial acceleration data, including `BoundingBox`, moves into a dedicated spatial data store.
- Hot paths still access patch bounds in O(1), with no hash lookup and no repeated recomputation.
- The rendered result of the Galerkin cube Jacobi case remains identical to the current output.

This plan is for the C++ codebase. Do not apply these steps to the Java, Typescript or TurboC ports.

## Non-Negotiable Constraints

- Do not introduce `std::map`, `std::unordered_map`, or pointer-keyed lookup for per-patch bounds in hot paths.
- Do not recompute patch bounding boxes during hierarchy traversal, voxel insertion, shaft culling, ray traversal, or Galerkin refinement.
- Do not remove `Patch` bounding box storage until all direct reads have been replaced and verified.
- After every implementation step, build outside the sandbox, run `scripts/01_runCubeJacobi.sh`, and inspect/compare `output/01_*.ppm`.
- Preserve binary/layout compatibility temporarily if existing scene loading depends on serialized patch bounds.

## Baseline Verification

Before changing code, establish the baseline.

1. Check out the intended C++ repository root.
2. Build outside the sandbox using the project's normal C++ build command.
3. Run:

```bash
scripts/01_runCubeJacobi.sh
```

4. Identify the generated image:

```bash
ls -lt output/01_*.ppm
```

5. Save a reference copy outside generated output, for example:

```bash
mkdir -p /tmp/renderpark_patch_bbox_baseline
cp output/01_*.ppm /tmp/renderpark_patch_bbox_baseline/
```

6. Inspect the image visually. If tooling is available, convert it to PNG for easier viewing:

```bash
convert output/01_*.ppm /tmp/renderpark_patch_bbox_baseline/current.png
```

7. Record metadata for exact comparison:

```bash
sha256sum output/01_*.ppm
```

Acceptance criteria:

- The project builds cleanly.
- `scripts/01_runCubeJacobi.sh` completes successfully.
- `output/01_*.ppm` exists and looks correct.
- The baseline checksum and image copy are preserved for later comparisons.

## Step 1: Inventory Direct Patch Bounding Box Access

Find every direct use of the patch-owned bounding box.

Suggested searches:

```bash
rg -n "patch->.*bounding|patch\\.bounding|Patch.*bounding|boundingBox" .
rg -n "Compute.*Bounding|compute.*Bounding|get.*Bounding|Bounds" .
```

Classify every usage into one of these groups:

- Construction-time computation.
- Cluster hierarchy construction.
- Voxel/grid insertion.
- Shaft culling.
- Ray intersection acceleration.
- Serialization/deserialization.
- Debug/render visualization.
- Tests or examples.

Create a short checklist in the implementation notes before editing code.

Build and run verification after this inventory even if no code changed:

```bash
# outside sandbox
<normal C++ build command>
scripts/01_runCubeJacobi.sh
ls -lt output/01_*.ppm
sha256sum output/01_*.ppm
```

Acceptance criteria:

- No code changed yet.
- Baseline output still exists and still matches the saved reference.

## Step 2: Introduce Explicit Patch Spatial Data

Add a small C++ type dedicated to per-patch spatial data. Keep it simple and cache-friendly.

Suggested shape:

```cpp
struct PatchSpatialData {
    BoundingBox boundingBox;
    MinMaxBox rayIntersectionBox; // optional only if the C++ code already caches this per patch
   bool boundingBoxValid = false;
};
```

Add an owning store, preferably scene-owned or model-owned:

```cpp
class PatchSpatialDataStore {
public:
    int allocateForPatch(Patch& patch);
    PatchSpatialData& get(int index);
    const PatchSpatialData& get(int index) const;
    void computeForPatch(Patch& patch);
    void computeAll(const std::vector<Patch*>& patches);
};
```

Add one integer handle to `Patch`:

```cpp
int spatialDataIndex = -1;
```

At this stage, do not remove the old `Patch::boundingBox`. The new store should be populated from the same bounding box computation path.

Performance rule:

- Access must be array/vector indexed by `patch.spatialDataIndex`.
- The index should be assigned once during scene construction/loading.
- `PatchSpatialDataStore::get()` should be inline or trivial enough for the compiler to optimize.

Build and run:

```bash
# outside sandbox
<normal C++ build command>
scripts/01_runCubeJacobi.sh
ls -lt output/01_*.ppm
sha256sum output/01_*.ppm
```

Compare against baseline:

```bash
cmp output/01_*.ppm /tmp/renderpark_patch_bbox_baseline/$(basename output/01_*.ppm)
```

If exact byte comparison is not stable, use the existing image diff tool if available, or compare visually and record the maximum pixel difference.

Acceptance criteria:

- Build succeeds.
- Cube Jacobi script succeeds.
- Output image is identical or visually indistinguishable with zero/near-zero diff.
- No hot path uses dynamic lookup for bounds.

## Step 3: Add a Patch Bounds Accessor API

Introduce a single accessor for code that needs patch bounds.

Suggested API:

```cpp
inline const BoundingBox& patchBoundingBox(const Patch& patch, const PatchSpatialDataStore& store) {
    return store.get(patch.spatialDataIndex).boundingBox;
}
```

If passing the store everywhere is too invasive, add the accessor to the scene/context object that already owns patches and acceleration structures.

Rules:

- Do not hide a global mutable store behind this API unless the existing architecture already relies on global scene state.
- Do not return by value.
- Do not compute bounds inside the accessor except in debug assertions.
- In debug builds, assert that `spatialDataIndex >= 0` and `boundingBoxValid == true`.

Keep existing `Patch::boundingBox` in place and keep it synchronized for now.

Build and run:

```bash
# outside sandbox
<normal C++ build command>
scripts/01_runCubeJacobi.sh
ls -lt output/01_*.ppm
sha256sum output/01_*.ppm
```

Inspect `output/01_*.ppm` and compare with the baseline.

Acceptance criteria:

- No behavior change.
- Accessor compiles and has at least one non-critical call site migrated.
- No image change.

## Step 4: Migrate Non-Hot Call Sites First

Replace direct `Patch::boundingBox` reads in low-risk areas first.

Suggested order:

1. Debug printing and visualization.
2. Scene statistics or diagnostic code.
3. Serialization writer paths, while still writing the same data.
4. Serialization reader paths, while still filling both old and new storage.

For binary loading:

- If serialized files contain patch bounding boxes, read them into `PatchSpatialDataStore`.
- Also populate the old `Patch::boundingBox` field during this transition.
- Do not change file format yet.

Build and run after each small group:

```bash
# outside sandbox
<normal C++ build command>
scripts/01_runCubeJacobi.sh
ls -lt output/01_*.ppm
sha256sum output/01_*.ppm
```

Inspect and compare `output/01_*.ppm`.

Acceptance criteria:

- Cube Jacobi remains unchanged.
- Serialized input/output compatibility is preserved.
- The old field still exists but fewer call sites read it directly.

## Step 5: Migrate Cluster Hierarchy Construction

Move cluster hierarchy construction away from `Patch::boundingBox`.

Likely affected areas:

- Patch octree/BVH creation.
- Patch-to-child-octant assignment.
- Cluster bounding box enlargement.
- Any conversion from patch clusters to geometry clusters.

Replace patterns like:

```cpp
patch->boundingBox
```

with:

```cpp
patchBoundingBox(*patch, spatialStore)
```

or the equivalent scene/context accessor.

Important:

- The cluster builder must not recompute patch bounds.
- It should receive the spatial store explicitly or via the scene/context.
- Cluster nodes may still own their own aggregate `BoundingBox`.

Build and run:

```bash
# outside sandbox
<normal C++ build command>
scripts/01_runCubeJacobi.sh
ls -lt output/01_*.ppm
sha256sum output/01_*.ppm
```

Inspect and compare `output/01_*.ppm`.

Acceptance criteria:

- Cluster hierarchy builds successfully.
- Galerkin initialization reaches the same number of patches/elements/clusters as before, if those stats are printed.
- Image output matches baseline.

## Step 6: Migrate Voxel/Grid and Ray Acceleration Code

Replace direct patch bounds access in voxel/grid insertion and ray acceleration setup.

Likely affected areas:

- Voxel grid patch insertion.
- Any per-patch ray pretest based on bounding boxes.
- Patch list bounds computation.

Rules:

- Bounds must come from `PatchSpatialDataStore`.
- `MinMaxBox` may be cached in `PatchSpatialData` if the old code cached it on `Patch`.
- If `MinMaxBox` is only derived from `BoundingBox`, update it when spatial data is computed, not during every ray test.

Build and run:

```bash
# outside sandbox
<normal C++ build command>
scripts/01_runCubeJacobi.sh
ls -lt output/01_*.ppm
sha256sum output/01_*.ppm
```

Inspect and compare `output/01_*.ppm`.

Acceptance criteria:

- No performance-sensitive code recomputes patch bounds.
- Ray/voxel acceleration still works.
- Image output matches baseline.

## Step 7: Migrate Shaft Culling

Move shaft culling away from `Patch::boundingBox`.

Likely affected areas:

- Patch omission/candidate tests.
- Bounding box side classification.
- Cluster or patch overlap tests.

Rules:

- Shaft culling may receive scene/context/spatial store as an argument.
- Avoid adding global access unless the C++ code already uses a global Galerkin state for this data.
- Preserve exact logic for plane tests and overlap classification.

Build and run:

```bash
# outside sandbox
<normal C++ build command>
scripts/01_runCubeJacobi.sh
ls -lt output/01_*.ppm
sha256sum output/01_*.ppm
```

Inspect and compare `output/01_*.ppm`.

Acceptance criteria:

- Shaft culling produces the same visibility behavior.
- Cube Jacobi image matches baseline.
- No new allocation appears in culling loops.

## Step 8: Migrate Galerkin Direct Dependencies

Replace remaining direct patch bounds usage in Galerkin linking, refinement, form factor setup, and rendering helpers.

Rules:

- Surface element bounds should resolve through the spatial store when the element references a `Patch`.
- Cluster element bounds may continue to use aggregate geometry/cluster bounding boxes.
- Do not merge this with a larger redesign of cluster nodes. This step only removes patch-owned bounding box dependence.

Build and run:

```bash
# outside sandbox
<normal C++ build command>
scripts/01_runCubeJacobi.sh
ls -lt output/01_*.ppm
sha256sum output/01_*.ppm
```

Inspect and compare `output/01_*.ppm`.

Acceptance criteria:

- Galerkin completes the same script.
- Output image matches baseline.
- Remaining `Patch::boundingBox` reads are only compatibility writes/reads or temporary assertions.

## Step 9: Remove or Deprecate Patch-Owned BoundingBox

Once all hot and normal call sites use `PatchSpatialDataStore`, choose one of two finalization paths.

Preferred final path:

- Remove `Patch::boundingBox`.
- Make any old serialized patch bounding box data load into `PatchSpatialDataStore`.
- Keep binary format backward compatibility if old assets are still used.

Lower-risk transitional path:

- Keep `Patch::boundingBox` behind a compatibility macro or deprecated field.
- Add a comment stating it must not be used by runtime code.
- Add a search/check in the build or test notes to prevent new direct usage.

Suggested enforcement:

```bash
rg -n "patch->boundingBox|\\.boundingBox" <patch-related-source-dirs>
```

Manually inspect any remaining hits.

Build and run:

```bash
# outside sandbox
<normal C++ build command>
scripts/01_runCubeJacobi.sh
ls -lt output/01_*.ppm
sha256sum output/01_*.ppm
```

Inspect and compare `output/01_*.ppm`.

Acceptance criteria:

- No runtime code depends on `Patch` owning `BoundingBox`.
- Cube Jacobi image matches baseline.
- Backward-compatible scene loading still works, or the format change is explicitly documented.

## Step 10: Performance Guardrail

Run a basic timing comparison with the same scene before and after finalization.

Suggested process:

```bash
# outside sandbox
<normal C++ build command>
time scripts/01_runCubeJacobi.sh
time scripts/01_runCubeJacobi.sh
time scripts/01_runCubeJacobi.sh
```

Compare against baseline timing from before the refactor.

Acceptance criteria:

- No measurable regression beyond normal run-to-run noise.
- If timing is worse, inspect for accidental recomputation, non-inline accessor overhead, extra allocation, or non-contiguous storage.

## Final Acceptance Checklist

- `Patch` no longer owns `BoundingBox`, or the field is only temporary/deprecated compatibility storage.
- All patch bounds are stored in a dedicated spatial data store.
- Patch bounds lookup is O(1) through an integer index.
- Bounds are computed once during scene construction/loading.
- Cluster hierarchy construction uses the spatial store.
- Voxel/grid acceleration uses the spatial store.
- Shaft culling uses the spatial store.
- Galerkin still builds and runs.
- `scripts/01_runCubeJacobi.sh` completes successfully.
- `output/01_*.ppm` matches the baseline image.
- No new hot-path allocation or hash lookup was introduced.

## Notes for the Implementing Agent

After every step, explicitly report:

- The build command used.
- Whether it ran outside the sandbox.
- Whether `scripts/01_runCubeJacobi.sh` completed.
- Which `output/01_*.ppm` file was inspected.
- Whether image comparison passed.
- Any remaining direct `Patch::boundingBox` references and why they remain.

If any step changes the output image, stop and investigate before continuing.
