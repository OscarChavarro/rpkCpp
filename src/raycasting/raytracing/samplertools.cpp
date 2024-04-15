#include "common/error.h"
#include "skin/Patch.h"
#include "QMC/nied31.h"
#include "raycasting/raytracing/samplertools.h"

void
CSamplerConfig::init(bool useQMC, int qmcDepth) {
    m_useQMC = useQMC;
    m_qmcDepth = qmcDepth;

    if ( m_useQMC ) {
        m_qmcSeed = new unsigned[m_qmcDepth];

        for ( int i = 0; i < m_qmcDepth; i++ ) {
            m_qmcSeed[i] = lrand48();
            printf("Seed %i\n", m_qmcSeed[i]);

            // Every possible path depth gets its own qmc seed to prevent
            // correlation.
        }
    } else {
        m_qmcSeed = nullptr;
    }
}

void
CSamplerConfig::getRand(int depth, double *x_1, double *x_2) const {
    if ( !m_useQMC || depth >= m_qmcDepth ) {
        *x_1 = drand48();
        *x_2 = drand48();
    } else {
        // Niederreiter
        if ( depth == 0 ) {
            *x_1 = drand48();
            *x_2 = drand48();
        } else if ( depth == 1 ) {
            unsigned *nrs = Nied31(m_qmcSeed[1]++);
            *x_1 = nrs[0] * RECIP;
            *x_2 = nrs[1] * RECIP;
        } else if ( depth == 2 ) {
            // unsigned *nrs = Nied(m_qmcSeed[1]++);
            *x_1 = drand48(); // nrs[2] * RECIP;
            *x_2 = drand48(); // nrs[3] * RECIP;
        } else {
            printf("Hmmmm MD %i D%i\n", m_qmcDepth, depth);
            *x_1 = drand48();
            *x_2 = drand48();
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
SimpleRaytracingPathNode *
CSamplerConfig::traceNode(
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *nextNode,
    double x1,
    double x2,
    BSDF_FLAGS flags) const
{
    SimpleRaytracingPathNode *lastNode;

    if ( nextNode == nullptr ) {
        nextNode = new SimpleRaytracingPathNode;
    }

    lastNode = nextNode->previous();

    if ( lastNode == nullptr ) {
        // Fill in first node
        if ( !pointSampler->sample(sceneVoxelGrid, sceneBackground, nullptr, nullptr, nextNode, x1, x2) ) {
            logWarning("CSamplerConfig::traceNode", "Point sampler failed");
            return nullptr;
        }
    } else if ( lastNode->m_depth == 0 ) {
        // Fill in second node : dir sampler
        if ( (lastNode->m_depth + 1) < maxDepth ) {
            if ( !dirSampler->sample(sceneVoxelGrid, sceneBackground, nullptr, lastNode, nextNode, x1, x2) ) {
                // No point !
                lastNode->m_rayType = STOPS;
                return nullptr;
            }
        } else {
            lastNode->m_rayType = STOPS;
            return nullptr;
        }
    } else {
        // In the middle of a path
        if ( (lastNode->m_depth + 1) < maxDepth ) {
            if ( !surfaceSampler->sample(
                    sceneVoxelGrid,
                    sceneBackground,
                    lastNode->previous(),
                    lastNode,
                    nextNode,
                    x1,
                    x2,
                    lastNode->m_depth >= minDepth,
                    flags) ) {
                lastNode->m_rayType = STOPS;
                return nullptr;
            }
        } else {
            lastNode->m_rayType = STOPS;
            return nullptr;
        }
    }

    // We're sure that nextNode contains a new sampled point
    if ( nextNode->m_depth > 0 ) {
        // Lights and cam reside in vacuum
        nextNode->assignBsdfAndNormal();
    }

    return nextNode;
}

SimpleRaytracingPathNode *
CSamplerConfig::tracePath(
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *nextNode,
    BSDF_FLAGS flags)
{
    double x1;
    double x2;

    if ( nextNode == nullptr || nextNode->previous() == nullptr ) {
        getRand(0, &x1, &x2);
    } else {
        getRand(nextNode->previous()->m_depth + 1, &x1, &x2);
    }

    nextNode = traceNode(sceneVoxelGrid, sceneBackground, nextNode, x1, x2, flags);

    if ( nextNode != nullptr ) {
        nextNode->ensureNext();

        // Recursive call
        tracePath(sceneVoxelGrid, sceneBackground, nextNode->next(), flags);
    }

    return nextNode;
}

double
pathNodeConnect(
        SimpleRaytracingPathNode *nodeX,
        SimpleRaytracingPathNode *nodeY,
        CSamplerConfig *eyeConfig,
        CSamplerConfig *lightConfig,
        CONNECT_FLAGS flags,
        BSDF_FLAGS bsdfFlagsE,
        BSDF_FLAGS bsdfFlagsL,
        Vector3D *pDirEl)
{
    SimpleRaytracingPathNode *nodeEP; // previous nodes
    SimpleRaytracingPathNode *nodeLP;
    double pdf;
    double pdfRR;
    double dist2;
    double dist;
    double geom;
    Vector3D dirLE;
    Vector3D dirEL;

    vectorSubtract(nodeY->m_hit.point, nodeX->m_hit.point, dirEL);
    dist2 = vectorNorm2(dirEL);
    dist = std::sqrt(dist2);
    vectorScaleInverse((float) dist, dirEL, dirEL);
    vectorScale(-1, dirEL, dirLE);

    if ( pDirEl ) vectorCopy(dirEL, *pDirEl);

    // Always test the FOLLOW NEXT flags!
    nodeEP = nodeX->previous();
    nodeLP = nodeY->previous();

    if ( flags & CONNECT_EL ) {
        // pdf (E->L)

        // Determine the sampler
        if ( nodeX->m_depth < (eyeConfig->maxDepth - 1) ) {
            if ( nodeEP == nullptr ) {
                // nodeE is the eye -> use the pixel sampler !
                // -- Which pixel?
                pdf = eyeConfig->dirSampler->evalPDF(nodeX, nodeY);
                pdfRR = 1.0;
            } else {
                eyeConfig->surfaceSampler->evalPDF(nodeX, nodeY, bsdfFlagsE, &pdf,
                                                   &pdfRR);
            }
        } else {
            pdf = 0.0;
            pdfRR = 0.0; // Light point cannot be generated from eye sub-path
        }

        nodeY->m_pdfFromNext = pdf;

        PNAN(nodeY->m_pdfFromNext);

        nodeY->m_rrPdfFromNext = pdfRR;

        if ( (flags & FILL_OTHER_PDF) && (nodeLP != nullptr) ) {
            if ( nodeX->m_depth < (eyeConfig->maxDepth - 2) ) {
                lightConfig->surfaceSampler->EvalPDFPrev(nodeX, nodeY, nodeLP,
                                                         bsdfFlagsE,
                                                         &pdf, &pdfRR);
            } else {
                // nodeLP is too many bounces from the light
                pdf = 0.0;
                pdfRR = 0.0;
            }

            nodeLP->m_pdfFromNext = pdf;
            nodeLP->m_rrPdfFromNext = pdfRR;
        }

    }


    if ( flags & CONNECT_LE ) {
        // pdf (L->E)

        if ( nodeY->m_depth < (lightConfig->maxDepth - 1) ) {
            // Determine the sampler

            if ( nodeLP == nullptr ) {
                // nodeE is the light point -> use the dir sampler!
                pdf = lightConfig->dirSampler->evalPDF(nodeY, nodeX);
                pdfRR = 1.0;
            } else {
                lightConfig->surfaceSampler->evalPDF(nodeY, nodeX, bsdfFlagsL, &pdf,
                                                     &pdfRR);
            }
        } else {
            pdf = 0.0;
            pdfRR = 0.0; // Eye point cannot be generated from light sub-path
        }

        nodeX->m_pdfFromNext = pdf;
        PNAN(nodeX->m_pdfFromNext);

        nodeX->m_rrPdfFromNext = pdfRR;

        if ( (flags & FILL_OTHER_PDF) && (nodeEP != nullptr) ) {
            if ( nodeY->m_depth < (lightConfig->maxDepth - 2) ) {
                lightConfig->surfaceSampler->EvalPDFPrev(nodeY, nodeX, nodeEP,
                                                         bsdfFlagsL,
                                                         &pdf, &pdfRR);
            } else {
                // nodeEP is too many bounces from the light
                pdf = 0.0;
                pdfRR = 0.0;
            }

            nodeEP->m_pdfFromNext = pdf;
            nodeEP->m_rrPdfFromNext = pdfRR;
        }
    }

    // The bsdf in E and L are ALWAYS filled in

    // bsdf(EP->E->L)
    if ( nodeEP == nullptr ) {
        // Eye
        nodeX->m_bsdfEval.setMonochrome(1.0);
        nodeX->m_bsdfComp.Clear();
        nodeX->m_bsdfComp.Fill(nodeX->m_bsdfEval, BRDF_DIFFUSE_COMPONENT);
    } else {
        nodeX->m_bsdfEval =
                eyeConfig->surfaceSampler->DoBsdfEval(nodeX->m_useBsdf,
                                                      &nodeX->m_hit,
                                                      nodeX->m_inBsdf, nodeX->m_outBsdf,
                                                      &nodeX->m_inDirF, &dirEL,
                                                      bsdfFlagsE,
                                                      &nodeX->m_bsdfComp);
    }

    // bsdf(LP->L->E)  (reciprocity assumed !)
    if ( nodeLP == nullptr ) {
        // nodeL is  light source
        nodeY->m_bsdfEval = edfEval(nodeY->m_hit.material->edf,
                                    &nodeY->m_hit,
                                    &dirLE,
                                    bsdfFlagsL, nullptr);
        nodeY->m_bsdfComp.Clear();
        nodeY->m_bsdfComp.Fill(nodeY->m_bsdfEval, BRDF_DIFFUSE_COMPONENT);
    } else {
        nodeY->m_bsdfEval =
                lightConfig->surfaceSampler->DoBsdfEval(nodeY->m_useBsdf,
                                                        &nodeY->m_hit,
                                                        nodeY->m_inBsdf, nodeY->m_outBsdf,
                                                        &nodeY->m_inDirF, &dirLE,
                                                        bsdfFlagsL,
                                                        &nodeY->m_bsdfComp);
    }


    double cosA = -vectorDotProduct(dirEL, nodeY->m_normal);
    geom = std::fabs(cosA * vectorDotProduct(nodeX->m_normal, dirEL) / dist2);

    // Geom is always positive !  Visibility checking cannot be done
    // by checking cos signs because materials can be refractive.

    return geom;
}
