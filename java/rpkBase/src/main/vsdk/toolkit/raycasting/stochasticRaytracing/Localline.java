/**
Generate and trace a local line
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.skin.Numeric;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;

public final class Localline {
    private Localline() {
    }

    /**
Creates a coordinate system on the patch P with Z direction along the normal
*/
    private static void patchCoordSys(Patch patch, CoordinateSystem coord) {
        Vector3Dd z = patch.getNormal();
        Vector3Dd x = new Vector3Dd();
        x = patch.getVertices()[1].point.subtract(patch.getVertices()[0].point);
        x = x.normalized(Numeric.EPSILON);
        Vector3Dd y = new Vector3Dd();
        y = z.crossProduct(x);

        coord.setXAxis(x);
        coord.setYAxis(y);
        coord.setZAxis(z);
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

        ray = ray.withOrigin(patch.uniformPoint(xi[0], xi[1]));
        // Section [ARVO1995b].2: use two uniform samples from [0,1]^2 to
        // generate a local hemisphere direction in the patch frame.
        ray = ray.withDirection(coordSys.sampleHemisphereCosTheta(xi[2], xi[3], pdf));

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
    public static RpkRayHit mcrShootRay(SceneQuery sceneQuery, SceneQueryWorkspace sceneWorldSceneQueryWorkspace, Patch P, Ray ray, RpkRayHit hitStore) {
        double[] distance = new double[] {Double.MAX_VALUE};

        // Reject self-intersections
        RayExclusionSet exclusions = new RayExclusionSet(P, P.getTwin());
        RpkRayHit hit = sceneQuery.intersect(
            ray,
            Numeric.EPSILON < P.getTolerance() ? Numeric.EPSILON : P.getTolerance(),
            distance,
            vsdk.toolkit.environment.geometry.elements.RayHitFlag.FRONT | vsdk.toolkit.environment.geometry.elements.RayHitFlag.POINT,
            hitStore,
            exclusions,
            sceneWorldSceneQueryWorkspace);
        someFeedback();

        return hit;
    }

    private static Patch previousPatch = null;
    private static final CoordinateSystem coordSys = new CoordinateSystem();
}
