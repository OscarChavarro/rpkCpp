package vsdk.toolkit.raycasting.raytracing;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;

public class PixelSampler extends Sampler {
    private double m_px;
    private double m_py;

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
        Vector3D dir = new Vector3D();

        // Pre-conditions: 1. thisNode == eye 2. prevNode == nullptr 3. SetPixel called

        // Sample direction
        double xSample = (m_px + camera.pixelWidth * x1);
        double ySample = (m_py + camera.pixelHeight * x2);

        dir.combine3(camera.Z, (float)xSample, camera.X, (float)ySample, camera.Y);
        double distPixel2 = dir.norm2();
        double distPixel = Math.sqrt(distPixel2);
        dir.inverseScaledCopy((float)distPixel, dir, Numeric.EPSILON_FLOAT);

        double cosPixel = Math.abs(camera.Z.dotProduct(dir));

        double pdfDir = ((1.0 / (camera.pixelWidth * camera.pixelHeight)) * // 1 / Area pixel
                         (distPixel2 / cosPixel));  // Spherical angle measure

        // Determine ray type
        thisNode.m_rayType = PathRayType.STARTS;
        newNode.m_inBsdf = thisNode.m_outBsdf; // Camera can be placed in a medium

        // Transfer
        if ( !sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, dir, pdfDir) ) {
            thisNode.m_rayType = PathRayType.STOPS;
            return false;
        }

        // "Bsdf" in thisNode

        // Potential is one for all directions through a pixel
        thisNode.m_bsdfEval.setMonochrome(1.0f);

        // Make sure evaluation of eye components always includes the diff ref.
        thisNode.m_bsdfComp.Clear();
        thisNode.m_bsdfComp.Fill(thisNode.m_bsdfEval, (byte)BsdfComponent.BRDF_DIFFUSE_COMPONENT);

        // Component propagation
        thisNode.m_usedComponents = 0; // the eye...
        newNode.m_accUsedComponents = (byte)(thisNode.m_accUsedComponents | thisNode.m_usedComponents);

        newNode.accumulatedRussianRouletteFactors = thisNode.accumulatedRussianRouletteFactors; // No russian roulette

        return true;
    }

    // Set pixel : sets the current pixel. This pixel will be sampled
    public void SetPixel(Camera defaultCamera, int nx, int ny, Camera camera) {
        if ( camera == null ) {
            // Primary camera
            camera = defaultCamera;
        }

        m_px = -camera.pixelWidth * (double)camera.xSize / 2.0 + (double)nx * camera.pixelWidth;
        m_py = -camera.pixelHeight * (double)camera.ySize / 2.0 + (double)ny * camera.pixelHeight;
    }

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
        double dist2;
        double dist;
        double cosA;
        double cosB;
        double localPdf;
        Vector3D outDir = new Vector3D();

        // More efficient with extra params?
        outDir.subtraction(newNode.m_hit.getPoint(), thisNode.m_hit.getPoint());
        dist2 = outDir.norm2();
        dist = Math.sqrt(dist2);
        outDir.inverseScaledCopy((float)dist, outDir, Numeric.EPSILON_FLOAT);

        // pdf = 1 / A_pixel transformed to area measure

        cosA = thisNode.m_normal.dotProduct(outDir);

        // pdf = 1/APixel * (r^2 / cos(dir, eyeNormal) * (cos(dir, patchNormal) / d^2)
        //                 |__> to spherical.angle           |__> to area on patch

        // Three cosines : r^2 / cos = 1 / cos^3 since r is length
        // of viewing ray to the pixel.
        localPdf = 1.0 / (camera.pixelHeight * camera.pixelWidth * cosA * cosA * cosA);

        cosB = -newNode.m_normal.dotProduct(outDir);
        localPdf = localPdf * cosB / dist2;

        if ( pdf != null && pdf.length > 0 ) {
            pdf[0] = localPdf;
        }
        if ( pdfRR != null && pdfRR.length > 0 ) {
            pdfRR[0] = 1.0;
        }

        return localPdf;
    }
}
