#include <cstring>

#include "common/error.h"
#include "shared/stratification.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/eyesampler.h"
#include "raycasting/raytracing/lightsampler.h"
#include "raycasting/raytracing/lightdirsampler.h"
#include "raycasting/raytracing/bsdfsampler.h"
#include "raycasting/raytracing/samplertools.h"
#include "raycasting/raytracing/screeniterate.h"
#include "raycasting/bidirectionalRaytracing/spar.h"
#include "raycasting/raytracing/densitybuffer.h"
#include "raycasting/raytracing/densitykernel.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracer.h"

// Persistent BidirPath state, contains actual GUI state and some other stuff
BIDIRPATH_STATE GLOBAL_rayTracing_biDirectionalPath;

/**
Bidirectional path tracing configuration structure.
non persistently used each time an image is rendered
*/
class BPCONFIG {
  public:
    BP_BASECONFIG *bcfg;

    // Configuration for tracing the paths
    CSamplerConfig eyeConfig;
    CSamplerConfig lightConfig;

    // Internal vars
    ScreenBuffer *screen;
    double fluxToRadFactor;
    int nx;
    int ny;
    double pdfLNE; // pdf for sampling light point separately

    CDensityBuffer *dBuffer;
    CDensityBuffer *dBuffer2;
    float xSample;
    float ySample;
    CPathNode *eyePath;
    CPathNode *lightPath;

    //  int maxCombinedLength;

    // SPaR configuration

    CSparConfig sparConfig;
    CSparList *sparList;

    bool deStoreHits;

    ScreenBuffer *ref;
    ScreenBuffer *dest;
    ScreenBuffer *ref2;
    ScreenBuffer *dest2;
    CKernel2D kernel;
    int scaleSamples;
};

#define STRINGS_SIZE 300

static bool SpikeCheck(COLOR col) {
    //  return false;

    double colAvg = colorAverage(col);

    if ( ISNAN(colAvg)) {
        printf("COL ");
        col.print(stdout);
        printf("\n");
        return true;
    }

    if ( colAvg > 60000 /* Aaaaaaarrrrggggghh */) {
        printf("Spike\n");
        return true;
    }

    if ( colAvg < 0 ) {
        printf("Negative");
        return true;
    }

    return false;
}

static void AddWithSpikeCheck(BPCONFIG *config, CBiPath *path, int nx, int ny,
                              float pix_x, float pix_y, COLOR f,
                              double pdf, double weight, bool radSample = false) {
    if ( config->bcfg->doDensityEstimation ) {
        ScreenBuffer *rs, *ds;
        CDensityBuffer *db;
        float baseSize;

        if ( radSample ) {
            rs = config->ref2;
            ds = config->dest2;
            db = config->dBuffer2;
            baseSize = 1.5;
        } else {
            rs = config->ref;
            ds = config->dest;
            db = config->dBuffer;
            baseSize = 5;
        }

        if ( config->deStoreHits && db ) {
            db->Add(pix_x, pix_y, f, pdf, weight);
        } else {
            // Now splat directly into the dest screen

            Vector2D center;
            COLOR g;

            // Get the center:

            center.u = pix_x;
            center.v = pix_y;

            // Convert f into DE quantities

            float factor = rs->getPixXSize() * rs->getPixYSize()
                           * (float) config->bcfg->totalSamples;

            if ( colorAverage(f) > EPSILON ) {
                colorScale(factor, f, g); // Undo part of flux to rad factor

                config->kernel.VarCover(center, g, rs, ds,
                                        config->bcfg->totalSamples, config->scaleSamples,
                                        baseSize);
            }
            return;
        }
    }

    if ( config->bcfg->eliminateSpikes ) {
        if ( !SpikeCheck(f)) {
            /*
            if(!radSample)
            {
          printf("L %i E %i F %g\n", path->m_lightSize, path->m_eyeSize, COLORAVERAGE(f));
            }
            */

            config->screen->add(nx, ny, f);
        } else {
            // Wanna see the spikes !
            //  config->screen->Add(config->nx, config->ny, f);
        }
    } else {
        config->screen->add(nx, ny, f);
    }
}

void HandlePath_X_0(BPCONFIG *config, CBiPath *path) {
    EDF *endingEdf = path->m_eyeEndNode->m_hit.material ? path->m_eyeEndNode->m_hit.material->edf : nullptr;
    COLOR oldBsdfEval, f, frad;
    CBsdfComp oldBsdfComp;
    double factor, pdfLNE, oldPdfLNE;
    double oldPDFLightEval, oldPDFDirEval;
    double oldRRPDFLightEval, oldRRPDFDirEval;
    double pdf = 1.0, weight = 1.0;
    CPathNode *eyePrevNode, *eyeEndNode;

    if ( path->m_eyeSize > config->bcfg->maximumPathDepth ) {
        return;
    }

    if ( endingEdf != nullptr || config->bcfg->useSpars ) {
        eyeEndNode = path->m_eyeEndNode;

        // Store the Bsdf and PDF evaluations that will be overwritten
        // by other values needed to compute the contribution

        eyePrevNode = eyeEndNode->Previous(); // always != nullptr

        oldBsdfEval = eyeEndNode->m_bsdfEval;
        oldBsdfComp = eyeEndNode->m_bsdfComp;
        oldPDFLightEval = eyeEndNode->m_pdfFromNext;
        PNAN(eyeEndNode->m_pdfFromNext);

        oldPDFDirEval = eyePrevNode->m_pdfFromNext;
        PNAN(eyePrevNode->m_pdfFromNext);

        oldRRPDFLightEval = eyeEndNode->m_rrPdfFromNext;
        oldRRPDFDirEval = eyePrevNode->m_rrPdfFromNext;

        oldPdfLNE = path->m_pdfLNE;

        // Fill in new values

        eyeEndNode->m_bsdfComp.Clear();

        if ( endingEdf != nullptr ) {
            // Landed on a light : fill in values for BPT
            eyeEndNode->m_bsdfEval = edfEval(endingEdf,
                                             &eyeEndNode->m_hit,
                                             &eyeEndNode->m_inDirF,
                                             ALL_COMPONENTS,
                                             nullptr);
            eyeEndNode->m_bsdfComp.Fill(eyeEndNode->m_bsdfEval,
                                        BRDF_DIFFUSE_COMPONENT);

            if ( config->lightConfig.maxDepth > 0 ) {
                eyeEndNode->m_pdfFromNext =
                        config->lightConfig.pointSampler->EvalPDF(nullptr, eyeEndNode);
                PNAN(eyeEndNode->m_pdfFromNext);

                eyeEndNode->m_rrPdfFromNext = 1.0; // Light point: no Russian R.
            } else {
                // Impossible to generate light point with a light sub path
                eyeEndNode->m_pdfFromNext = 0.0;
                eyeEndNode->m_rrPdfFromNext = 0.0;
            }

            if ( config->lightConfig.maxDepth > 1 ) {
                eyePrevNode->m_pdfFromNext =
                        config->lightConfig.dirSampler->EvalPDF(eyeEndNode, eyePrevNode);
                PNAN(eyePrevNode->m_pdfFromNext);

                eyePrevNode->m_rrPdfFromNext = 1.0;
            } else {
                eyePrevNode->m_pdfFromNext = 0.0;
                eyePrevNode->m_rrPdfFromNext = 0.0;
            }

            // Compute
            if ((config->bcfg->sampleImportantLights) && (config->lightConfig.maxDepth > 0)
                && (path->m_eyeSize > 2)) {
                pdfLNE = config->eyeConfig.neSampler->EvalPDF(eyePrevNode, eyeEndNode);
            } else {
                pdfLNE = eyeEndNode->m_pdfFromNext; // same sampling as lightpath
            }

            PNAN(pdfLNE);
            path->m_pdfLNE = pdfLNE;
        }

        // Path radiance evaluation

        path->m_geomConnect = 1.0; // Fake connection for X_0 paths

        if ( config->bcfg->useSpars ) {
            // f = config->sparList->photonMapHandlePath(&config->sparConfig, path);
            config->sparList->handlePath(&config->sparConfig, path, &frad, &f);
            factor = 1.0; // pdf and weight already taken into account
        } else {
            // endingEdf != nullptr !!
            f = path->EvalRadiance();

            //      if(COLORAVERAGE(f) > EPSILON)
            //{
            factor = path->EvalPDFAndWeight(config->bcfg, &pdf, &weight);
            //}
            //else
            //{
            //factor = 0.0;
            //}
        }

        factor *= config->fluxToRadFactor / config->bcfg->samplesPerPixel;

        if ( config->bcfg->useSpars ) {
            colorScale(factor, frad, frad);
            AddWithSpikeCheck(config, path, config->nx, config->ny,
                              config->xSample, config->ySample, frad, pdf, weight, true);
            colorScale(factor, f, f);
            AddWithSpikeCheck(config, path, config->nx, config->ny,
                              config->xSample, config->ySample, f, pdf, weight, false);

        } else {
            colorScale(factor, f, f);
            AddWithSpikeCheck(config, path, config->nx, config->ny,
                              config->xSample, config->ySample, f, pdf, weight);
        }

        // Restore the Brdf and PDF evaluations

        path->m_pdfLNE = oldPdfLNE;
        eyeEndNode->m_bsdfEval = oldBsdfEval;
        eyeEndNode->m_bsdfComp = oldBsdfComp;
        eyeEndNode->m_pdfFromNext = oldPDFLightEval;
        eyePrevNode->m_pdfFromNext = oldPDFDirEval;
        eyeEndNode->m_rrPdfFromNext = oldRRPDFLightEval;
        eyePrevNode->m_rrPdfFromNext = oldRRPDFDirEval;
    }
}

/*************************************************************************/
COLOR ComputeNEFluxEstimate(BPCONFIG *config, CBiPath *path,
                            double *pPdf = nullptr, double *pWeight = nullptr, COLOR *frad = nullptr) {
    CPathNode *eyePrevNode, *lightPrevNode;
    COLOR oldBsdfL, oldBsdfE;
    CBsdfComp oldBsdfCompL, oldBsdfCompE;
    double oldPdfL, oldPdfE, oldRRPdfL, oldRRPdfE;
    double oldPdfLP = 0., oldPdfEP = 0., oldRRPdfLP = 0., oldRRPdfEP = 0.;
    COLOR f;
    CPathNode *eyeEndNode, *lightEndNode;

    // Store PDF and BSDF evals that will be overwritten

    eyeEndNode = path->m_eyeEndNode;
    lightEndNode = path->m_lightEndNode;
    eyePrevNode = eyeEndNode->Previous();
    lightPrevNode = lightEndNode->Previous();

    oldBsdfL = lightEndNode->m_bsdfEval;
    oldBsdfCompL = lightEndNode->m_bsdfComp;

    oldBsdfE = eyeEndNode->m_bsdfEval;
    oldBsdfCompE = eyeEndNode->m_bsdfComp;

    oldPdfL = lightEndNode->m_pdfFromNext;
    PNAN(lightEndNode->m_pdfFromNext);

    oldRRPdfL = lightEndNode->m_rrPdfFromNext;

    if ( lightPrevNode ) {
        oldPdfLP = lightPrevNode->m_pdfFromNext;
        PNAN(lightPrevNode->m_pdfFromNext);
        oldRRPdfLP = lightPrevNode->m_rrPdfFromNext;
    }

    oldPdfE = eyeEndNode->m_pdfFromNext;
    PNAN(eyeEndNode->m_pdfFromNext);
    oldRRPdfE = eyeEndNode->m_rrPdfFromNext;

    if ( eyePrevNode ) {
        oldPdfEP = eyePrevNode->m_pdfFromNext;
        PNAN(eyePrevNode->m_pdfFromNext);
        oldRRPdfEP = eyePrevNode->m_rrPdfFromNext;
    }

    // Connect the subpaths

    path->m_geomConnect =
            pathNodeConnect(eyeEndNode, lightEndNode,
                            &config->eyeConfig, &config->lightConfig,
                            CONNECT_EL | CONNECT_LE | FILL_OTHER_PDF,
                            BSDF_ALL_COMPONENTS, BSDF_ALL_COMPONENTS, &path->m_dirEL);

    VECTORSCALE(-1, path->m_dirEL, path->m_dirLE);

    // Evaluate radiance and pdf and weight

    if ( config->bcfg->useSpars ) {
        // f = config->sparList->photonMapHandlePath(&config->sparConfig, path);
        config->sparList->handlePath(&config->sparConfig, path, frad, &f);
    } else {
        f = path->EvalRadiance();


        //if(COLORAVERAGE(f) > EPSILON)
        //{
        double factor = path->EvalPDFAndWeight(config->bcfg, pPdf, pWeight);
        colorScale(factor, f, f); // Flux estimate
        //}
        //else
        //{
        //  colorClear(f);
        //}
    }

    // Restore old values

    lightEndNode->m_bsdfEval = oldBsdfL;
    lightEndNode->m_bsdfComp = oldBsdfCompL;

    eyeEndNode->m_bsdfEval = oldBsdfE;
    eyeEndNode->m_bsdfComp = oldBsdfCompE;

    lightEndNode->m_pdfFromNext = oldPdfL;
    lightEndNode->m_rrPdfFromNext = oldRRPdfL;

    if ( lightPrevNode ) {
        lightPrevNode->m_pdfFromNext = oldPdfLP;
        lightPrevNode->m_rrPdfFromNext = oldRRPdfLP;
    }

    eyeEndNode->m_pdfFromNext = oldPdfE;
    eyeEndNode->m_rrPdfFromNext = oldRRPdfE;

    if ( eyePrevNode ) {
        eyePrevNode->m_pdfFromNext = oldPdfEP;
        eyePrevNode->m_rrPdfFromNext = oldRRPdfEP;
    }

    return f;
}

/*************************************************************************/
/*
 * HandlePath_X_X : handle a path with eyeSize >= 2 and
 * lightsize >= 1
 */
void HandlePath_X_X(BPCONFIG *config, CBiPath *path) {
    COLOR f, frad;
    double oldPdfLNE = 0.0, pdf, weight;
    bool doLNE;
    CPathNode newLightNode;
    CPathNode *oldLightPath = nullptr, *oldLightEndNode = nullptr;
    int oldLightSize = 0;

    if ((path->m_eyeSize + path->m_lightSize) > config->bcfg->maximumPathDepth ) {
        return;
    }

    // Check if we need to sample another lightpoint with
    // importance sampling.

    doLNE = (path->m_lightSize == 1) && config->bcfg->sampleImportantLights;

    if ( doLNE ) {
        // Replace the current lightpath with new importance sampled path

        oldLightPath = path->m_lightPath;
        oldLightSize = path->m_lightSize;
        oldLightEndNode = path->m_lightEndNode;

        path->m_lightPath = &newLightNode;

        newLightNode.m_pdfFromPrev = 0.0;
        newLightNode.m_pdfFromNext = 0.0;

        if ( !config->eyeConfig.neSampler->Sample(nullptr, path->m_eyeEndNode,
                                                  &newLightNode,
                                                  drand48(), drand48())) {
            // No light point sampled, no contribution possible

            path->m_lightPath = oldLightPath;
            return;
        }

        // Compute the normal sampling pdf for the new path/point

        oldPdfLNE = path->m_pdfLNE;
        path->m_pdfLNE = newLightNode.m_pdfFromPrev;
        newLightNode.m_pdfFromPrev =
                config->lightConfig.pointSampler->EvalPDF(nullptr, &newLightNode);

        PNAN(newLightNode.m_pdfFromPrev);
        PNAN(path->m_pdfLNE);

        path->m_lightEndNode = &newLightNode;
    }

    if ( pathNodesVisible(path->m_eyeEndNode, path->m_lightEndNode)) {
        f = ComputeNEFluxEstimate(config, path, &pdf, &weight, &frad);

        float factor = config->fluxToRadFactor / config->bcfg->samplesPerPixel;
        colorScale(factor, f, f);
        AddWithSpikeCheck(config, path, config->nx, config->ny,
                          config->xSample, config->ySample, f, pdf, weight);

        if ( config->bcfg->useSpars ) {
            colorScale(factor, frad, frad);
            AddWithSpikeCheck(config, path, config->nx, config->ny,
                              config->xSample, config->ySample, frad, pdf, weight, true);
        }
    }

    if ( doLNE ) {
        // Restore the current lightpath

        path->m_lightPath = oldLightPath;
        path->m_lightSize = oldLightSize;
        path->m_lightEndNode = oldLightEndNode;
        path->m_pdfLNE = oldPdfLNE;
    }
}

void HandlePath_1_X(BPCONFIG *config, CBiPath *path) {
    int nx, ny;
    float pix_x, pix_y;
    COLOR f, frad;
    double oldPdfLNE, pdf, weight;

    if ((path->m_eyeSize + path->m_lightSize) > config->bcfg->maximumPathDepth ) {
        return;
    }


    // We don't do light importance sampling for particle tracing paths

    oldPdfLNE = path->m_pdfLNE;
    path->m_pdfLNE = config->lightPath->m_pdfFromPrev;

    // First we need to determine if the lightEndNode can be seen from
    // the camera. At the same time the pixel hit is computed

    if ( eyeNodeVisible(path->m_eyeEndNode, path->m_lightEndNode,
                        &pix_x, &pix_y)) {
        // Visible !

        f = ComputeNEFluxEstimate(config, path, &pdf, &weight, &frad);

        config->screen->getPixel(pix_x, pix_y, &nx, &ny);

        float factor;
        if ( config->bcfg->doDensityEstimation ) {
            factor = (computeFluxToRadFactor(nx, ny)
                      / (float) config->bcfg->totalSamples);
        } else {
            factor = (computeFluxToRadFactor(nx, ny)
                      / (float) config->bcfg->totalSamples);
        }
        colorScale(factor, f, f);

        AddWithSpikeCheck(config, path, nx, ny, pix_x, pix_y, f, pdf, weight);

        if ( config->bcfg->useSpars ) {
            colorScale(factor, frad, frad);
            AddWithSpikeCheck(config, path, nx, ny, pix_x, pix_y, frad, pdf, weight, true);
        }

    }

    // restore
    path->m_pdfLNE = oldPdfLNE;
}

void BPCombinePaths(BPCONFIG *config) {
    int eyeSize, lightSize;
    bool eyeSubPathDone, lightSubPathDone;
    CPathNode *eyeEndNode, *lightEndNode;
    CPathNode *eyePath, *lightPath;
    CBiPath path;

    eyePath = config->eyePath;
    lightPath = config->lightPath;


    // Compute pdfLNE for the second point in the light path

    if ( config->lightPath ) {
        // Single light point get importance sampled, so compute
        // the pdf for it, in order to get correct weights.
        if ( config->bcfg->sampleImportantLights && config->lightPath->Next()) {
            config->pdfLNE =
                    config->eyeConfig.neSampler->EvalPDF(lightPath->Next(),
                                                         lightPath);

        } else {
            config->pdfLNE = lightPath->m_pdfFromPrev;
        }
    } else {
        config->pdfLNE = 0.0;
    }

    path.m_eyePath = eyePath;
    path.m_lightPath = lightPath;
    path.m_pdfLNE = config->pdfLNE;

    // Loop over all possible paths that can be constructed from
    // the eye and light path

    eyeSubPathDone = false;
    eyeSize = 1;  // Always include the eyepoint !
    eyeEndNode = eyePath;

    while ( !eyeSubPathDone ) {
        // Handle direct hit on light (lightSize = 0)

        if ( eyeSize > 1 ) {
            // printf("Handle : E %i L %i\n", eyeSize, 0);

            path.m_eyeSize = eyeSize;
            path.m_eyeEndNode = eyeEndNode;
            path.m_lightSize = 0;
            path.m_lightEndNode = nullptr;

            HandlePath_X_0(config, &path);
        }

        // Handle lightSize > 0 (with N.E.E.)

        lightSubPathDone = lightPath == nullptr;
        lightSize = 1; // lightSize == 0 handled in X_0
        lightEndNode = lightPath;

        while ( !lightSubPathDone ) {
            // Compute N.E.

            if ( eyeSize > 1 ) {
                // printf("Handle : E %i L %i\n", eyeSize, lightSize);

                path.m_eyeSize = eyeSize;
                path.m_eyeEndNode = eyeEndNode;
                path.m_lightSize = lightSize;
                path.m_lightEndNode = lightEndNode;
                HandlePath_X_X(config, &path);
            } else {
                // printf("Handle : E %i L %i\n", eyeSize, lightSize);

                path.m_eyeSize = eyeSize;
                path.m_eyeEndNode = eyeEndNode;
                path.m_lightSize = lightSize;
                path.m_lightEndNode = lightEndNode;

                HandlePath_1_X(config, &path);
            }

            if ( lightEndNode->Ends()) {
                lightSubPathDone = true;
            } else {
                lightSize++;
                lightEndNode = lightEndNode->Next();
            }
        }

        if ( eyeEndNode->Ends()) {
            eyeSubPathDone = true;
        } else {
            eyeSize++;
            eyeEndNode = eyeEndNode->Next();
        }
    }
}

static COLOR
BPCalcPixel(int nx, int ny, BPCONFIG *config) {
    int i;
    double x_1, x_2;
    COLOR result;
    CStrat2D strat(config->bcfg->samplesPerPixel);
    CPathNode *pixNode, *nextNode;

    colorClear(result);

    // We sample the eye here since it's always the same point

    if ( config->eyePath == nullptr ) {
        config->eyePath = new CPathNode;
    }

    config->eyeConfig.pointSampler->Sample(nullptr, nullptr, config->eyePath, 0, 0);
    ((CPixelSampler *) config->eyeConfig.dirSampler)->SetPixel(nx, ny);

    // Provide a node for the pixel sampling

    pixNode = config->eyePath->Next();

    if ( pixNode == nullptr ) {
        pixNode = new CPathNode;
        config->eyePath->Attach(pixNode);
    }

    nextNode = pixNode->Next();
    if ( nextNode == nullptr ) {
        nextNode = new CPathNode;
        pixNode->Attach(nextNode);
    }

    config->nx = nx;
    config->ny = ny;
    if ( config->bcfg->doDensityEstimation ) {
        config->fluxToRadFactor = computeFluxToRadFactor(nx, ny);
    } else {
        config->fluxToRadFactor = computeFluxToRadFactor(nx, ny);
    }

    for ( i = 0; i < config->bcfg->samplesPerPixel; i++ ) {
        if ( config->eyeConfig.maxDepth > 1 ) {
            // Generate an eye path
            strat.sample(&x_1, &x_2);

            config->eyePath->m_rayType = Starts;

            Vector2D tmpVec2D = config->screen->getPixelCenter((int) (x_1), (int) (x_2));
            config->xSample = tmpVec2D.u; // pix_x + (GLOBAL_camera_mainCamera.pixh * x_1);
            config->ySample = tmpVec2D.v; //pix_y + (GLOBAL_camera_mainCamera.pixv * x_2);

            if ( config->eyeConfig.dirSampler->Sample(nullptr, config->eyePath,
                                                      pixNode, x_1, x_2)) {
                pixNode->AssignBsdfAndNormal();

                config->eyeConfig.tracePath(nextNode);
            }
        } else {
            config->eyePath->m_rayType = Stops;
        }

        // Generate a light path

        if ( config->lightConfig.maxDepth > 0 ) {
            config->lightPath = config->lightConfig.tracePath(config->lightPath);
        } else {
            config->lightPath = nullptr;
        } // Normally this is already so, so no delete necessary ?!

        // Connect all endpoints and compute contribution

        BPCombinePaths(config);
    }

    // Radiance contributions are added to the screen buffer directly

    if ( config->bcfg->doDensityEstimation ) {
        if ( config->dBuffer != nullptr ) {
            result = config->screen->get(nx, ny);
        } else {
            result = config->dest->get(nx, ny);
        }
    } else {
        result = config->screen->get(nx, ny);
    }

    return result;
}

static void DoBPTAndSubsequentImages(BPCONFIG *config) {
    int maxSamples, nrIterations;
    char *format1 = new char[STRINGS_SIZE];
    char *format2 = new char[STRINGS_SIZE];
    char *filename = new char[STRINGS_SIZE];

    // Do some trick to render several images, with different
    // number of samples per pixel.

    // Get highest power of two < number of samples

    nrIterations = (int) (log((double) GLOBAL_rayTracing_biDirectionalPath.basecfg.samplesPerPixel) / log(2.0));
    maxSamples = (int) pow(2.0, nrIterations);

    nrIterations += 1; // First two are 1 and 1

    printf("nrIter %i, maxSamples %i, origSamples %i\n", nrIterations,
           maxSamples, GLOBAL_rayTracing_biDirectionalPath.basecfg.samplesPerPixel);

    printf("Base name '%s'\n", GLOBAL_rayTracing_biDirectionalPath.baseFilename);

    // Numbers are placed after the last point. If
    // no point is given, both ppm.gz and tif images
    // are saved.

    char *last_occ = strrchr(GLOBAL_rayTracing_biDirectionalPath.baseFilename, '.');

    if ( last_occ == nullptr ) {
        snprintf(format1, STRINGS_SIZE, "%s%%i.tif", GLOBAL_rayTracing_biDirectionalPath.baseFilename);
        snprintf(format2, STRINGS_SIZE, "%s%%i.ppm.gz", GLOBAL_rayTracing_biDirectionalPath.baseFilename);
    } else {
        strncpy(format1, GLOBAL_rayTracing_biDirectionalPath.baseFilename, last_occ - GLOBAL_rayTracing_biDirectionalPath.baseFilename);
        strcat(format1, "%i");
        strcat(format1, last_occ);
        format2[0] = '\0';
    }

    printf("Format 1 '%s'\n", format1);
    printf("Format 2 '%s'\n", format2);

    // Now trace the screen several times, each time
    // with twice the number of samples as the previous
    // time. The screenbuffer is scaled by 0.5 each iteration

    int currentSamples = 1;
    int totalSamples = 1;

    for ( int i = 0; i < nrIterations; i++ ) {
        // Halve the screen contribs

        if ( i > 0 ) {
            config->screen->scaleRadiance(0.5);
            config->screen->setAddScaleFactor(0.5);
        }

        config->bcfg->samplesPerPixel = currentSamples;
        config->bcfg->totalSamples = currentSamples * GLOBAL_camera_mainCamera.xSize * GLOBAL_camera_mainCamera.ySize;;

        ScreenIterateSequential((COLOR(*)(int, int, void *)) BPCalcPixel, config);

        config->screen->render();

        // Save images
        if ( format1[0] != '\0' ) {
            snprintf(filename, STRINGS_SIZE, format1, totalSamples);
            config->screen->writeFile(filename);
        }
        if ( format2[0] != '\0' ) {
            snprintf(filename, STRINGS_SIZE, format2, totalSamples);
            config->screen->writeFile(filename);
        }


        if ( i > 0 ) {
            currentSamples *= 2;
        }

        totalSamples += currentSamples;
    }

    delete[] format1;
    delete[] format2;
    delete[] filename;
}

void DoBPTDensityEstimation(BPCONFIG *config) {
    char *fname = new char[STRINGS_SIZE];

    // mainInit the screens, one reference one destination

    config->ref = new ScreenBuffer(nullptr);
    config->dest = new ScreenBuffer(nullptr);

    config->ref->setFactor(1.0);
    config->dest->setFactor(1.0);

    config->dBuffer = new CDensityBuffer(config->ref, config->bcfg);

    if ( config->bcfg->useSpars ) {
        config->ref2 = new ScreenBuffer(nullptr);
        config->dest2 = new ScreenBuffer(nullptr);

        config->ref2->setFactor(1.0);
        config->dest2->setFactor(1.0);

        config->dBuffer2 = new CDensityBuffer(config->ref2, config->bcfg);
    } else {
        config->ref2 = nullptr;
        config->dest2 = nullptr;
        config->dBuffer2 = nullptr;
    }

    // First make a run with D.E. to build ref

    config->bcfg->samplesPerPixel = 1;
    config->bcfg->totalSamples = (config->bcfg->samplesPerPixel *
            config->ref->getHRes() * config->ref->getVRes());

    config->deStoreHits = true;

    // Do the run
    ScreenIterateSequential((COLOR(*)(int, int, void *)) BPCalcPixel, config);

    // Now we have a noisy screen in dest and hits in dbuffer

    // Density estimation

    config->dBuffer->Reconstruct(); // Estimates ref with fixed kernel width
    config->ref->render();
    //  config->ref->WriteFile("./firstreffix.ppm.gz");

    if ( config->dBuffer2 ) {
        config->dBuffer2->Reconstruct(); // Estimates ref with fixed kernel width
        config->ref2->render();
        //    config->ref2->WriteFile("./firstref2fix.ppm.gz");
    }

    // Now reconstruct the hits using variable kernels
    config->dBuffer->ReconstructVariable(config->dest, 5.0);
    config->dest->render();
    //  config->dest->WriteFile("./firstrefvar.ppm.gz");

    if ( config->dBuffer2 ) {
        config->dBuffer2->ReconstructVariable(config->dest2, 1.5);
        config->dest2->render();
        //    config->dest2->WriteFile("./firstref2var.ppm.gz");
    }

    delete config->dBuffer; // Not needed anymore
    config->dBuffer = nullptr;


    if ( config->dBuffer2 ) {
        delete config->dBuffer2; // Not needed anymore
        config->dBuffer2 = nullptr;
    }

    // We have a better approximation in dest based
    // on some samples per pixel BPT.

    // We will now do subsequent BPT passes, adding the
    // hits directly to dest.

    config->deStoreHits = false;

    int nrIterations = (int) floor(log(GLOBAL_rayTracing_biDirectionalPath.basecfg.samplesPerPixel) / (log(2)));
    int maxSamples = (int) pow(2.0, nrIterations);

    printf("Doing %i iterations, thus %i samples per pixel\n", nrIterations,
           maxSamples);

    int oldSPP = config->bcfg->samplesPerPixel; // These were stored
    int newSPP = oldSPP;
    int oldTotalSPP = oldSPP;
    int newTotalSPP = oldTotalSPP + newSPP;

    for ( int i = 1; i <= nrIterations && !GLOBAL_rayCasting_interruptRaytracing; i++ ) {
        printf("Doing run with %i samples, %i samples already done\n", newSPP,
               oldTotalSPP);

        // copy dest to ref

        config->ref->copy(config->dest);

        if ( config->ref2 ) {
            config->ref2->copy(config->dest2);
        }


        // Rescale dest : Nold / Nnew
        config->dest->scaleRadiance((float) oldTotalSPP / (float) newTotalSPP);

        // Set scale factor for added radiance in this run
        config->dest->setAddScaleFactor((float) newSPP / (float) newTotalSPP);

        if ( config->dest2 ) {
            // Rescale dest : Nold / Nnew
            config->dest2->scaleRadiance((float) oldTotalSPP / (float) newTotalSPP);

            // Set scale factor for added radiance in this run
            config->dest2->setAddScaleFactor((float) newSPP / (float) newTotalSPP);
        }

        // Set current vars

        config->bcfg->samplesPerPixel = newSPP;
        config->bcfg->totalSamples = (config->bcfg->samplesPerPixel *
                config->ref->getHRes() * config->ref->getVRes());

        config->scaleSamples = newTotalSPP; // Base kernel size on total #s/pix

        // Iterate screen : Nnew - Nold, using an appropriate scale factor

        ScreenIterateSequential((COLOR(*)(int, int, void *)) BPCalcPixel, config);

        // Render screen & write

        config->dest->render();
        snprintf(fname, STRINGS_SIZE, "deScreen%i.ppm.gz", newTotalSPP);
        //    config->dest->WriteFile(fileName);


        if ( config->dest2 ) {
            config->dest2->render();
            snprintf(fname, STRINGS_SIZE, "de2Screen%i.ppm.gz", newTotalSPP);
            //      config->dest2->WriteFile(fileName);

            // Merge two images (just add!) into screen

            config->screen->merge(config->dest, config->dest2);
            config->screen->render();
            snprintf(fname, STRINGS_SIZE, "deMRGScreen%i.ppm.gz", newTotalSPP);
            //      config->screen->WriteFile(fileName);
        } else {
            config->screen->copy(config->dest);
        }

        // Update vars

        oldSPP = newSPP;
        newSPP = newSPP * 2;
        oldTotalSPP = newTotalSPP;
        newTotalSPP = oldTotalSPP + newSPP;
    }

    if ( GLOBAL_rayCasting_interruptRaytracing ) {
        if ( config->ref2 ) {
            config->screen->merge(config->ref, config->ref2);
        } else {
            config->screen->copy(config->ref);  // Interrupt, use last reference image
        }
    }

    delete config->dest;
    delete config->ref;
    if ( config->ref2 ) {
        delete config->ref2;
    }
    if ( config->dest2 ) {
        delete config->dest2;
    }
}

/**
Raytrace the current scene as seen with the current camera. If fp
is not a nullptr pointer, write the raytraced image to the file
pointed to by 'fp'
*/
static void
bidirPathTrace(ImageOutputHandle *ip, java::ArrayList<Patch *> * /*scenePatches*/, java::ArrayList<Patch *> *lightPatches) {
    // Install the samplers to be used in the state

    BPCONFIG config;

    // Copy base config (so that rendering is independent of GUI)
    config.bcfg = new BP_BASECONFIG;
    *(config.bcfg) = GLOBAL_rayTracing_biDirectionalPath.basecfg;
    config.bcfg->totalSamples = GLOBAL_rayTracing_biDirectionalPath.basecfg.samplesPerPixel * GLOBAL_camera_mainCamera.xSize * GLOBAL_camera_mainCamera.ySize;

    config.dBuffer = nullptr;

    // Eye and light path sampling config
    config.eyeConfig.pointSampler = new CEyeSampler;
    config.eyeConfig.dirSampler = new CPixelSampler;
    config.eyeConfig.surfaceSampler = new CBsdfSampler;
    config.eyeConfig.surfaceSampler->SetComputeFromNextPdf(true);
    config.eyeConfig.surfaceSampler->SetComputeBsdfComponents(GLOBAL_rayTracing_biDirectionalPath.basecfg.useSpars);

    if ( GLOBAL_rayTracing_biDirectionalPath.basecfg.sampleImportantLights ) {
        config.eyeConfig.neSampler = new CImportantLightSampler;
    } else {
        config.eyeConfig.neSampler = new CUniformLightSampler;
    }

    config.eyeConfig.minDepth = GLOBAL_rayTracing_biDirectionalPath.basecfg.minimumPathDepth;

    if ( GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumEyePathDepth < 1 ) {
        fprintf(stderr, "Maximum Eye Path Length too small (<1), using 1\n");
        config.eyeConfig.maxDepth = 1;
    } else {
        config.eyeConfig.maxDepth = GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumEyePathDepth;
    }

    config.lightConfig.pointSampler = new CUniformLightSampler;
    config.lightConfig.dirSampler = new CLightDirSampler;
    config.lightConfig.surfaceSampler = new CBsdfSampler;
    config.lightConfig.surfaceSampler->SetComputeFromNextPdf(true);
    config.lightConfig.surfaceSampler->SetComputeBsdfComponents(GLOBAL_rayTracing_biDirectionalPath.basecfg.useSpars);

    config.lightConfig.minDepth = GLOBAL_rayTracing_biDirectionalPath.basecfg.minimumPathDepth;
    config.lightConfig.maxDepth = GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumLightPathDepth;
    config.lightConfig.neSampler = nullptr; // eyeSampler ?

    // config.maxCombinedLength = bidir.basecfg.maximumPathDepth;

    config.screen = new ScreenBuffer(nullptr);
    config.screen->setFactor(1.0); // We're storing plain radiance

    config.eyePath = nullptr;
    config.lightPath = nullptr;

    // SPaR configuration

    CLeSpar *leSpar = nullptr;
    CLDSpar *ldSpar = nullptr;

    if ( GLOBAL_rayTracing_biDirectionalPath.basecfg.useSpars ) {
        CSparConfig *sc = &config.sparConfig;

        sc->m_bcfg = config.bcfg; // Share base config options

        config.sparList = new CSparList;

        leSpar = new CLeSpar;
        ldSpar = new CLDSpar;

        sc->m_leSpar = leSpar;
        sc->m_ldSpar = ldSpar;

        leSpar->init(sc);
        ldSpar->init(sc);

        config.sparList->Add(leSpar);
        config.sparList->Add(ldSpar);
    }

    if ( GLOBAL_rayTracing_biDirectionalPath.saveSubsequentImages ) {
        DoBPTAndSubsequentImages(&config);
    } else if ( config.bcfg->doDensityEstimation ) {
        DoBPTDensityEstimation(&config);
    } else if ( !GLOBAL_rayTracing_biDirectionalPath.basecfg.progressiveTracing ) {
        ScreenIterateSequential((COLOR(*)(int, int, void *)) BPCalcPixel, &config);
    } else {
        ScreenIterateProgressive((COLOR(*)(int, int, void *)) BPCalcPixel, &config);
    }

    config.screen->render();

    if ( ip ) {
        config.screen->writeFile(ip);
    }

    if ( GLOBAL_rayTracing_biDirectionalPath.lastscreen ) {
        delete GLOBAL_rayTracing_biDirectionalPath.lastscreen;
    }
    GLOBAL_rayTracing_biDirectionalPath.lastscreen = config.screen;


    if ( GLOBAL_rayTracing_biDirectionalPath.basecfg.useSpars ) {
        delete config.sparList;

        delete leSpar;
        delete ldSpar;
    }

    delete config.eyeConfig.pointSampler;
    delete config.eyeConfig.dirSampler;
    delete config.eyeConfig.surfaceSampler;
    delete config.eyeConfig.neSampler;

    delete config.lightConfig.pointSampler;
    delete config.lightConfig.dirSampler;
    delete config.lightConfig.surfaceSampler;

    if ( config.dBuffer ) {
        delete config.dBuffer;
        config.dBuffer = nullptr;
    }

    delete config.bcfg;
}

static int BidirPathRedisplay() {
    if ( GLOBAL_rayTracing_biDirectionalPath.lastscreen ) {
        GLOBAL_rayTracing_biDirectionalPath.lastscreen->render();
        return true;
    } else {
        return false;
    }
}

static int BidirPathSaveImage(ImageOutputHandle *ip) {
    if ( ip && GLOBAL_rayTracing_biDirectionalPath.lastscreen ) {
        GLOBAL_rayTracing_biDirectionalPath.lastscreen->sync();
        GLOBAL_rayTracing_biDirectionalPath.lastscreen->writeFile(ip);
        return true;
    } else {
        return false;
    }
}

static void
BidirPathInterrupt() {
    GLOBAL_rayCasting_interruptRaytracing = true;
}

static void
BidirPathInit(java::ArrayList<Patch *> *lightPatches) {
    // mainInit the light list
    if ( GLOBAL_lightList ) {
        delete GLOBAL_lightList;
    }
    GLOBAL_lightList = new CLightList(lightPatches);
}

static void
BidirPathTerminate() {
    if ( GLOBAL_rayTracing_biDirectionalPath.lastscreen ) {
        delete GLOBAL_rayTracing_biDirectionalPath.lastscreen;
    }
    GLOBAL_rayTracing_biDirectionalPath.lastscreen = (ScreenBuffer *) 0;
}

Raytracer GLOBAL_raytracing_biDirectionalPathMethod =
{
    "BidirectionalPathTracing",
    4,
    "Bidirectional Path Tracing",
    biDirectionalPathDefaults,
    biDirectionalPathParseOptions,
    BidirPathInit,
    bidirPathTrace,
    BidirPathRedisplay,
    BidirPathSaveImage,
    BidirPathInterrupt,
    BidirPathTerminate
};
