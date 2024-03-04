/*
 * bipath.C: CBiPath method implementation
 */

#include "common/error.h"
#include "raycasting/raytracing/bipath.h"

// Constructor
CBiPath::CBiPath() {
    Init();
}

void CBiPath::Init() {
    // Empty path, no allocated nodes.
    m_lightPath = m_lightEndNode = nullptr;
    m_eyePath = m_eyeEndNode = nullptr;
    m_eyeSize = m_lightSize = 0;
}

// Evaluate the radiance contribution of a bipath. Only Function
// evaluation, no pdf involved
COLOR CBiPath::EvalRadiance() {
    COLOR col;
    double factor = 1.0;
    SimpleRaytracingPathNode *node;
    int i;

    colorSetMonochrome(col, 1.0);

    node = m_eyePath;

    for ( i = 0; i < m_eyeSize; i++ ) {
        colorProduct(node->m_bsdfEval, col, col);
        factor *= node->m_G;
        node = node->next();
    }

    node = m_lightPath;

    for ( i = 0; i < m_lightSize; i++ ) {
        colorProduct(node->m_bsdfEval, col, col);
        factor *= node->m_G;
        node = node->next();
    }

    factor *= m_geomConnect; // Next event ray geometry factor

    colorScale((float)factor, col, col);

    return col;
}

// Evaluate accumulated PDF of a bipath (no weighting)
double CBiPath::EvalPDFAcc() {
    SimpleRaytracingPathNode *node;
    double pdfAcc = 1.0;
    int i;

    node = m_eyePath;

    for ( i = 0; i < m_eyeSize; i++ ) {
        pdfAcc *= node->m_pdfFromPrev;
        node = node->next();
    }

    node = m_lightPath;

    for ( i = 0; i < m_lightSize; i++ ) {
        pdfAcc *= node->m_pdfFromPrev;
        node = node->next();
    }

    return pdfAcc;
}

// Evaluate weight/pdf for a bipath, taking into account other pdf's
// depending on the config (bcfg).
float CBiPath::EvalPDFAndWeight(BP_BASECONFIG *bcfg, float *pPdf,
                                 float *pWeight) {
    int currentConnect;
    double pdfAcc;
    double pdfSum;
    double currentPdf;
    double newPdf;
    double c;
    SimpleRaytracingPathNode *nextNode;
    double realPdf, tmpPdf, weight;

    // First we compute the pdf for generating this path

    pdfAcc = EvalPDFAcc();

    // now we compute the weight using the recurrence relation (see Veach PhD)

    currentConnect = m_eyeSize; // Position of real next event estimator
    //    from this estimator position we go back to the eye and forth
    //    to the light and compute the pdf for those n.e.e. positions
    // currentConnect = depth of eyenode where connection starts

    currentPdf = pdfAcc; // Basis for subsequent pdf computations

    if ( m_eyeSize == 1 ) {
        c = (double)bcfg->totalSamples; // N.E. to the eye
    } else {
        c = bcfg->samplesPerPixel;
    }

    if ( m_lightSize == 1 ) {
        // Account for the importance sampling of the light point
        realPdf = pdfAcc * m_pdfLNE / m_lightEndNode->m_pdfFromPrev;
    } else {
        realPdf = pdfAcc;
    }

    weight = MULTIPLE_IMPORTANCE_SAMPLING_FUNC(c * realPdf); // Still needs to be divided by the sum
    pdfSum = weight;

    // To the light

    nextNode = m_lightEndNode;

    while ( currentConnect < m_eyeSize + m_lightSize )
        // largest eyenode depth <= light source node (depth seen from eye)
    {
        currentConnect++; // Handle next N.E.E.

        newPdf = currentPdf * nextNode->m_pdfFromNext / nextNode->m_pdfFromPrev;

        if ( currentConnect - 1 >=
             bcfg->minimumPathDepth ) {
            // At this light depth, RR is applied
            newPdf *= nextNode->m_rrPdfFromNext;
        }

        if ( currentConnect == 1 ) {
            c = (double)bcfg->totalSamples; // N.E. to the eye
        } else {
            c = bcfg->samplesPerPixel;
        }

        if ( currentConnect == m_eyeSize + m_lightSize - 1 ) {
            // Account for light importance sampling pdf
            tmpPdf = newPdf * m_pdfLNE / nextNode->previous()->m_pdfFromPrev;
        } else {
            tmpPdf = newPdf;
        }

        pdfSum += MULTIPLE_IMPORTANCE_SAMPLING_FUNC(c * tmpPdf);

        currentPdf = newPdf;
        nextNode = nextNode->previous();
    }

    // To the eye
    nextNode = m_eyeEndNode;
    currentConnect = m_eyeSize;
    currentPdf = pdfAcc; // Start from actual path pdf

    while ( currentConnect > 1 )  // N.E.E. to eye (=1) included, direct hit
        // on eye not,  light node depth = eye size + light size - 1 - currentConnect
    {
        currentConnect--; // Handle next N.E.E.

        newPdf = currentPdf * nextNode->m_pdfFromNext / nextNode->m_pdfFromPrev;

        if ( m_eyeSize + m_lightSize - 2 - currentConnect >=
             bcfg->minimumPathDepth ) {
            // At this light depth, RR is applied
            newPdf *= nextNode->m_rrPdfFromNext;
        }

        if ( currentConnect == 1 ) {
            c = (double)bcfg->totalSamples; // N.E. to the eye
        } else {
            c = bcfg->samplesPerPixel;
        }

        if ( currentConnect == m_eyeSize + m_lightSize - 1 ) {
            // Account for light importance sampling pdf
            // This code is only reached for X_0 paths !
            tmpPdf = newPdf * m_pdfLNE / nextNode->m_pdfFromNext;
        } else {
            tmpPdf = newPdf;
        }


        pdfSum += MULTIPLE_IMPORTANCE_SAMPLING_FUNC(c * tmpPdf);

        currentPdf = newPdf;
        nextNode = nextNode->previous();
    } /* while   (to the eye) */

    weight = weight / pdfSum;

    // printf("W %f P %f\n", weight, realPdf);

    PNAN(weight);

    if ( pWeight ) {
        *pWeight = (float)weight;
    }

    if ( pPdf ) {
        *pPdf = (float)realPdf;
    }

    return (float)(weight / realPdf);
}
