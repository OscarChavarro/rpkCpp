#include <cmath>

#include "common/error.h"
#include "skin/Patch.h"
#include "material/edf.h"
#include "common/Ray.h"
#include "QMC/nied31.h"
#include "raycasting/raytracing/samplertools.h"

void
CSamplerConfig::Init(bool useQMC, int qmcDepth) {
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

void CSamplerConfig::GetRand(int depth, double *x_1, double *x_2) {
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


// TraceNode: trace a new node, given two random numbers
// The correct sampler is chosen depending on the current
// path depth.
// RETURNS: 
//   if sampling ok: nextNode or a newly allocated node if nextNode == nullptr
//   if sampling fails: nullptr

CPathNode *CSamplerConfig::TraceNode(CPathNode *nextNode,
                                     double x_1, double x_2,
                                     BSDFFLAGS flags) {
    CPathNode *lastNode;

    if ( nextNode == nullptr ) {
        nextNode = new CPathNode;
    }

    lastNode = nextNode->Previous();

    if ( lastNode == nullptr ) {
        // Fill in first node

        if ( !pointSampler->Sample(nullptr, nullptr, nextNode, x_1, x_2)) {
            logWarning("CSamplerConfig::TraceNode", "Point sampler failed");
            return nullptr;
        }
    } else if ( lastNode->m_depth == 0 ) {
        // Fill in second node : dir sampler

        if ((lastNode->m_depth + 1) < maxDepth ) {
            if ( !dirSampler->Sample(nullptr, lastNode, nextNode, x_1, x_2)) {
                // No point !
                lastNode->m_rayType = Stops;
                return nullptr;
            }
        } else {
            lastNode->m_rayType = Stops;
            return nullptr;
        }
    } else {
        // In the middle of a path
        if ((lastNode->m_depth + 1) < maxDepth ) {
            if ( !surfaceSampler->Sample(lastNode->Previous(), lastNode, nextNode,
                                         x_1, x_2,
                                         lastNode->m_depth >= minDepth,
                                         flags)) {
                lastNode->m_rayType = Stops;
                return nullptr;
            }
        } else {
            lastNode->m_rayType = Stops;
            return nullptr;
        }
    }

    // We're sure that nextNode contains a new sampled point

    if ( nextNode->m_depth > 0 ) {
        nextNode->AssignBsdfAndNormal();
    } // Lights and cam reside in vacuum

    return nextNode;
}


CPathNode *CSamplerConfig::TracePath(CPathNode *nextNode,
                                     BSDFFLAGS flags) {
    double x_1, x_2;

    if ( nextNode == nullptr || nextNode->Previous() == nullptr ) {
        GetRand(0, &x_1, &x_2);
    } else {
        GetRand(nextNode->Previous()->m_depth + 1, &x_1, &x_2);
    }

    nextNode = TraceNode(nextNode, x_1, x_2, flags);

    if ( nextNode != nullptr ) {
        nextNode->EnsureNext();

        // Recursive call
        TracePath(nextNode->Next(), flags);
    }

    return nextNode;
}


/**** Utility routines ******/

int epcount = 0;

double PathNodeConnect(CPathNode *nodeE,
                       CPathNode *nodeL,
                       CSamplerConfig *eyeConfig,
                       CSamplerConfig *lightConfig,
                       CONNECTFLAGS flags,
                       BSDFFLAGS bsdfFlagsE,
                       BSDFFLAGS bsdfFlagsL,
                       Vector3D *p_dirEL) {
    CPathNode *nodeEP, *nodeLP; // previous nodes
    double pdf, pdfRR, dist2, dist, geom;
    Vector3D dirLE, dirEL;

    //epcount++;
    //printf("ep %i\n", epcount);

    VECTORSUBTRACT(nodeL->m_hit.point, nodeE->m_hit.point, dirEL);
    dist2 = VECTORNORM2(dirEL);
    dist = sqrt(dist2);
    VECTORSCALEINVERSE(dist, dirEL, dirEL);
    VECTORSCALE(-1, dirEL, dirLE);

    if ( p_dirEL ) VECTORCOPY(dirEL, *p_dirEL);

    /* Always test the FOLLOW NEXT flags !!!! */

    nodeEP = nodeE->Previous();
    nodeLP = nodeL->Previous();

    if ( flags & CONNECT_EL ) {
        // pdf (E->L)

        // Determine the sampler

        if ( nodeE->m_depth < (eyeConfig->maxDepth - 1)) {
            if ( nodeEP == nullptr ) {
                // nodeE is the eye -> use the pixel sampler !
                /* -- Which pixel ?? -- */
                pdf = eyeConfig->dirSampler->EvalPDF(nodeE, nodeL);
                pdfRR = 1.0;
            } else {
                eyeConfig->surfaceSampler->EvalPDF(nodeE, nodeL, bsdfFlagsE, &pdf,
                                                   &pdfRR);
            }
        } else {
            pdf = 0.0;
            pdfRR = 0.0; // Light point cannot be generated from eye subpath
        }

        nodeL->m_pdfFromNext = pdf;

        PNAN(nodeL->m_pdfFromNext);

        nodeL->m_rrPdfFromNext = pdfRR;

        if ((flags & FILL_OTHER_PDF) && (nodeLP != nullptr)) {
            if ( nodeE->m_depth < (eyeConfig->maxDepth - 2)) {
                lightConfig->surfaceSampler->EvalPDFPrev(nodeE, nodeL, nodeLP,
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

        if ( nodeL->m_depth < (lightConfig->maxDepth - 1)) {
            // Determine the sampler

            if ( nodeLP == nullptr ) {
                // nodeE is the lightpoint -> use the dir sampler !
                pdf = lightConfig->dirSampler->EvalPDF(nodeL, nodeE);
                pdfRR = 1.0;
            } else {
                lightConfig->surfaceSampler->EvalPDF(nodeL, nodeE, bsdfFlagsL, &pdf,
                                                     &pdfRR);
            }
        } else {
            pdf = 0.0;
            pdfRR = 0.0; // Eye point cannot be generated from light subpath
        }

        nodeE->m_pdfFromNext = pdf;
        PNAN(nodeE->m_pdfFromNext);

        nodeE->m_rrPdfFromNext = pdfRR;


        if ((flags & FILL_OTHER_PDF) && (nodeEP != nullptr)) {
            if ( nodeL->m_depth < (lightConfig->maxDepth - 2)) {
                lightConfig->surfaceSampler->EvalPDFPrev(nodeL, nodeE, nodeEP,
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
        colorSetMonochrome(nodeE->m_bsdfEval, 1.0);
        nodeE->m_bsdfComp.Clear();
        nodeE->m_bsdfComp.Fill(nodeE->m_bsdfEval, BRDF_DIFFUSE_COMPONENT);
    } else {
        nodeE->m_bsdfEval =
                eyeConfig->surfaceSampler->DoBsdfEval(nodeE->m_useBsdf,
                                                      &nodeE->m_hit,
                                                      nodeE->m_inBsdf, nodeE->m_outBsdf,
                                                      &nodeE->m_inDirF, &dirEL,
                                                      bsdfFlagsE,
                                                      &nodeE->m_bsdfComp);
    }

    // bsdf(LP->L->E)  (reciprocity assumed !)

    if ( nodeLP == nullptr ) {
        // nodeL is  light source

        nodeL->m_bsdfEval = EdfEval(nodeL->m_hit.material->edf,
                                    &nodeL->m_hit,
                                    &dirLE,
                                    bsdfFlagsL, (double *) 0);
        nodeL->m_bsdfComp.Clear();
        nodeL->m_bsdfComp.Fill(nodeL->m_bsdfEval, BRDF_DIFFUSE_COMPONENT);
    } else {
        nodeL->m_bsdfEval =
                lightConfig->surfaceSampler->DoBsdfEval(nodeL->m_useBsdf,
                                                        &nodeL->m_hit,
                                                        nodeL->m_inBsdf, nodeL->m_outBsdf,
                                                        &nodeL->m_inDirF, &dirLE,
                                                        bsdfFlagsL,
                                                        &nodeL->m_bsdfComp);
    }


    double cosa = -VECTORDOTPRODUCT(dirEL, nodeL->m_normal);
    geom = fabs(cosa * VECTORDOTPRODUCT(nodeE->m_normal, dirEL) / dist2);

    // Geom is always positive !  Visibility checking cannot be done
    // by checking cos signs because materials can be refractive.

    return geom;
}
