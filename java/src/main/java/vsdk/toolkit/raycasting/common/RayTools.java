/**
Some utility routines for ray intersections and for statistics
*/

package vsdk.toolkit.raycasting.common;

import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.material.RayHitFlag;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.RayHit;

public class RayTools {
    private static int pathFrontHitFlags() {
        return RayHitFlag.FRONT | RayHitFlag.POINT | RayHitFlag.MATERIAL;
    }

    private static RayHit
    traceWorld(
        VoxelGrid sceneWorldVoxelGrid,
        Ray ray,
        Patch patch,
        int flags,
        Patch extraPatch,
        RayHit hitStore)
    {
        RayHit myHitStore = new RayHit();
        float[] dist = new float[1];
        RayHit result;
        if (hitStore == null) {
            hitStore = myHitStore;
        }

        dist[0] = Numeric.HUGE_FLOAT_VALUE;
        Patch.dontIntersect3(patch, patch != null ? patch.twin : null, extraPatch);
        result = sceneWorldVoxelGrid.gridIntersect(ray, 0.0f, dist, flags, hitStore);

        if (result != null) {
            // Compute shading frame (Z-axis = shading normal) at intersection point
            CoordinateSystem frame = result.getShadingFrame();
            result.setShadingFrame(frame);
        }
        Patch.dontIntersect0();

        return result;
    }

    public static RayHit
    findRayIntersection(
        VoxelGrid sceneWorldVoxelGrid,
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
        newHit = traceWorld(sceneWorldVoxelGrid, ray, patch, hitFlags, null, hitStore);
        Statistics.instance().rayTracer.rayCount++; // statistics

        // Robustness test : If a back is hit, check the current
        // bsdf and the bsdf of the material hit. If they
        // don't match, exclude this patch and trace again :-(
        if (newHit != null && (newHit.getFlags() & RayHitFlag.BACK) != 0 &&
            newHit.getPatch().material.getBsdf() != currentBsdf) {
            // Whoops, intersected with wrong patch (accuracy problem)
            newHit = traceWorld(sceneWorldVoxelGrid, ray, patch, hitFlags, newHit.getPatch(), hitStore);
            Statistics.instance().rayTracer.rayCount++; // Statistics
        }

        return newHit;
    }

    /**
pathNodesVisible : send a shadow ray
*/
    public static boolean
    pathNodesVisible(
        VoxelGrid sceneWorldVoxelGrid,
        SimpleRaytracingPathNode node1,
        SimpleRaytracingPathNode node2)
    {
        Vector3D dir = new Vector3D();
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

        dir.subtraction(node2.m_hit.getPoint(), node1.m_hit.getPoint());

        dist2 = dir.norm2();
        dist = Math.sqrt(dist2);

        dir.inverseScaledCopy((float)dist, dir, Numeric.EPSILON_FLOAT);

        dist = dist * (1 - Numeric.EPSILON);

        ray.position = node1.m_hit.getPoint();
        ray.direction.copy(dir);

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
                fDistance[0] = Numeric.HUGE_FLOAT_VALUE;
            }
            else {
                fDistance[0] = (float)dist;
            }

            Patch.dontIntersect3(
                node2.m_hit.getPatch(),
                node1.m_hit.getPatch(),
                node1.m_hit.getPatch() != null ? node1.m_hit.getPatch().twin : null);
            hit = sceneWorldVoxelGrid.gridIntersect(
                ray,
                0.0f,
                fDistance,
                RayHitFlag.FRONT | RayHitFlag.BACK | RayHitFlag.ANY,
                hitStore);
            Patch.dontIntersect0();
            visible = (hit == null);

            Statistics.instance().rayTracer.rayCount++; // Statistics
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
        VoxelGrid sceneWorldVoxelGrid,
        SimpleRaytracingPathNode eyeNode,
        SimpleRaytracingPathNode node,
        float[] pixX,
        float[] pixY)
    {
        Vector3D dir = new Vector3D();
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
        dir.subtraction(node.m_hit.getPoint(), eyeNode.m_hit.getPoint());

        dist2 = dir.norm2();
        dist = Math.sqrt(dist2);

        dir.inverseScaledCopy((float)dist, dir, Numeric.EPSILON_FLOAT);

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
                    dist = dist * (1 - Numeric.EPSILON);

                    ray.position = eyeNode.m_hit.getPoint();
                    ray.direction.copy(dir);

                    cosRayEye = dir.dotProduct(eyeNode.m_normal);
                    cosRayLight = -dir.dotProduct(node.m_normal);

                    if ((cosRayLight > 0) && (cosRayEye > 0)) {
                        fDistance[0] = (float)dist;
                        Patch.dontIntersect3(
                            node.m_hit.getPatch(), eyeNode.m_hit.getPatch(),
                            eyeNode.m_hit.getPatch() != null ? eyeNode.m_hit.getPatch().twin : null);
                        hit = sceneWorldVoxelGrid.gridIntersect(ray,
                            0.0f, fDistance,
                            RayHitFlag.FRONT | RayHitFlag.ANY, hitStore);
                        Patch.dontIntersect0();
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
