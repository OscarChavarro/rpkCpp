/**
Some utility routines for ray intersections and for statistics
*/

package vsdk.toolkit.raycasting.common;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;
import vsdk.toolkit.scene.VisibilityQueryRecorder;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayHit;

public class RayTools {
    private static void countRay(SceneQueryWorkspace workspace) {
        if ( !workspace.recordRay() ) {
            Statistics.instance().rayTracer.rayCount++;
        }
    }

    private static int pathFrontHitFlags() {
        return RayHitFlag.FRONT | RayHitFlag.POINT | RayHitFlag.MATERIAL;
    }

    private static RayHit
    traceWorld(
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneWorldSceneQueryWorkspace,
        Ray ray,
        Patch patch,
        int flags,
        Patch extraPatch,
        RayHit hitStore)
    {
        float[] dist = new float[1];
        RayHit result;
        if (hitStore == null) {
            hitStore = new RayHit();
        }

        dist[0] = Float.MAX_VALUE;
        RayExclusionSet exclusions = new RayExclusionSet(patch, patch != null ? patch.twin : null, extraPatch);
        result = sceneQuery.intersect(ray, 0.0f, dist, flags, hitStore, exclusions, sceneWorldSceneQueryWorkspace);
        if (sceneWorldSceneQueryWorkspace.getRecorder() != null) {
            sceneWorldSceneQueryWorkspace.getRecorder().recordQuery(
                VisibilityQueryRecorder.RAYTRACER_CLOSEST_HIT, ray, 0.0f, Float.MAX_VALUE,
                flags, exclusions, result, dist[0]);
        }

        if (result != null) {
            // Compute shading frame (Z-axis = shading normal) at intersection point
            CoordinateSystem frame = result.getShadingFrame();
            result.setShadingFrame(frame);
        }

        return result;
    }

    public static RayHit
    findRayIntersection(
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneWorldSceneQueryWorkspace,
        Ray ray,
        Patch patch,
        PhongBidirectionalScatteringDistributionFunction currentBsdf,
        RayHit hitStore)
    {
        int hitFlags;
        RayHit newHit;

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
            newHit.getPatch().material.getBsdf() != currentBsdf) {
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
        Vector3Df dir = new Vector3Df();
        Ray ray = new Ray();
        RayHit hit;
        RayHit hitStore = new RayHit();
        double cosRay1;
        double cosRay2;
        double dist;
        double dist2;
        float[] fDistance = new float[1];
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

        dir = dir.multiplyByInverse((float)dist, Numeric.EPSILON_FLOAT);

        dist = dist * (1 - VSDK.EPSILON);

        ray.position = node1.m_hit.getPoint();
        ray.direction = dir;

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
                fDistance[0] = Float.MAX_VALUE;
            }
            else {
                fDistance[0] = (float)dist;
            }

            final float queryMaximumDistance = fDistance[0];
            RayExclusionSet exclusions = new RayExclusionSet(
                node2.m_hit.getPatch(),
                node1.m_hit.getPatch(),
                node1.m_hit.getPatch() != null ? node1.m_hit.getPatch().twin : null);
            hit = sceneQuery.intersect(
                ray,
                0.0f,
                fDistance,
                RayHitFlag.FRONT | RayHitFlag.BACK | RayHitFlag.ANY,
                hitStore,
                exclusions,
                sceneWorldSceneQueryWorkspace);
            if (sceneWorldSceneQueryWorkspace.getRecorder() != null) {
                sceneWorldSceneQueryWorkspace.getRecorder().recordQuery(
                    VisibilityQueryRecorder.RAYTRACER_VISIBILITY, ray, 0.0f, queryMaximumDistance,
                    RayHitFlag.FRONT | RayHitFlag.BACK | RayHitFlag.ANY, exclusions, hit, fDistance[0]);
            }
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
        float[] pixX,
        float[] pixY)
    {
        Vector3Df dir = new Vector3Df();
        Ray ray = new Ray();
        RayHit hit;
        RayHit hitStore = new RayHit();
        double cosRayLight;
        double cosRayEye;
        double dist;
        double dist2;
        float[] fDistance = new float[1];
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

        dir = dir.multiplyByInverse((float)dist, Numeric.EPSILON_FLOAT);

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

                    ray.position = eyeNode.m_hit.getPoint();
                    ray.direction = dir;

                    cosRayEye = dir.dotProduct(eyeNode.m_normal);
                    cosRayLight = -dir.dotProduct(node.m_normal);

                    if ((cosRayLight > 0) && (cosRayEye > 0)) {
                        fDistance[0] = (float)dist;
                        final float queryMaximumDistance = fDistance[0];
                        RayExclusionSet exclusions = new RayExclusionSet(
                            node.m_hit.getPatch(), eyeNode.m_hit.getPatch(),
                            eyeNode.m_hit.getPatch() != null ? eyeNode.m_hit.getPatch().twin : null);
                        hit = sceneQuery.intersect(ray,
                            0.0f, fDistance,
                            RayHitFlag.FRONT | RayHitFlag.ANY, hitStore,
                            exclusions, sceneWorldSceneQueryWorkspace);
                        if (sceneWorldSceneQueryWorkspace.getRecorder() != null) {
                            sceneWorldSceneQueryWorkspace.getRecorder().recordQuery(
                                VisibilityQueryRecorder.RAYTRACER_VISIBILITY, ray, 0.0f, queryMaximumDistance,
                                RayHitFlag.FRONT | RayHitFlag.ANY, exclusions, hit, fDistance[0]);
                        }
                        // HIT_BACK removed ! So you can see through back walls with N.E.E
                        visible = (hit == null);

                        // Geometry factor
                        if (visible) {
                            if (pixX != null && pixX.length > 0) {
                                pixX[0] = (float)xz;
                            }
                            if (pixY != null && pixY.length > 0) {
                                pixY[0] = (float)yz;
                            }
                        }
                    }
                }
            }
        }

        return visible;
    }
}
