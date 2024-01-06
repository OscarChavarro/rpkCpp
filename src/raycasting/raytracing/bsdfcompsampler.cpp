#include "material/bsdf.h"
#include "shared/discretesampling.h"
#include "raycasting/raytracing/bsdfcompsampler.h"

bool CBsdfCompSampler::Sample(CPathNode *prevNode, CPathNode *thisNode,
                              CPathNode *newNode, double x_1, double x_2,
                              bool doRR, BSDFFLAGS flags) {
    // Get the reflectances/transmittances for the different
    // component groups

    COLOR col;
    float scatteredPower[MAX_COMP_GROUPS + 1];
    float totalPower = 0.;
    BSDF *bsdf = thisNode->m_useBsdf;
    int i, choice;

    for ( i = 0; i < m_compArraySize; i++ ) {
        if ((m_compArray[i] & flags) != NO_COMPONENTS ) {
            col = BsdfScatteredPower(bsdf, &thisNode->m_hit, &thisNode->m_normal,
                                     m_compArray[i] & flags);
            scatteredPower[i] = colorAverage(col);
            totalPower += scatteredPower[i];
        } else {
            scatteredPower[i] = 0;
        }
    }

    // Choose one

    if ( totalPower < EPSILON ) {
        return false;
    }

    // Account for russian roulette

    if ( doRR ) {
        scatteredPower[m_compArraySize] = 1.0 - totalPower;
        totalPower = 1.0;
    }

    double choicePdf;
    choice = SampleDiscrete(scatteredPower, totalPower, &x_1, &choicePdf);

    if ( choice < m_compArraySize ) {
        // Now sample a real node, using the standard bsdf sampler

        bool ok = CBsdfSampler::Sample(prevNode, thisNode, newNode, x_1, x_2,
                                       false, flags & m_compArray[choice]);

        if ( ok ) {
            // Modify generating pdf

            newNode->m_pdfFromPrev *= choicePdf; // scatteredPower[choice] / totalPower;
        }

        return ok;
    } else {
        return false;  // Absorbed
    }
}
