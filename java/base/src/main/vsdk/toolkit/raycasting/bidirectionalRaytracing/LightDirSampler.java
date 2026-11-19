/**
Samples a direction from a light source point.
It's kind of a dual of a pixel sampler.
*/

package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.raycasting.raytracing.Sampler;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;

public class LightDirSampler extends Sampler {
    /**
    sample : newNode gets filled, others may change
    */
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
        double[] pdfDir = new double[] {0.0};

        if ( thisNode.m_hit.getMaterial().getEdf() == null ) {
            Logger.error("CLightDirSampler::sample", "No EDF");
            return false;
        }

        // Sample a light direction
        Vector3D dir = new Vector3D(0.0f, 0.0f, 0.0f);
        if ( thisNode.m_hit.getMaterial().getEdf() != null ) {
            dir = thisNode.m_hit.getMaterial().getEdf().phongEdfSample(thisNode.m_hit.shadingContext(), XxdfComponentFlag.DIFFUSE_COMPONENT, x1, x2, thisNode.m_bsdfEval, pdfDir);
        }

        if ( pdfDir[0] < Numeric.EPSILON ) {
            return false;
        } // Zero probabilityDensityFunction event, no valid sample

        // Determine ray type
        thisNode.m_rayType = PathRayType.STARTS;
        newNode.m_inBsdf = thisNode.m_outBsdf; // Light can be placed in a medium

        // Transfer
        if ( !sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, dir, pdfDir[0]) ) {
            thisNode.m_rayType = PathRayType.STOPS;
            return false;
        }

        // Diffuse lights only, put this correctly in 'bsdfEval'
        thisNode.m_bsdfComp.Clear();
        thisNode.m_bsdfComp.Fill(thisNode.m_bsdfEval, (byte)BsdfComponent.BRDF_DIFFUSE_COMPONENT);

        // Component propagation
        thisNode.m_usedComponents = 0; // the light...
        newNode.m_accUsedComponents = (byte)(thisNode.m_accUsedComponents | thisNode.m_usedComponents);

        newNode.accumulatedRussianRouletteFactors = thisNode.accumulatedRussianRouletteFactors;

        return true;
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
        double pdfDir;
        double cosA;
        double dist;
        double dist2;
        Vector3D outDir = new Vector3D();

        if ( thisNode.m_hit.getMaterial().getEdf() == null ) {
            Logger.error("CLightDirSampler::evalPdf", "No EDF");
            return 0.0;
        }
        // More efficient with extra params?

        outDir.subtraction(newNode.m_hit.getPoint(), thisNode.m_hit.getPoint());
        dist2 = outDir.norm2();
        dist = Math.sqrt(dist2);
        outDir.inverseScaledCopy((float)dist, outDir, Numeric.EPSILON_FLOAT);

        // EDF sampling
        if ( thisNode.m_hit.getMaterial().getEdf() == null ) {
            pdfDir = 0.0;
        } else {
            double[] outPdf = new double[] {0.0};
            thisNode.m_hit.getMaterial().getEdf().phongEdfEval(thisNode.m_hit.shadingContext(), outDir, XxdfComponentFlag.DIFFUSE_COMPONENT, outPdf);
            pdfDir = outPdf[0];
        }

        if ( pdfDir < 0.0 ) {
            // Back face of a light does not radiate !
            return 0.0;
        }

        cosA = -outDir.dotProduct(newNode.m_normal);

        double pdf = pdfDir * cosA / dist2;
        if ( probabilityDensityFunction != null && probabilityDensityFunction.length > 0 ) {
            probabilityDensityFunction[0] = pdf;
        }
        if ( probabilityDensityFunctionRR != null && probabilityDensityFunctionRR.length > 0 ) {
            probabilityDensityFunctionRR[0] = 1.0;
        }

        return pdf;
    }
}
