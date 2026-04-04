package vsdk.toolkit.galerkin;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.material.RayHitFlag;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.RayHit;

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
    public RayHit cacheHit(Ray ray, float[] distance, RayHit hitStore) {
        for ( int i = 0; i < numberOfCachedPatches; i++ ) {
            if ( patchCache[i] == null ) {
                continue;
            }
            RayHit hit = patchCache[i].intersect(
                ray,
                Numeric.EPSILON_FLOAT * distance[0],
                distance,
                RayHitFlag.FRONT | RayHitFlag.ANY,
                hitStore);
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
