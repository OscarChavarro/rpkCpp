/**
A configuration structure, that determines the sampling
procedure of a monte carlo ray tracing like algorithm
*/

package vsdk.toolkit.raycasting.raytracing;

import java.util.concurrent.ThreadLocalRandom;

import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.Niederreiter31;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;

public class SamplerConfig {
    public Sampler pointSampler;  // Samples first point
    public Sampler dirSampler; // Samples first direction
    public SurfaceSampler surfaceSampler; // Samples on surfaces
    public NextEventSampler neSampler; // Samples last point separately for next event

    public boolean m_useQMC;
    public int m_qmcDepth;
    public long[] m_qmcSeed;

    public int minDepth;
    public int maxDepth;

    public SamplerConfig() {
        clearVars();
        init();
    }

    public void clearVars() {
        pointSampler = null;
        dirSampler = null;
        surfaceSampler = null;
        neSampler = null;
        m_qmcSeed = null;
    }

    public void releaseVars() {
        pointSampler = null;
        dirSampler = null;
        surfaceSampler = null;
        neSampler = null;
        m_qmcSeed = null;
    }

    public void init() {
        init(false, 0);
    }

    public void init(boolean useQMC, int qmcDepth) {
        m_useQMC = useQMC;
        m_qmcDepth = qmcDepth;

        if ( m_useQMC ) {
            m_qmcSeed = new long[m_qmcDepth];

            for ( int i = 0; i < m_qmcDepth; i++ ) {
                m_qmcSeed[i] = ThreadLocalRandom.current().nextLong(0L, Integer.MAX_VALUE);
                System.out.printf("Seed %d\n", m_qmcSeed[i]);

                // Every possible path depth gets its own qmc seed to prevent correlation
            }
        } else {
            m_qmcSeed = null;
        }
    }

    // Generate two random numbers. Depth needed for QMC sampling
    public void getRand(int depth, double[] x1, double[] x2) {
        if ( x1 == null || x1.length == 0 || x2 == null || x2.length == 0 ) {
            return;
        }

        if ( !m_useQMC || depth >= m_qmcDepth ) {
            x1[0] = Math.random();
            x2[0] = Math.random();
        } else {
            // Niederreiter
            if ( depth == 0 || depth == 2 ) {
                x1[0] = Math.random();
                x2[0] = Math.random();
            } else if ( depth == 1 ) {
                long[] nrs = Niederreiter31.niederreiter31(m_qmcSeed[1]++);
                x1[0] = nrs[0] * Niederreiter31.RECIP;
                x2[0] = nrs[1] * Niederreiter31.RECIP;
            } else {
                System.out.printf("Hmmmm MD %d D%d\n", m_qmcDepth, depth);
                x1[0] = Math.random();
                x2[0] = Math.random();
            }
        }
    }

    /**
    Trace a new node, given two random numbers
    The correct sampler is chosen depending on the current
    path depth.
    RETURNS:
      if sampling ok: nextNode or a newly allocated node if nextNode == nullptr
      if sampling fails: nullptr
    */
    public SimpleRaytracingPathNode
    traceNode(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        SimpleRaytracingPathNode nextNode,
        double x1,
        double x2,
        byte flags)
    {
        SimpleRaytracingPathNode lastNode;

        if ( nextNode == null ) {
            nextNode = new SimpleRaytracingPathNode();
        }

        lastNode = nextNode.previous();

        if ( lastNode == null ) {
            // Fill in first node
            if ( !pointSampler.sample(camera, sceneVoxelGrid, sceneBackground, null, null, nextNode, x1, x2) ) {
                Error.warning("SamplerConfig::traceNode", "Point sampler failed");
                return null;
            }
        } else if ( lastNode.m_depth == 0 ) {
            // Fill in second node : dir sampler
            if ( (lastNode.m_depth + 1) < maxDepth ) {
                if ( !dirSampler.sample(camera, sceneVoxelGrid, sceneBackground, null, lastNode, nextNode, x1, x2) ) {
                    // No point !
                    lastNode.m_rayType = PathRayType.STOPS;
                    return null;
                }
            } else {
                lastNode.m_rayType = PathRayType.STOPS;
                return null;
            }
        } else {
            // In the middle of a path
            if ( (lastNode.m_depth + 1) < maxDepth ) {
                if ( !surfaceSampler.sample(
                    camera,
                    sceneVoxelGrid,
                    sceneBackground,
                    lastNode.previous(),
                    lastNode,
                    nextNode,
                    x1,
                    x2,
                    lastNode.m_depth >= minDepth,
                    flags) ) {
                    lastNode.m_rayType = PathRayType.STOPS;
                    return null;
                }
            } else {
                lastNode.m_rayType = PathRayType.STOPS;
                return null;
            }
        }

        // We're sure that nextNode contains a new sampled point
        if ( nextNode.m_depth > 0 ) {
            // Lights and cam reside in vacuum
            nextNode.assignBsdfAndNormal();
        }

        return nextNode;
    }

    public SimpleRaytracingPathNode
    tracePath(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        SimpleRaytracingPathNode nextNode,
        byte flags)
    {
        double[] x1 = new double[1];
        double[] x2 = new double[1];

        if ( nextNode == null || nextNode.previous() == null ) {
            getRand(0, x1, x2);
        } else {
            getRand(nextNode.previous().m_depth + 1, x1, x2);
        }

        nextNode = traceNode(camera, sceneVoxelGrid, sceneBackground, nextNode, x1[0], x2[0], flags);

        boolean continueTrace = nextNode != null;
        if ( continueTrace && sceneBackground != null && nextNode.ends() ) {
            continueTrace = false;
        }

        if ( continueTrace ) {
            nextNode.ensureNext();

            // Recursive call
            tracePath(camera, sceneVoxelGrid, sceneBackground, nextNode.next(), flags);
        }

        return nextNode;
    }

    public SimpleRaytracingPathNode
    tracePath(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        SimpleRaytracingPathNode nextNode)
    {
        return tracePath(camera, sceneVoxelGrid, sceneBackground, nextNode, Sampler.BSDF_ALL_COMPONENTS);
    }

    /**
    pathNodeConnect : this is a flexible function for connecting
    path nodes.

    IN : 2 nodes are needed. nodeE and nodeL are going to be connected
         Visibility is NOT checked !

         nodeE must be the node in an EYE sub-path
         nodeL must be the node in a LIGHT sub-path

         config determines the samplers used to generate the paths.
         These samplers are needed for pdf evaluations

         Flags determine what needs to be computed. See the flag definitions

    OUT : geometry factor is returned (not filled in cause it would overwrite
          other geometries !)
    */
    public static double
    pathNodeConnect(
        Camera camera,
        SimpleRaytracingPathNode nodeX,
        SimpleRaytracingPathNode nodeY,
        SamplerConfig eyeConfig,
        SamplerConfig lightConfig,
        int flags,
        byte bsdfFlagsE,
        byte bsdfFlagsL,
        Vector3D pDirEl)
    {
        SimpleRaytracingPathNode nodeEP; // previous nodes
        SimpleRaytracingPathNode nodeLP;
        double[] pdf = new double[1];
        double[] pdfRR = new double[1];
        double dist2;
        double dist;
        double geom;
        Vector3D dirLE = new Vector3D();
        Vector3D dirEL = new Vector3D();

        dirEL.subtraction(nodeY.m_hit.getPoint(), nodeX.m_hit.getPoint());
        dist2 = dirEL.norm2();
        dist = Math.sqrt(dist2);
        dirEL.inverseScaledCopy((float)dist, dirEL, Numeric.EPSILON_FLOAT);
        dirLE.scaledCopy(-1.0f, dirEL);

        if ( pDirEl != null ) {
            pDirEl.copy(dirEL);
        }

        // Always test the FOLLOW NEXT flags!
        nodeEP = nodeX.previous();
        nodeLP = nodeY.previous();

        if ( (flags & SampleConnectionFlags.CONNECT_EL) != 0 ) {
            // pdf (E->L)

            // Determine the sampler
            if ( nodeX.m_depth < (eyeConfig.maxDepth - 1) ) {
                if ( nodeEP == null ) {
                    // nodeE is the eye -> use the pixel sampler !
                    // -- Which pixel?
                    pdf[0] = eyeConfig.dirSampler.evalPDF(camera, nodeX, nodeY);
                    pdfRR[0] = 1.0;
                } else {
                    eyeConfig.surfaceSampler.evalPDF(camera, nodeX, nodeY, bsdfFlagsE, pdf, pdfRR);
                }
            } else {
                pdf[0] = 0.0;
                pdfRR[0] = 0.0; // Light point cannot be generated from eye sub-path
            }

            nodeY.m_pdfFromNext = pdf[0];
            nodeY.m_rrPdfFromNext = pdfRR[0];

            if ( ((flags & SampleConnectionFlags.FILL_OTHER_PDF) != 0) && (nodeLP != null) ) {
                if ( nodeX.m_depth < (eyeConfig.maxDepth - 2) ) {
                    lightConfig.surfaceSampler.EvalPDFPrev(nodeX, nodeY, nodeLP,
                                                           bsdfFlagsE,
                                                           pdf, pdfRR);
                } else {
                    // nodeLP is too many bounces from the light
                    pdf[0] = 0.0;
                    pdfRR[0] = 0.0;
                }

                nodeLP.m_pdfFromNext = pdf[0];
                nodeLP.m_rrPdfFromNext = pdfRR[0];
            }
        }

        if ( (flags & SampleConnectionFlags.CONNECT_LE) != 0 ) {
            // pdf (L->E)

            if ( nodeY.m_depth < (lightConfig.maxDepth - 1) ) {
                // Determine the sampler

                if ( nodeLP == null ) {
                    // nodeE is the light point -> use the dir sampler!
                    pdf[0] = lightConfig.dirSampler.evalPDF(camera, nodeY, nodeX);
                    pdfRR[0] = 1.0;
                } else {
                    lightConfig.surfaceSampler.evalPDF(camera, nodeY, nodeX, bsdfFlagsL, pdf, pdfRR);
                }
            } else {
                pdf[0] = 0.0;
                pdfRR[0] = 0.0; // Eye point cannot be generated from light sub-path
            }

            nodeX.m_pdfFromNext = pdf[0];
            nodeX.m_rrPdfFromNext = pdfRR[0];

            if ( ((flags & SampleConnectionFlags.FILL_OTHER_PDF) != 0) && (nodeEP != null) ) {
                if ( nodeY.m_depth < (lightConfig.maxDepth - 2) ) {
                    lightConfig.surfaceSampler.EvalPDFPrev(nodeY, nodeX, nodeEP,
                                                           bsdfFlagsL,
                                                           pdf, pdfRR);
                } else {
                    // nodeEP is too many bounces from the light
                    pdf[0] = 0.0;
                    pdfRR[0] = 0.0;
                }

                nodeEP.m_pdfFromNext = pdf[0];
                nodeEP.m_rrPdfFromNext = pdfRR[0];
            }
        }

        // The bsdf in E and L are ALWAYS filled in

        // bsdf(EP->E->L)
        if ( nodeEP == null ) {
            // Eye
            nodeX.m_bsdfEval.setMonochrome(1.0f);
            nodeX.m_bsdfComp.Clear();
            nodeX.m_bsdfComp.Fill(nodeX.m_bsdfEval, (byte)BsdfComponent.BRDF_DIFFUSE_COMPONENT);
        } else {
            nodeX.m_bsdfEval = eyeConfig.surfaceSampler.DoBsdfEval(
                nodeX.m_useBsdf,
                nodeX.m_hit,
                nodeX.m_inBsdf,
                nodeX.m_outBsdf,
                nodeX.m_inDirF,
                dirEL,
                bsdfFlagsE,
                nodeX.m_bsdfComp);
        }

        // bsdf LP->L->E reciprocity assumed!
        if ( nodeLP == null ) {
            // nodeL is  light source
            if ( nodeY.m_hit.getMaterial() == null || nodeY.m_hit.getMaterial().getEdf() == null ) {
                nodeY.m_bsdfEval.clear();
            } else {
                nodeY.m_bsdfEval = nodeY.m_hit.getMaterial().getEdf().phongEdfEval(
                    nodeY.m_hit, dirLE, bsdfFlagsL & 0xFF, null);
            }
            nodeY.m_bsdfComp.Clear();
            nodeY.m_bsdfComp.Fill(nodeY.m_bsdfEval, (byte)BsdfComponent.BRDF_DIFFUSE_COMPONENT);
        } else {
            nodeY.m_bsdfEval = lightConfig.surfaceSampler.DoBsdfEval(
                nodeY.m_useBsdf,
                nodeY.m_hit,
                nodeY.m_inBsdf, nodeY.m_outBsdf,
                nodeY.m_inDirF, dirLE,
                bsdfFlagsL,
                nodeY.m_bsdfComp);
        }

        double cosA = -dirEL.dotProduct(nodeY.m_normal);
        geom = Math.abs(cosA * nodeX.m_normal.dotProduct(dirEL) / dist2);

        // Geom is always positive !  Visibility checking cannot be done
        // by checking cos signs because materials can be refractive.

        return geom;
    }

    public static double
    pathNodeConnect(
        Camera camera,
        SimpleRaytracingPathNode nodeX,
        SimpleRaytracingPathNode nodeY,
        SamplerConfig eyeConfig,
        SamplerConfig lightConfig,
        int flags,
        Vector3D pDirEl)
    {
        return pathNodeConnect(
            camera,
            nodeX,
            nodeY,
            eyeConfig,
            lightConfig,
            flags,
            Sampler.BSDF_ALL_COMPONENTS,
            Sampler.BSDF_ALL_COMPONENTS,
            pDirEl);
    }
}
