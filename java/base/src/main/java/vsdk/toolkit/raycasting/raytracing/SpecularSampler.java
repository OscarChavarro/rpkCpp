/**
Path node sampler using PERFECT SPECULAR reflection/refraction only

BEWARE
This is a 'dirac' sampler, meaning that the pdf
for generating the outgoing direction is infinite (dirac)
You CANNOT use these samplers together with other samplers
for multiple importance sampling!

All probabilityDensityFunction evaluations should be multiplied by infinity.

I currently use it only in classical raytracing
*/

package vsdk.toolkit.raycasting.raytracing;

import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;

public class SpecularSampler extends SurfaceSampler {
    @Override
    public boolean sample(
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
        return false;
    }

    @Override
    public double evalPDF(
        Camera camera,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        byte flags,
        double[] probabilityDensityFunction,
        double[] probabilityDensityFunctionRR)
    {
        if ( probabilityDensityFunction != null && probabilityDensityFunction.length > 0 ) {
            probabilityDensityFunction[0] = 0.0;
        }
        if ( probabilityDensityFunctionRR != null && probabilityDensityFunctionRR.length > 0 ) {
            probabilityDensityFunctionRR[0] = 0.0;
        }
        return 0.0;
    }

    @Override
    public double EvalPDFPrev(
        SimpleRaytracingPathNode prevNode,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        byte flags,
        double[] probabilityDensityFunction,
        double[] probabilityDensityFunctionRR)
    {
        if ( probabilityDensityFunction != null && probabilityDensityFunction.length > 0 ) {
            probabilityDensityFunction[0] = 0.0;
        }
        if ( probabilityDensityFunctionRR != null && probabilityDensityFunctionRR.length > 0 ) {
            probabilityDensityFunctionRR[0] = 0.0;
        }
        return 0.0;
    }
}
