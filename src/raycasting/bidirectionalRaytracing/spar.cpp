#include "material/statistics.h"
#include "common/error.h"
#include "skin/Patch.h"
#include "GALERKIN/GalerkinRadiosity.h"
#include "raycasting/bidirectionalRaytracing/spar.h"

#define ZEROEPSILON 1e-12
#define MPMISFUNC(Ci) (Ci)

static void
sparStripLightNode(CBiPath *path, CBiPath *newPath) {
    // Copy first. No deep copy so the new path should not be deleted
    *newPath = *path;

    // Now remove node
    if ( newPath->m_lightSize == 0 ) {
        newPath->m_eyeSize -= 1; // Last node from an eyepath
    } else if ( newPath->m_lightSize == 1 ) {
        newPath->m_lightSize = 0;
        newPath->m_lightPath = newPath->m_lightEndNode = nullptr;
    } else {
        newPath->m_lightSize -= 1;
        newPath->m_lightPath = newPath->m_lightPath->Next();
    }
}

void
CSparList::handlePath(
    CSparConfig *sconfig,
    CBiPath *path,
    COLOR *frad,
    COLOR *fbpt)
{
    CSparListIter iter(*this);
    CSpar **pspar;
    COLOR col;

    colorClear(*fbpt);
    colorClear(*frad);

    while ( (pspar = iter.Next()) ) {
        col = (*pspar)->handlePath(sconfig, path);

        if ( *pspar == sconfig->m_leSpar ) {
            colorAdd(col, *fbpt, *fbpt);
        } else {
            colorAdd(col, *frad, *frad);
        }
    }
}

CSpar::CSpar() {
    m_contrib = new CContribHandler[MAXPATHGROUPS];
    m_sparList = new CSparList[MAXPATHGROUPS];
}

void
CSpar::init(CSparConfig *config) {
    int i;

    for ( i = 0; i < MAXPATHGROUPS; i++ ) {
        m_contrib[i].Init(config->m_bcfg->maximumPathDepth);
        m_sparList[i].RemoveAll();
    }
}

void
CSpar::parseAndInit(int group, char *regExp) {
    int beginPos = 0, endPos = 0;
    char tmpChar;

    while ( regExp[endPos] != '\0' ) {
        if ( regExp[endPos] == ',' ) {
            // Next RegExp

            tmpChar = regExp[endPos];
            regExp[endPos] = '\0';

            m_contrib[group].AddRegExp(regExp + beginPos);

            regExp[endPos] = tmpChar; // Restore
            beginPos = endPos + 1; // Begin next regexp
        }

        endPos++;
    }

    // Still parse last regexp in list
    if ( beginPos != endPos ) {
        m_contrib[group].AddRegExp(regExp + beginPos);
    }
}

CSpar::~CSpar() {
    delete[] m_contrib;
    delete[] m_sparList;
}

COLOR
CSpar::handlePath(CSparConfig *config, CBiPath *path) {
    COLOR result;

    colorClear(result);

    return result;
}

COLOR
CSpar::evalFunction(CContribHandler *contrib, CBiPath *path) {
    COLOR col;
    CBsdfComp oldComp;
    CPathNode *readoutNode;

    // First readout is filled in the path
    if ( path->m_lightSize == 0 ) {
        readoutNode = path->m_eyeEndNode;
    } else {
        readoutNode = path->m_lightPath;
    }

    oldComp = readoutNode->m_bsdfComp;

    getStoredRadiance(readoutNode);

    col = contrib->Compute(path);

    // Restore
    readoutNode->m_bsdfComp = oldComp;

    return col;
}

double
CSpar::evalPdfAndWeight(CSparConfig *config, CBiPath *path) {
    // Standard bi-path pdf evaluation

    double pdf = path->EvalPDFAcc();

    if ( path->m_lightSize == 1 ) {
        // Account for the importance sampling of the light point
        pdf = pdf * path->m_pdfLNE / path->m_lightEndNode->m_pdfFromPrev;
    }

    return 1.0 / pdf; // Weight = 1;
}

double
CSpar::computeWeightTerms(TPathGroupID ID, CSparConfig *config, CBiPath *path, double *sum) {
    *sum = 0.0;
    return 0.0;
}

void
CSpar::getStoredRadiance(CPathNode *node) {
    node->m_bsdfComp.Clear();
}

void
CLeSpar::init(CSparConfig *sconfig) {
    CSpar::init(sconfig);

    // Disjunct path group for BPT
    if ( sconfig->m_bcfg->doLe ) {
        parseAndInit(DISJUNCTGROUP, sconfig->m_bcfg->leRegExp);
    }

    if ( sconfig->m_bcfg->doWeighted ) {
        parseAndInit(LDGROUP, sconfig->m_bcfg->wleRegExp);
        m_sparList[LDGROUP].Add(sconfig->m_ldSpar);
    }
}

COLOR
CLeSpar::handlePath(CSparConfig *sconfig, CBiPath *path) {
    COLOR result;
    COLOR col;
    double totalGeom = path->ComputeTotalGeomFactor();
    double wp;

    colorClear(result);

    if ( !sconfig->m_bcfg->doLe && !sconfig->m_bcfg->doWeighted ) {
        return (result);
    }

    // DISJUNCT GROUP
    if ( sconfig->m_bcfg->doLe ) {
        col = evalFunction(&m_contrib[DISJUNCTGROUP], path);

        if ( colorAverage(col) > ZEROEPSILON ) {
            wp = evalPdfAndWeight(sconfig, path);

            colorAddScaled(result, (float)wp * totalGeom, col, result);
        }
    }

    // OVERLAP GROUP
    if ( sconfig->m_bcfg->doWeighted && sconfig->m_bcfg->doWLe ) {
        if ( path->m_lightSize > 0 ) {
            // No direct hits on a light source
            col = evalFunction(&m_contrib[LDGROUP], path);

            if ( colorAverage(col) > ZEROEPSILON ) {
                wp = evalPdfAndMpWeight(sconfig, path);

                colorAddScaled(result, (float)wp * totalGeom, col, result);
            }
        }
    }

    return result;
}

static double
ComputeCx(
    CSparConfig *sconfig,
    int currentConnect,
    int nodesTotal,
    double pdf,
    double Se,
    double Sl)
{
    double Cx;
    double S;
    int Ni;

    if ( currentConnect == 1 ) {
        Ni = sconfig->m_bcfg->totalSamples;
    } else {
        Ni = sconfig->m_bcfg->samplesPerPixel;
    }
    if ( currentConnect == nodesTotal ) {
        S = Se;
    } else {
        S = Sl;
    }
    if ( S > ZEROEPSILON ) {
        Cx = Ni * pdf / S;
    } else {
        Cx = 0.0; // 1.0?
    }

    return Cx;
}

double
CLeSpar::computeWeightTerms(
    TPathGroupID,
    CSparConfig *sconfig,
    CBiPath *path,
    double *sum)
{
    CPathNode *nextNode;
    CPathNode *L1;
    CPathNode *L2;
    double pdfRR = 0.0;
    double oldPdfL2;
    double oldPdfRR = 0.0;
    double *p_pdfL2;
    double *p_pdfRR;

    double currentPdf, pdfAcc, newPdf;
    double Ci, weight, Sl, Se;
    int currentConnect, nodesTotal;
    double L2L1pdf;

    // name L1, L2 nodes in the path and
    // compute the pdf for generating L2, so that it is
    // independent of L0. L2 can be the eye !!
    if ( path->m_lightSize >= 1 ) {
        L1 = path->m_lightPath;

        if ( path->m_lightSize >= 2 ) {
            L2 = L1->Next(); // L2 is part of lightpath : pdfFromPrev changes
            p_pdfL2 = &L2->m_pdfFromPrev;
            p_pdfRR = nullptr;

            oldPdfL2 = *p_pdfL2;

            bsdfEvalPdf(L1->m_useBsdf, &L1->m_hit,
                        L1->m_inBsdf, L1->m_outBsdf,
                        &L1->m_normal, // DUMMY IN
                        &L2->m_inDirT, // away from L1
                    //		  &L1->m_normal,
                        BRDF_DIFFUSE_COMPONENT,
                        p_pdfL2, &pdfRR);
            if ( L1->m_depth >= sconfig->m_bcfg->minimumPathDepth ) {
                *p_pdfL2 *= pdfRR;
            } // Deep enough for russian roulette
            // Normally depth == 1 and mindepth >= 2, so no rr

            L2L1pdf = L1->m_pdfFromNext; //pdf from eye
            if ( path->m_lightSize - L1->m_depth + path->m_eyeSize ) {
                L2L1pdf *= L1->m_rrPdfFromNext;
            }
        } else {
            L2 = path->m_eyeEndNode;

            p_pdfL2 = &L2->m_pdfFromNext;
            p_pdfRR = &L2->m_rrPdfFromNext;

            oldPdfL2 = *p_pdfL2;
            oldPdfRR = *p_pdfRR;

            if ( L2 == path->m_eyePath ) {
                L2->m_pdfFromNext = 0.0;
                L2->m_rrPdfFromNext = 0.0;
            } else {
                bsdfEvalPdf(L1->m_useBsdf, &L1->m_hit,
                            L1->m_inBsdf, L1->m_outBsdf,
                            &L1->m_normal, // DUMMY IN
                            &path->m_dirLE, // away from L1, N.E.E.
                        //		    &L1->m_normal,
                            BRDF_DIFFUSE_COMPONENT,
                            p_pdfL2, p_pdfRR);
                if ( L1->m_depth >= sconfig->m_bcfg->minimumPathDepth ) {
                    *p_pdfL2 *= pdfRR;
                } // Deep enough for russian roulette
                // Normally depth == 1 and mindepth >= 2, so no rr
            }

            L2L1pdf = L1->m_pdfFromNext; //pdf from eye
            if ( path->m_lightSize - L1->m_depth + path->m_eyeSize ) {
                L2L1pdf *= L1->m_rrPdfFromNext;
            }
        }
    } else {
        L1 = path->m_eyeEndNode;
        L2 = L1->Previous();

        p_pdfL2 = &L2->m_pdfFromNext;
        p_pdfRR = &L2->m_rrPdfFromNext;

        oldPdfL2 = *p_pdfL2;
        oldPdfRR = *p_pdfRR;

        if ( L2 == path->m_eyePath ) {
            L2->m_pdfFromNext = 0.0;
            L2->m_rrPdfFromNext = 0.0;
        } else {
            bsdfEvalPdf(L1->m_useBsdf, &L1->m_hit,
                        L1->m_inBsdf, L1->m_outBsdf,
                        &L1->m_normal, // DUMMY IN
                        &L1->m_inDirF, // away from L1, N.E.E.
                    //		    &L1->m_normal,
                        BRDF_DIFFUSE_COMPONENT,
                        p_pdfL2, p_pdfRR);

        }

        L2L1pdf = L1->m_pdfFromPrev; //pdf from eye
    }


    // Precompute Sl and Se (S value for light path and eyepath/nee to light)
    double sumAl = colorAverage(GLOBAL_statistics_totalEmittedPower);
    double u;
    double v;
    COLOR col;

    // (u, v) coordinates of intersection point
    L1->m_hit.patch->uv(&L1->m_hit.point, &u, &v);

    colorClear(col);

    colorClipPositive(col, col);

    Sl = sumAl * colorAverage(col);

    colorClear(col);

    colorClipPositive(col, col);

    Se = colorAverage(col) * L2L1pdf;

    // pdfAcc for the path

    pdfAcc = path->EvalPDFAcc();

    pdfAcc /= L1->m_pdfFromPrev; // this pdf can be dependent on L0 !

    // now compute the weight of the current path pdf

    currentPdf = pdfAcc;
    currentConnect = path->m_eyeSize;
    nodesTotal = path->m_eyeSize + path->m_lightSize;


    Ci = ComputeCx(sconfig, currentConnect, nodesTotal, currentPdf,
                   Se, Sl);

    weight = MPMISFUNC(Ci);
    *sum = weight;

    // compute other weights (recurrence relation !)

    // To the light
    nextNode = path->m_lightEndNode;

    while ( currentConnect < nodesTotal ) {
        currentConnect++; // Handle next N.E.E.

        if ( currentConnect != nodesTotal ) {
            newPdf = currentPdf * nextNode->m_pdfFromNext / nextNode->m_pdfFromPrev;
            if ( currentConnect >=
                 sconfig->m_bcfg->minimumPathDepth ) {
                // At this eye depth, RR is applied
                newPdf *= nextNode->m_rrPdfFromNext;
            }
        } else {
            newPdf = currentPdf;
        } // Change is in Se <-> Sl

        currentPdf = newPdf;

        Ci = ComputeCx(sconfig, currentConnect, nodesTotal, currentPdf,
                       Se, Sl);
        *sum += MPMISFUNC(Ci);

        nextNode = nextNode->Previous();
    }

    // To the eye
    nextNode = path->m_eyeEndNode;
    currentConnect = path->m_eyeSize;
    currentPdf = pdfAcc; // Start from actual path pdf

    while ( currentConnect > 1 )  // N.E.E. to eye (=1) included, direct hit
        // on eye not !
    {
        currentConnect--;

        if ( currentConnect != nodesTotal - 1 ) {
            newPdf = currentPdf * nextNode->m_pdfFromNext / nextNode->m_pdfFromPrev;

            if ( nodesTotal - currentConnect >= sconfig->m_bcfg->minimumPathDepth ) {
                // At this light depth, RR is applied
                newPdf *= nextNode->m_rrPdfFromNext;
            }
        } else {
            newPdf = currentPdf;
        }

        currentPdf = newPdf;

        Ci = ComputeCx(sconfig, currentConnect, nodesTotal, currentPdf,
                       Se, Sl);

        *sum += MPMISFUNC(Ci);

        nextNode = nextNode->Previous();
    }

    PNAN(weight);
    PNAN(*sum);

    // Restore path
    *p_pdfL2 = oldPdfL2;
    if ( p_pdfRR ) {
        *p_pdfRR = oldPdfRR;
    }

    return weight;
}

double
CLeSpar::evalPdfAndMpWeight(CSparConfig *config, CBiPath *path) {
    CBiPath ldPath;
    double totalPdf;
    double sum = 0.0, partialSum, thisCi;

    // bi path pdf (weight = 1.0 in CSpar)
    totalPdf = 1.0 / CSpar::evalPdfAndWeight(config, path);

    sparStripLightNode(path, &ldPath);

    // Ci's for ldPath

    thisCi = computeWeightTerms(LDGROUP, config, &ldPath, &partialSum);
    sum += partialSum;

    // Sum Ci's from other Spars
    CSparListIter iter(m_sparList[LDGROUP]);
    CSpar **pspar;

    while ((pspar = iter.Next())) {
        (*pspar)->computeWeightTerms(LDGROUP, config, &ldPath, &partialSum);

        sum += partialSum;
    }

    // Combine weight and pdf
    return (thisCi / (sum * totalPdf));
}

// Standard BPT pdf and weights
double
CLeSpar::evalPdfAndWeight(CSparConfig *sconfig, CBiPath *path) {
    return path->EvalPDFAndWeight(sconfig->m_bcfg);
}


void CLeSpar::getStoredRadiance(CPathNode *node) {
    // If node is on an edf, the correct value is filled in,
    // otherwise make any current value zero.
    EDF *endingEdf = node->m_hit.material->edf;

    if ( endingEdf == nullptr ) {
        node->m_bsdfComp.Clear();
    }
}

void
CLDSpar::init(CSparConfig *sconfig) {
    CSpar::init(sconfig);

    if ( !(sconfig->m_bcfg->doLD || sconfig->m_bcfg->doWeighted)) {
        return;
    }

    if ( !GLOBAL_radiance_currentRadianceMethodHandle ) {
        logError("CLDSpar::mainInit", "Galerkin Radiance method not active !");
    }

    // Overlap group
    if ( sconfig->m_bcfg->doLD ) {
        parseAndInit(DISJUNCTGROUP, sconfig->m_bcfg->ldRegExp);
    }

    if ( sconfig->m_bcfg->doWeighted ) {
        parseAndInit(LDGROUP, sconfig->m_bcfg->wldRegExp);
        m_sparList[LDGROUP].Add(sconfig->m_leSpar);
    }
}

COLOR
CLDSpar::handlePath(CSparConfig *sconfig, CBiPath *path) {
    COLOR result;
    COLOR col;

    colorClear(result);

    // Only path tracing paths !!
    if ( path->m_lightSize == 0 ) {
        double wp;
        double totalGeom = path->ComputeTotalGeomFactor();

        // DISJUNCT GROUP
        if ( sconfig->m_bcfg->doLD ) {
            col = evalFunction(&m_contrib[DISJUNCTGROUP], path);

            if ( colorAverage(col) > ZEROEPSILON ) {
                wp = evalPdfAndWeight(sconfig, path);
                colorAddScaled(result, wp * totalGeom, col, result);
            }
        }

        // OVERLAP GROUP
        if ( sconfig->m_bcfg->doWeighted && sconfig->m_bcfg->doWLD ) {
            col = evalFunction(&m_contrib[LDGROUP], path);

            if ( colorAverage(col) > ZEROEPSILON ) {
                wp = EvalPDFAndMPWeight(sconfig, path);

                colorAddScaled(result, wp * totalGeom, col, result);
            }
        }
    }

    return result;
}

double
CLDSpar::computeWeightTerms(
    TPathGroupID,
    CSparConfig *sconfig,
    CBiPath *path,
    double *sum)
{
    double Sld;
    double weight;
    CPathNode *node, *L1;
    double pdfAcc = 1.0;
    int i;

    if ( path->m_lightSize >= 1 ) {
        L1 = path->m_lightPath;
    } else {
        L1 = path->m_eyeEndNode;
    }

    node = path->m_eyePath;

    for ( i = 0; i < path->m_eyeSize; i++ ) {
        pdfAcc *= node->m_pdfFromPrev;
        node = node->Next();
    }


    node = path->m_lightPath;

    for ( i = 0; i < path->m_lightSize; i++ ) {
        pdfAcc *= node->m_pdfFromNext;

        if ( path->m_eyeSize + path->m_lightSize - i >= sconfig->m_bcfg->minimumPathDepth ) {
            pdfAcc *= node->m_rrPdfFromNext;
        }

        node = node->Next();
    }

    double u;
    double v;
    COLOR col;

    // (u, v) coordinates of intersection point
    L1->m_hit.patch->uv(&L1->m_hit.point, &u, &v);

    colorClear(col);

    colorClipPositive(col, col);

    Sld = colorAverage(col);
    Sld = Sld * Sld;

    //  COLOR accuracy;
    weight = sconfig->m_bcfg->samplesPerPixel * pdfAcc / Sld;
    weight = MPMISFUNC(weight);

    *sum = weight;

    return weight;
}


double
CLDSpar::EvalPDFAndMPWeight(CSparConfig *sconfig, CBiPath *path) {
    double totalPdf;
    double sum = 0.0;
    double partialSum;
    double thisCi;

    // bi path pdf (weight = 1.0 in CSpar)
    totalPdf = 1.0 / CSpar::evalPdfAndWeight(sconfig, path);

    // Ci's for path
    thisCi = computeWeightTerms(LDGROUP, sconfig, path, &partialSum);
    sum += partialSum;

    // sum Ci's from other Spars
    CSparListIter iter(m_sparList[LDGROUP]);
    CSpar **pspar;

    while ((pspar = iter.Next())) {
        (*pspar)->computeWeightTerms(LDGROUP, sconfig, path, &partialSum);

        sum += partialSum;
    }

    // Combine weight and pdf
    return (thisCi / (sum * totalPdf));
}

// standard bpt weights, no other spars
double
CLDSpar::evalPdfAndWeight(CSparConfig *sconfig, CBiPath *path) {
    return CSpar::evalPdfAndWeight(sconfig, path);
}

void
CLDSpar::getStoredRadiance(CPathNode *node) {
    double u;
    double v;
    COLOR col;

    // u, v coordinates of intersection point
    node->m_hit.patch->uv(&node->m_hit.point, &u, &v);

    col = GLOBAL_radiance_currentRadianceMethodHandle->GetRadiance(node->m_hit.patch, u, v,
                                                                   node->m_inDirF);

    colorClipPositive(col, col);

    node->m_bsdfComp.Clear();
    node->m_bsdfComp.Fill(col, BRDF_DIFFUSE_COMPONENT);

    return;
}
