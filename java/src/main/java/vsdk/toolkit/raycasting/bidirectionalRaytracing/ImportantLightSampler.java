package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.RayHitFlag;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.raycasting.raytracing.NextEventSampler;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.Patch;

public class ImportantLightSampler extends NextEventSampler {
    private LightList lightList;

    public ImportantLightSampler(LightList inLightList) {
        lightList = inLightList;
    }

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
        double pdfLight;
        double pdfPoint;
        Patch light;
        Vector3D point = new Vector3D();

        // thisNode is NOT used or altered. If you want the nodes connected,
        // use the Connect method of a surface sampler and a light direction
        // sampler. Otherwise, pdf's cannot be calculated
        // Visibility is NOT determined here!
        newNode.m_depth = 0;
        newNode.m_rayType = PathRayType.STOPS;
        newNode.m_useBsdf = null;
        newNode.m_inBsdf = null;
        newNode.m_outBsdf = null;
        newNode.m_G = 1.0;

        // Choose light
        if ( lightList == null ) {
            return false;
        }

        double[] localX1 = new double[] {x1};
        double[] outPdfLight = new double[] {0.0};
        if ( (thisNode.m_hit.getFlags() & RayHitFlag.BACK) != 0 ) {
            if ( thisNode.m_outBsdf == null ) {
                Vector3D invNormal = new Vector3D();

                invNormal.scaledCopy(-1.0f, thisNode.m_normal);

                Vector3D position = thisNode.m_hit.getPoint();
                light = lightList.sampleImportant(position, invNormal, localX1, outPdfLight);
            } else {
                // No (important) light sampling inside a material
                light = null;
            }
        } else {
            if ( thisNode.m_inBsdf == null ) {
                Vector3D position = thisNode.m_hit.getPoint();
                light = lightList.sampleImportant(position, thisNode.m_normal, localX1, outPdfLight);
            } else {
                light = null;
            }
        }

        x1 = localX1[0];
        pdfLight = outPdfLight[0];

        if ( light == null ) {
            // No light found
            return false;
        }

        // Choose point (uniform for real, sampled for background)
        if ( light.hasZeroVertices() ) {
            double[] pdf = new double[] {0.0};
            Vector3D dir = new Vector3D(0.0f, 0.0f, 0.0f);

            if ( light.material.getEdf() != null ) {
                dir = light.material.getEdf().phongEdfSample(null, flags & 0xFF, x1, x2, null, pdf);
            }

            point.addition(thisNode.m_hit.getPoint(), dir);   // fake hit at distance 1!

            newNode.m_hit.init(light, point, null, light.material);

            // fill in directions
            newNode.m_inDirT.scaledCopy(-1.0f, dir);
            newNode.m_inDirF.copy(dir);
            newNode.m_normal.copy(dir);

            pdfPoint = pdf[0]; // Every direction corresponds to 1 point
        } else {
            light.uniformPoint(x1, x2, point);

            pdfPoint = 1.0 / light.area;

            // Light position and value are known now

            // Fake a hit record
            newNode.m_hit.init(light, point, light.normal, light.material);
            Vector3D normal = new Vector3D();
            newNode.m_hit.shadingNormal(normal);
            newNode.m_hit.setNormal(normal);
            newNode.m_normal.copy(newNode.m_hit.getNormal());
        }

        // outDir's, m_G not filled in yet (light direction sampler does this)

        newNode.m_pdfFromPrev = pdfLight * pdfPoint;

        return true;
    }

    @Override
    public double evalPDF(
        Camera camera,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        byte flags,
        double[] pdf,
        double[] pdfRR)
    {
        double localPdf;
        double pdfDir;

        // The light point is in NEW NODE !!
        if ( lightList == null ) {
            return 0.0;
        }
        Vector3D newPosition = newNode.m_hit.getPoint();
        Vector3D thisPosition = thisNode.m_hit.getPoint();
        localPdf = lightList.evalPdfImportant(
            newNode.m_hit.getPatch(),
            newPosition,
            thisPosition,
            thisNode.m_normal);

        // Prob for choosing this point(/direction)
        if ( newNode.m_hit.getPatch().hasZeroVertices() ) {
            // virtual patch has no area!
            // choosing newPosition point == choosing newPosition dir --> use pdf from evalEdf
            if ( newNode.m_hit.getPatch().material.getEdf() == null ) {
                pdfDir = 0.0;
            } else {
                double[] outPdfDir = new double[] {0.0};
                newNode.m_hit.getPatch().material.getEdf().phongEdfEval(
                    null,
                    newNode.m_inDirF,
                    XxdfComponentFlag.DIFFUSE_COMPONENT | XxdfComponentFlag.GLOSSY_COMPONENT | XxdfComponentFlag.SPECULAR_COMPONENT,
                    outPdfDir);
                pdfDir = outPdfDir[0];
            }

            localPdf *= pdfDir;
        } else {
            // Normal patch, choosing point uniformly
            if ( localPdf >= Numeric.EPSILON && newNode.m_hit.getPatch().area > Numeric.EPSILON ) {
                localPdf = localPdf / newNode.m_hit.getPatch().area;
            } else {
                localPdf = 0.0;
            }
        }

        if ( pdf != null && pdf.length > 0 ) {
            pdf[0] = localPdf;
        }
        if ( pdfRR != null && pdfRR.length > 0 ) {
            pdfRR[0] = 1.0;
        }

        return localPdf;
    }
}
