/**
Path node sampler using bsdf sampling
*/

package vsdk.toolkit.raycasting.raytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;

public class BsdfSampler extends SurfaceSampler {
    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path ends.
    //   If path ends (absorption) the type of thisNode is adjusted to 'Ends'
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
        double[] pdfDir = new double[] {0.0};

        // Sample direction
        Vector3D dir = new Vector3D(0.0f, 0.0f, 0.0f);

        if ( thisNode.m_useBsdf != null ) {
            dir = thisNode.m_useBsdf.sample(
                thisNode.m_hit,
                thisNode.m_inBsdf,
                thisNode.m_outBsdf,
                thisNode.m_inDirF,
                doRR ? 1 : 0,
                flags & 0xFF,
                x1,
                x2,
                pdfDir);
        }

        if ( pdfDir[0] <= Numeric.EPSILON ) {
            // No good sample
            return false;
        }

        newNode.accumulatedRussianRouletteFactors = thisNode.accumulatedRussianRouletteFactors;
        if ( doRR ) {
            ColorRgb albedo = new ColorRgb();
            albedo.clear();
            if ( thisNode.m_useBsdf != null ) {
                albedo = thisNode.m_useBsdf.splitBsdfScatteredPower(thisNode.m_hit, flags & 0xFF);
            }
            newNode.accumulatedRussianRouletteFactors *= albedo.average();
        }

        // Reflection Type, changes thisNode->m_rayType and newNode->m_inBsdf
        DetermineRayType(thisNode, newNode, dir);

        // Transfer
        if ( !sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, dir, pdfDir[0]) ) {
            thisNode.m_rayType = PathRayType.STOPS;
            return false;
        }

        // Fill in bsdf of current node
        thisNode.m_bsdfEval = DoBsdfEval(
            thisNode.m_useBsdf,
            thisNode.m_hit,
            thisNode.m_inBsdf,
            thisNode.m_outBsdf,
            thisNode.m_inDirF,
            newNode.m_inDirT,
            flags,
            thisNode.m_bsdfComp);

        // Accumulate scattering components
        thisNode.m_usedComponents = flags;
        newNode.m_accUsedComponents = (byte)(thisNode.m_accUsedComponents | thisNode.m_usedComponents);

        // Fill in probability for previous node
        if ( m_computeFromNextPdf && prevNode != null ) {
            double cosI = thisNode.m_normal.dotProduct(thisNode.m_inDirF);
            double[] pdfDirI = new double[] {0.0};
            double[] pdfRR = new double[] {0.0};

            if ( thisNode.m_useBsdf != null ) {
                // prevpdf : new->this->prev pdf evaluation
                // normal direction is handled by the evalpdf routine
                // Are the flags usable in both directions?
                thisNode.m_useBsdf.evaluateProbabilityDensityFunction(
                    thisNode.m_hit,
                    thisNode.m_outBsdf,
                    thisNode.m_inBsdf,
                    newNode.m_inDirT,
                    thisNode.m_inDirF,
                    flags & 0xFF,
                    pdfDirI,
                    pdfRR);
            }

            prevNode.m_rrPdfFromNext = pdfRR[0];
            prevNode.m_pdfFromNext = pdfDirI[0] * thisNode.m_G / cosI;
        }

        return true; // Node filled in
    }

    // Use this for N.E.E. : connecting a light node with an eye node
    @Override
    public double
    evalPDF(
        Camera camera,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        byte flags,
        double[] pdf,
        double[] pdfRR)
    {
        double[] pdfH = new double[1];
        double[] pdfRRH = new double[1];

        if ( pdf == null ) {
            pdf = pdfH;
        }
        if ( pdfRR == null ) {
            pdfRR = pdfRRH;
        }

        // More efficient with extra params?
        double dist2;
        double dist;
        Vector3D outDir = new Vector3D();

        outDir.subtraction(newNode.m_hit.getPoint(), thisNode.m_hit.getPoint());
        dist2 = outDir.norm2();
        dist = Math.sqrt(dist2);
        outDir.inverseScaledCopy((float)dist, outDir, Numeric.EPSILON_FLOAT);

        // Beware : NOT RECIPROKE!
        double[] pdfDir = new double[] {0.0};
        pdfRR[0] = 0.0;
        if ( thisNode.m_useBsdf != null ) {
            thisNode.m_useBsdf.evaluateProbabilityDensityFunction(
                thisNode.m_hit,
                thisNode.m_inBsdf,
                thisNode.m_outBsdf,
                thisNode.m_inDirF,
                outDir,
                flags & 0xFF,
                pdfDir,
                pdfRR);
        }

        // To area measure
        double cosA;

        cosA = -outDir.dotProduct(newNode.m_normal);

        pdf[0] = pdfDir[0] * cosA / dist2;

        return pdf[0] * pdfRR[0];
    }

    // Use this for calculating f.i. eyeEndNode->Previous pdf(Next).
    // The newNode is calculated, thisNode should be and end node connecting
    // to another sub path end node. prevNode is that other subpath
    // endNode.
    @Override
    public double
    EvalPDFPrev(
        SimpleRaytracingPathNode prevNode,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        byte flags,
        double[] pdf,
        double[] pdfRR)
    {
        double[] pdfH = new double[1];
        double[] pdfRRH = new double[1];
        Vector3D outDir = new Vector3D();

        if ( pdf == null ) {
            pdf = pdfH;
        }
        if ( pdfRR == null ) {
            pdfRR = pdfRRH;
        }

        // More efficient with extra params?
        outDir.subtraction(prevNode.m_hit.getPoint(), thisNode.m_hit.getPoint());
        outDir.normalize(Numeric.EPSILON_FLOAT);

        // Beware : NOT RECIPROCAL!
        double[] pdfDir = new double[] {0.0};
        pdfRR[0] = 0.0;
        if ( thisNode.m_useBsdf != null ) {
            thisNode.m_useBsdf.evaluateProbabilityDensityFunction(
                thisNode.m_hit,
                thisNode.m_outBsdf,
                thisNode.m_inBsdf,
                outDir,
                thisNode.m_inDirF,
                flags & 0xFF,
                pdfDir,
                pdfRR);
        }

        // To area measure
        double cosB = thisNode.m_inDirF.dotProduct(thisNode.m_normal);

        pdf[0] = pdfDir[0] * thisNode.m_G / cosB;

        return pdf[0] * pdfRR[0];
    }
}
