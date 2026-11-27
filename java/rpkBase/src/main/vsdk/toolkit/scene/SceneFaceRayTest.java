package vsdk.toolkit.scene;

import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.elements.PatchIntersectionWorkspace;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;
import vsdk.toolkit.environment.geometry.spatialIndex.PrimitiveRayTest;

/**
RenderPark's exact face test as the PrimitiveRayTest of a VITRAL
SpatialIndex: primitive i is face i of the scene's face table, tested with
Patch.intersect on the original ray, flags and exclusions, so an index
answer is decided by the same arithmetic as the voxel grid's, the exclusions
included (a face below an excluded geometry is rejected by its face-index
range, plan decision D5). The interval ends and the accepted hit distance are
doubles, as the face test computes them.

One object per query: it writes the accepted hit into the caller's hit
record, so after a closest-hit query the record holds the closest face.

Java adaptation: the scene query keeps one in its workspace and sets it up
for each query with `set`, so a query allocates no test object; it also owns
the scratch of the face test (one workspace, so one test, per thread).
*/
public final class SceneFaceRayTest implements PrimitiveRayTest {
    private final Scene scene;
    private Ray ray;
    private int hitFlags;
    private RayExclusionSet exclusions;
    private RpkRayHit hitStore;
    private final double[] distance;
    private final PatchIntersectionWorkspace patchWorkspace;

    public SceneFaceRayTest(
        Scene inScene,
        Ray inRay,
        int inHitFlags,
        RayExclusionSet inExclusions,
        RpkRayHit inHitStore) {
        scene = inScene;
        distance = new double[1];
        patchWorkspace = new PatchIntersectionWorkspace();
        set(inRay, inHitFlags, inExclusions, inHitStore);
    }

    public void set(Ray inRay, int inHitFlags, RayExclusionSet inExclusions, RpkRayHit inHitStore) {
        ray = inRay;
        hitFlags = inHitFlags;
        exclusions = inExclusions;
        hitStore = inHitStore;
    }

    @Override
    public boolean test(int primitive, double minimumT, double maximumT, double[] hitT) {
        distance[0] = (double)maximumT;
        RpkRayHit hit = scene.getFace(primitive).intersect(
            ray, (double)minimumT, distance, hitFlags, hitStore, exclusions, patchWorkspace);
        if (hit == null) {
            return false;
        }
        hitT[0] = distance[0];
        return true;
    }
}
