package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.raycasting.raytracing.NextEventSampler;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayHit;

public class UniformLightSampler extends NextEventSampler {
    private LightList lightList;
    private LightListIterator iterator;
    private Patch currentPatch;
    private boolean unitsActive;

    public UniformLightSampler(LightList inLightList) {
        lightList = inLightList;

        // if(gLightList)
        // iterator = new CLightList_Iter(*gLightList);
        iterator = null;
        currentPatch = null;
        unitsActive = false;
    }

    @Override
    public boolean ActivateFirstUnit() {
        if ( lightList == null ) {
            return false;
        }

        if ( iterator == null ) {
            iterator = new LightListIterator(lightList);
        }

        currentPatch = iterator.First(lightList);

        if ( currentPatch != null ) {
            unitsActive = true;
            return true;
        } else {
            return false;
        }
    }

    @Override
    public boolean ActivateNextUnit() {
        currentPatch = iterator.Next();
        return (currentPatch != null);
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
        if ( unitsActive ) {
            if ( currentPatch != null ) {
                light = currentPatch;
                pdfLight = 1.0;
            } else {
                Logger.warning("sample Unit Light Node", "No valid light selected");
                return false;
            }
        } else {
            if ( lightList == null ) {
                Logger.warning("FillLightNode", "No light list available");
                return false;
            }
            double[] localX1 = new double[] {x1};
            double[] outPdfLight = new double[] {0.0};
            light = lightList.sample(localX1, outPdfLight);
            x1 = localX1[0];
            pdfLight = outPdfLight[0];

            if ( light == null ) {
                Logger.warning("FillLightNode", "No light found");
                return false;
            }
        }

        // Choose point (uniform for real, sampled for background)
        if ( light.hasZeroVertices() ) {
            double[] pdf = new double[] {0.0};
            Vector3D dir = new Vector3D(0.0f, 0.0f, 0.0f);

            if ( light.material.getEdf() != null ) {
                dir = light.material.getEdf().phongEdfSample(thisNode.m_hit, flags & 0xFF, x1, x2, null, pdf);
            }

            point.subtraction(thisNode.m_hit.getPoint(), dir); // Fake hit at distance 1!

            newNode.m_hit.init(light, point, null, light.material);

            // Fill in directions
            newNode.m_inDirT.scaledCopy(-1.0f, dir);
            newNode.m_inDirF.copy(dir);
            newNode.m_normal.copy(dir);

            pdfPoint = pdf[0];   // every direction corresponds to 1 point
        } else {
            light.uniformPoint(x1, x2, point);
            pdfPoint = 1.0 / light.area;

            // Fake a hit record
            newNode.m_hit.init(light, point, light.normal, light.material);
            Vector3D normal = new Vector3D();
            newNode.m_hit.shadingNormal(normal);
            newNode.m_hit.setNormal(normal);
            newNode.m_normal.copy(newNode.m_hit.getNormal());
        }

        // inDir's not filled in
        newNode.m_pdfFromPrev = pdfLight * pdfPoint;

        // Component propagation
        newNode.m_accUsedComponents = 0; // Light has no accumulated comps.

        newNode.accumulatedRussianRouletteFactors = 1.0;

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

        // The light point is in NEW NODE!
        if ( unitsActive ) {
            localPdf = 1.0;
        } else {
            if ( lightList == null ) {
                return 0.0;
            }
            Vector3D position = newNode.m_hit.getPoint();
            localPdf = lightList.evalPdf(newNode.m_hit.getPatch(), position);
        }

        // Prob for choosing this point(/direction)
        if ( newNode.m_hit.getPatch().hasZeroVertices() ) {
            // virtual patch has no area!
            // choosing a point == choosing a dir --> use pdf from evalEdf
            if ( newNode.m_hit.getPatch().material.getEdf() == null ) {
                pdfDir = 0.0;
            } else {
                double[] outPdfDir = new double[] {0.0};
                newNode.m_hit.getPatch().material.getEdf().phongEdfEval(
                    (RayHit)null,
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
