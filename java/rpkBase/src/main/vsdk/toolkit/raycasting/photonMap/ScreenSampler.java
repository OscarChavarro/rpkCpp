/**
Samples a random point on the view screen and traces the viewing ray.
*/

package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.color.ColorRgb;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.raycasting.raytracing.Sampler;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;

public class ScreenSampler extends Sampler {
    /**
newNode gets filled, others may change
*/
    @Override
    public boolean
    sample(
        Camera camera,
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneSceneQueryWorkspace,
        Background sceneBackground,
        SimpleRaytracingPathNode prevNode,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        double x1,
        double x2,
        boolean doRR,
        byte flags)
    {
        Vector3Dd dir = new Vector3Dd();

        // Pre-condition2: 1. thisNode == eye 2. prevNode == nullptr 3. SetPixel called

        // Sample direction
        double xSample = camera.pixelWidth * (double)camera.xSize * (-0.5 + x1);
        double ySample = camera.pixelHeight * (double)camera.ySize * (-0.5 + x2);

        dir = camera.Z.add(camera.X.multiply((double)xSample)).add(camera.Y.multiply((double)ySample));
        double distScreen2 = dir.lengthSquared();
        double distScreen = Math.sqrt(distScreen2);
        dir = dir.multiplyByInverse((double)distScreen, Numeric.EPSILON);

        double cosScreen = Math.abs(camera.Z.dotProduct(dir));

        double pdfDir = ((1.0 / (camera.pixelWidth * (double)camera.xSize *
                                camera.pixelHeight * (double)camera.ySize)) * // 1 / Area pixel
                         (distScreen2 / cosScreen));  // Spherical angle measure

        // Determine ray type
        thisNode.m_rayType = PathRayType.STARTS;
        newNode.m_inBsdf = thisNode.m_outBsdf; // Camera can be placed in a medium

        // Transfer
        if ( !sampleTransfer(sceneQuery, sceneSceneQueryWorkspace, sceneBackground, thisNode, newNode, dir, pdfDir) ) {
            thisNode.m_rayType = PathRayType.STOPS;
            return false;
        }

        // "Bsdf" in thisNode

        // Potential is one for all directions through a pixel
        thisNode.m_bsdfEval = new ColorRgb(1.0, 1.0, 1.0);

        // Make sure evaluation of eye components always includes the diff ref.
        thisNode.m_bsdfComp.Clear();
        thisNode.m_bsdfComp.Fill(thisNode.m_bsdfEval, (byte)BsdfComponent.BRDF_DIFFUSE_COMPONENT);

        // Component propagation
        thisNode.m_usedComponents = 0; // The eye...
        newNode.m_accUsedComponents = (byte)(thisNode.m_accUsedComponents | thisNode.m_usedComponents);
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
        double dist2;
        double dist;
        double cosA;
        double cosB;
        double pdf;
        Vector3Dd outDir = new Vector3Dd();

        // More efficient with extra params?
        outDir = newNode.m_hit.getPoint().subtract(thisNode.m_hit.getPoint());
        dist2 = outDir.lengthSquared();
        dist = Math.sqrt(dist2);
        outDir = outDir.multiplyByInverse((double)dist, Numeric.EPSILON);

        // probabilityDensityFunction = 1 / A_screen transformed to area measure
        cosA = thisNode.m_normal.dotProduct(outDir);

        // probabilityDensityFunction = 1/Apix * (r^2 / cos(dir, eyeNormal) * (cos(dir, patchNormal) / d^2)
        //                 |__> to spherical angle           |__> to area on patch

        // Three cosines : r^2 / cos = 1 / cos^3 since r is length
        // of viewing ray to the screen.
        pdf = 1.0 /
            (camera.pixelHeight * (double)camera.ySize * camera.pixelWidth * (double)camera.xSize * cosA * cosA * cosA);

        cosB = -newNode.m_normal.dotProduct(outDir);
        pdf = pdf * cosB / dist2;

        if ( probabilityDensityFunction != null && probabilityDensityFunction.length > 0 ) {
            probabilityDensityFunction[0] = pdf;
        }
        if ( probabilityDensityFunctionRR != null && probabilityDensityFunctionRR.length > 0 ) {
            probabilityDensityFunctionRR[0] = 1.0;
        }

        return pdf;
    }
}
