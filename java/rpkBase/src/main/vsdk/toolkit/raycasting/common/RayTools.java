/**
Some utility routines for ray intersections and for statistics
*/

package vsdk.toolkit.raycasting.common;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.skin.Numeric;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;

public class RayTools {
    private static void countRay(SceneQueryWorkspace workspace) {
        if ( !workspace.recordRay() ) {
            Statistics.instance().rayTracer.rayCount++;
        }
    }

    private static int pathFrontHitFlags() {
        return RayHitFlag.FRONT | RayHitFlag.POINT | RayHitFlag.MATERIAL;
    }

    private static RpkRayHit
    traceWorld(
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneWorldSceneQueryWorkspace,
        Ray ray,
        Patch patch,
        int flags,
        Patch extraPatch,
        RpkRayHit hitStore)
    {
        double[] dist = new double[1];
        RpkRayHit result;
        if (hitStore == null) {
            hitStore = new RpkRayHit();
        }

        dist[0] = Double.MAX_VALUE;
        RayExclusionSet exclusions = new RayExclusionSet(patch, patch != null ? patch.getTwin() : null, extraPatch);
        result = sceneQuery.intersect(ray, 0.0, dist, flags, hitStore, exclusions, sceneWorldSceneQueryWorkspace);

        if (result != null) {
            // Compute shading frame (Z-axis = shading normal) at intersection point
            CoordinateSystem frame = result.getShadingFrame();
            result.setShadingFrame(frame);
        }

        return result;
    }

    public static RpkRayHit
    findRayIntersection(
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneWorldSceneQueryWorkspace,
        Ray ray,
        Patch patch,
        PhongBidirectionalScatteringDistributionFunction currentBsdf,
        RpkRayHit hitStore)
    {
        int hitFlags;
        RpkRayHit newHit;

        if (currentBsdf == null) {
            // Outside everything in vacuum
            hitFlags = pathFrontHitFlags();
        }
        else {
            hitFlags = pathFrontHitFlags() | RayHitFlag.BACK;
        }

        // Trace the ray
        newHit = traceWorld(sceneQuery, sceneWorldSceneQueryWorkspace, ray, patch, hitFlags, null, hitStore);
        countRay(sceneWorldSceneQueryWorkspace);

        // Robustness test : If a back is hit, check the current
        // bsdf and the bsdf of the material hit. If they
        // don't match, exclude this patch and trace again :-(
        if (newHit != null && (newHit.getFlags() & RayHitFlag.BACK) != 0 &&
            newHit.getPatch().getMaterial().getBsdf() != currentBsdf) {
            // Whoops, intersected with wrong patch (accuracy problem)
            newHit = traceWorld(sceneQuery, sceneWorldSceneQueryWorkspace, ray, patch, hitFlags, newHit.getPatch(), hitStore);
            countRay(sceneWorldSceneQueryWorkspace);
        }

        return newHit;
    }

    /**
pathNodesVisible : send a shadow ray
*/
    public static boolean
    pathNodesVisible(
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneWorldSceneQueryWorkspace,
        SimpleRaytracingPathNode node1,
        SimpleRaytracingPathNode node2)
    {
        Vector3Dd dir = new Vector3Dd();
        Ray ray = new Ray();
        RpkRayHit hit;
        RpkRayHit hitStore = new RpkRayHit();
        double cosRay1;
        double cosRay2;
        double dist;
        double dist2;
        double[] fDistance = new double[1];
        boolean visible;
        boolean doTest;

        // Determines visibility between two nodes,
        // Returns visibility and direction from eye to light node (newDir_e)
        if (node1.m_hit.getPatch() == node2.m_hit.getPatch()) {
            // Same patch cannot see itself. Wrong for concave primitives!
            return false;
        }

        dir = node2.m_hit.getPoint().subtract(node1.m_hit.getPoint());

        dist2 = dir.lengthSquared();
        dist = Math.sqrt(dist2);

        dir = dir.multiplyByInverse((double)dist, Numeric.EPSILON);

        dist = dist * (1 - VSDK.EPSILON);

        ray = ray.withOrigin(node1.m_hit.getPoint());
        ray = ray.withDirection(dir);

        cosRay1 = dir.dotProduct(node1.m_normal);
        cosRay2 = -dir.dotProduct(node2.m_normal);

        doTest = false;

        if (cosRay1 > 0) {
            if (cosRay2 > 0) {
                // Normal case : reflected rays.
                doTest = true;
            }
            else {
                // Node1 reflects, node2 transmits
                if (node1.m_inBsdf == node2.m_outBsdf) {
                    doTest = true;
                }
            }
        }
        else {
            if (cosRay2 > 0) {
                if (node1.m_outBsdf == node2.m_inBsdf) {
                    doTest = true;
                }
            }
            else {
                if (node1.m_outBsdf == node2.m_outBsdf) {
                    doTest = true;
                }
            }
        }

        if (doTest) {
            if (node2.m_hit.getPatch().hasZeroVertices()) {
                fDistance[0] = Double.MAX_VALUE;
            }
            else {
                fDistance[0] = (double)dist;
            }

            RayExclusionSet exclusions = new RayExclusionSet(
                node2.m_hit.getPatch(),
                node1.m_hit.getPatch(),
                node1.m_hit.getPatch() != null ? node1.m_hit.getPatch().getTwin() : null);
            hit = sceneQuery.intersect(
                ray,
                0.0,
                fDistance,
                RayHitFlag.FRONT | RayHitFlag.BACK | RayHitFlag.ANY,
                hitStore,
                exclusions,
                sceneWorldSceneQueryWorkspace);
            visible = (hit == null);

            countRay(sceneWorldSceneQueryWorkspace);
        }
        else {
            visible = false;
        }

        return visible;
    }

    /**
Can the eye see the node ?  If so, pix_x and pix_y are filled in
*/
    public static boolean
    eyeNodeVisible(
        Camera camera,
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneWorldSceneQueryWorkspace,
        SimpleRaytracingPathNode eyeNode,
        SimpleRaytracingPathNode node,
        double[] pixX,
        double[] pixY)
    {
        Vector3Dd dir = new Vector3Dd();
        Ray ray = new Ray();
        RpkRayHit hit;
        RpkRayHit hitStore = new RpkRayHit();
        double cosRayLight;
        double cosRayEye;
        double dist;
        double dist2;
        double[] fDistance = new double[1];
        double x;
        double y;
        double z;
        double xz;
        double yz;

        // Determines visibility between two nodes,
        // Returns visibility and direction from eye to light node (newDir_e)
        dir = node.m_hit.getPoint().subtract(eyeNode.m_hit.getPoint());

        dist2 = dir.lengthSquared();
        dist = Math.sqrt(dist2);

        dir = dir.multiplyByInverse((double)dist, Numeric.EPSILON);

        // Determine which pixel is visible
        z = dir.dotProduct(camera.Z);

        boolean visible = false;
        if (z > 0.0) {
            x = dir.dotProduct(camera.X);
            xz = x / z;

            if (Math.abs(xz) < camera.pixelWidthTangent) {
                y = dir.dotProduct(camera.Y);
                yz = y / z;

                if (Math.abs(yz) < camera.pixelHeightTangent) {
                    // Point is within view pyramid

                    // Check normal directions
                    dist = dist * (1 - VSDK.EPSILON);

                    ray = ray.withOrigin(eyeNode.m_hit.getPoint());
                    ray = ray.withDirection(dir);

                    cosRayEye = dir.dotProduct(eyeNode.m_normal);
                    cosRayLight = -dir.dotProduct(node.m_normal);

                    if ((cosRayLight > 0) && (cosRayEye > 0)) {
                        fDistance[0] = (double)dist;
                        RayExclusionSet exclusions = new RayExclusionSet(
                            node.m_hit.getPatch(), eyeNode.m_hit.getPatch(),
                            eyeNode.m_hit.getPatch() != null ? eyeNode.m_hit.getPatch().getTwin() : null);
                        hit = sceneQuery.intersect(ray,
                            0.0, fDistance,
                            RayHitFlag.FRONT | RayHitFlag.ANY, hitStore,
                            exclusions, sceneWorldSceneQueryWorkspace);
                        // HIT_BACK removed ! So you can see through back walls with N.E.E
                        visible = (hit == null);

                        // Geometry factor
                        if (visible) {
                            if (pixX != null && pixX.length > 0) {
                                pixX[0] = (double)xz;
                            }
                            if (pixY != null && pixY.length > 0) {
                                pixY[0] = (double)yz;
                            }
                        }
                    }
                }
            }
        }

        return visible;
    }
}
