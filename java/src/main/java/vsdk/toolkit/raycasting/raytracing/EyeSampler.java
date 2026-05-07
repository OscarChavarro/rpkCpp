/**
Just fills in the eye point in the node
*/

package vsdk.toolkit.raycasting.raytracing;

import vsdk.toolkit.common.Error;
import vsdk.toolkit.skin.RayHitFlag;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.RayHit;

public class EyeSampler extends Sampler {
    // Sample : newNode gets filled, others may change
    @Override
    public boolean
    sample(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        SimpleRaytracingPathNode prevNode,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        double x1,
        double x2,
        boolean doRR,
        byte flags)
    {
        if ( prevNode != null || thisNode != null ) {
            Error.warning("EyeSampler::sample", "Not first node in path ?!");
        }

        // Just fill in newNode with camera data. Appropriate pdf fields are set to 1

        newNode.m_depth = 0; // We expect this to be the first node in an eye path
        newNode.m_rayType = PathRayType.STOPS;

        // Choose eye : N/A
        // Choose point on eye : N/A

        // Fake a hit record
        RayHit hit = newNode.m_hit;

        hit.init(null, camera.eyePosition, camera.Z, null);
        hit.setNormal(camera.Z);
        int newFlags = hit.getFlags() | RayHitFlag.NORMAL | RayHitFlag.SHADING_FRAME;
        hit.setFlags(newFlags);
        hit.setShadingFrame(camera.X, camera.Y, camera.Z);

        newNode.m_normal.copy(newNode.m_hit.getNormal());
        newNode.m_G = 1.0;

        // outDir's not filled in
        newNode.m_pdfFromPrev = 1.0; // Differential eye area cancels out with computing the flux

        newNode.m_pdfFromNext = 0.0; // Eye cannot be hit accidentally, this pdf is never used

        newNode.m_useBsdf = null;
        newNode.m_inBsdf = null;
        newNode.m_outBsdf = null;

        // Component propagation
        newNode.m_accUsedComponents = 0; // Eye had no accumulated comps.

        newNode.accumulatedRussianRouletteFactors = 1.0; // No russian roulette

        return true;
    }

    @Override
    public double
    evalPDF(
        Camera camera,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        byte flags,
        double[] probabilityDensityFunction,
        double[] probabilityDensityFunctionRR)
    {
        return 1.0;
    }
}
