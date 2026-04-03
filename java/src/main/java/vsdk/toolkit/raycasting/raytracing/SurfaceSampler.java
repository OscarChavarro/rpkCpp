package vsdk.toolkit.raycasting.raytracing;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.material.RayHitFlag;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.BsdfComp;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.RayHit;

/**
A surface sampler is for scattering on surfaces. Here we need
extra parameters to decide if russian roulette is necessary and
flags to indicate what components of the bsdf should be sampled
and evaluated.
*/
public abstract class SurfaceSampler extends Sampler {
    protected boolean m_computeFromNextPdf;
    protected boolean m_computeBsdfComponents;

    public SurfaceSampler() {
        m_computeFromNextPdf = false;
        m_computeBsdfComponents = false;
    }

    protected static void DetermineRayType(SimpleRaytracingPathNode thisNode, SimpleRaytracingPathNode newNode, Vector3D dir) {
        double cosThisPatch = dir.dotProduct(thisNode.m_normal);

        if ( cosThisPatch < 0 ) {
            // Refraction !
            if ( (thisNode.m_hit.getFlags() & RayHitFlag.BACK) != 0 ) {
                thisNode.m_rayType = PathRayType.LEAVES;
            } else {
                thisNode.m_rayType = PathRayType.ENTERS;
            }

            newNode.m_inBsdf = thisNode.m_outBsdf;
        } else {
            // Reflection
            thisNode.m_rayType = PathRayType.REFLECTS;
            newNode.m_inBsdf = thisNode.m_inBsdf; // staying in same medium
        }
    }

    /**
    DoBsdfEval : this just evaluates the bsdf but depending on
    m_computeBsdfComponents uses BsdfEval or BsdfEvalComponents
    Introduced to share code
    */
    public ColorRgb
    DoBsdfEval(
        PhongBidirectionalScatteringDistributionFunction bsdf,
        RayHit hit,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        Vector3D out,
        byte flags,
        BsdfComp bsdfComp)
    {
        if ( m_computeBsdfComponents ) {
            if ( bsdf == null ) {
                ColorRgb black = new ColorRgb();
                black.clear();
                return black;
            } else {
                BsdfComp localBsdfComp = bsdfComp != null ? bsdfComp : new BsdfComp();
                return bsdf.bsdfEvalComponents(hit, inBsdf, outBsdf, in, out, flags & 0xFF, localBsdfComp.asArray());
            }
        } else {
            if ( bsdfComp != null ) {
                bsdfComp.Clear();
            }
            ColorRgb radiance = new ColorRgb();
            if ( bsdf == null ) {
                radiance.clear();
            } else {
                radiance = bsdf.evaluate(hit, inBsdf, outBsdf, in, out, flags & 0xFF);
            }
            return radiance;
        }
    }

    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   When path ends (absorption) the type of thisNode is adjusted to 'Ends'
    public abstract boolean
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
        byte flags);

    // EvalPDF : returns probabilityDensityFunction INCLUDING russian roulette. Separate
    // components can be obtained through probabilityDensityFunction and probabilityDensityFunctionRR params
    public abstract double
    evalPDF(
        Camera camera,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        byte flags,
        double[] probabilityDensityFunction,
        double[] probabilityDensityFunctionRR);

    // Use this for calculating f.i. eyeEndNode->Previous pdf(Next).
    // The newNode is calculated, thisNode should be and end node connecting
    // to another sub path end node. prevNode is that other sub-path
    // endNode.
    public abstract double
    EvalPDFPrev(
        SimpleRaytracingPathNode prevNode,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        byte flags,
        double[] probabilityDensityFunction,
        double[] probabilityDensityFunctionRR);

    // bool computeFromNextPdf : if true the surface sampler will
    //   compute pdfFromNext in the prevNode. This is needed for
    //   bidirectional algorithm's
    public void SetComputeFromNextPdf(boolean computeFromNextPdf) {
        m_computeFromNextPdf = computeFromNextPdf;
    }

    public void SetComputeBsdfComponents(boolean computeBsdfComponents) {
        m_computeBsdfComponents = computeBsdfComponents;
    }
}
