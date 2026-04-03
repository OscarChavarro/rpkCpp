/**
Generate and trace a local line
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.RayHit;

public final class Localline {
    private Localline() {
    }

    /**
Creates a coordinate system on the patch P with Z direction along the normal
*/
    private static void patchCoordSys(Patch patch, CoordinateSystem coord) {
        Vector3D z = patch.normal;
        Vector3D x = new Vector3D();
        x.subtraction(patch.vertex[1].point, patch.vertex[0].point);
        x.normalize(Numeric.EPSILON_FLOAT);
        Vector3D y = new Vector3D();
        y.crossProduct(z, x);

        coord.setX(x);
        coord.setY(y);
        coord.setZ(z);
    }

    /**
Constructs a ray with uniformly chosen origin on patch and cosine distributed
direction with respect to patch normal. Origin and direction are uniquely determined
by the 4-dimensional sample vector xi
*/
    public static Ray mcrGenerateLocalLine(Patch patch, double[] xi) {
        Ray ray = new Ray();
        double[] pdf = new double[1];

        if ( patch != previousPatch ) {
            // Some work that does not need to be done over if the current patch is the
            // same as the previous one, which is often the case
            patchCoordSys(patch, coordSys);
            previousPatch = patch;
        }

        patch.uniformPoint(xi[0], xi[1], ray.position);
        // Section [ARVO1995b].2: use two uniform samples from [0,1]^2 to
        // generate a local hemisphere direction in the patch frame.
        ray.direction = coordSys.sampleHemisphereCosTheta(xi[2], xi[3], pdf);

        return ray;
    }

    /**
In order to let the user have the impression that the computations are proceeding
*/
    private static void someFeedback() {
        if ( (StochasticRelaxation.activeState().tracedRays + StochasticRelaxation.activeState().importanceTracedRays) % 1000 == 0 ) {
            System.err.print(".");
        }
    }

    /**
Determines nearest intersection point and patch
*/
    public static RayHit mcrShootRay(VoxelGrid sceneWorldVoxelGrid, Patch P, Ray ray, RayHit hitStore) {
        float[] distance = new float[] {Numeric.HUGE_FLOAT_VALUE};

        // Reject self-intersections
        Patch.dontIntersect2(P, P.twin);
        RayHit hit = sceneWorldVoxelGrid.gridIntersect(
            ray,
            Numeric.EPSILON_FLOAT < P.tolerance ? Numeric.EPSILON_FLOAT : P.tolerance,
            distance,
            vsdk.toolkit.material.RayHitFlag.FRONT | vsdk.toolkit.material.RayHitFlag.POINT,
            hitStore);
        Patch.dontIntersect0();
        someFeedback();

        return hit;
    }

    private static Patch previousPatch = null;
    private static final CoordinateSystem coordSys = new CoordinateSystem();
}
