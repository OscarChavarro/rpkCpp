package vsdk.toolkit.galerkin;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.PatchIntersectionWorkspace;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RayHit;

public class ShadowCache {
    private static final int MAX_CACHE = 5;
    private Patch[] patchCache = new Patch[MAX_CACHE];
    private int numberOfCachedPatches;

    /**
Initialize/empty the shadow cache
*/
    public ShadowCache() {
        numberOfCachedPatches = 0;
        for ( int i = 0; i < MAX_CACHE; i++ ) {
            patchCache[i] = null;
        }
    }

    /**
Test ray against patches in the shadow cache. Returns nullptr if the ray hits
no patches in the shadow cache, or a pointer to the first hit patch otherwise
*/
    public RayHit cacheHit(Ray ray, float[] distance, RayHit hitStore, RayExclusionSet exclusions) {
        return cacheHit(ray, distance, hitStore, exclusions, null);
    }

    /**
Same as the method above; patchWorkspace is the patch intersection scratch of
the calling thread, or null for the single threaded callers (Java adaptation)
*/
    public RayHit cacheHit(
        Ray ray,
        float[] distance,
        RayHit hitStore,
        RayExclusionSet exclusions,
        PatchIntersectionWorkspace patchWorkspace) {
        for ( int i = 0; i < numberOfCachedPatches; i++ ) {
            if ( patchCache[i] == null ) {
                continue;
            }
            RayHit hit = patchWorkspace == null ?
                patchCache[i].intersect(
                    ray,
                    Numeric.EPSILON_FLOAT * distance[0],
                    distance,
                    RayHitFlag.FRONT | RayHitFlag.ANY,
                    hitStore,
                    exclusions) :
                patchCache[i].intersect(
                    ray,
                    Numeric.EPSILON_FLOAT * distance[0],
                    distance,
                    RayHitFlag.FRONT | RayHitFlag.ANY,
                    hitStore,
                    exclusions,
                    patchWorkspace);
            if ( hit != null ) {
                return hit;
            }
        }
        return null;
    }

    /**
Replace least recently added patch
*/
    public void addToShadowCache(Patch patch) {
        patchCache[numberOfCachedPatches % MAX_CACHE] = patch;
        if ( numberOfCachedPatches < MAX_CACHE ) {
            numberOfCachedPatches++;
        }
    }
}
