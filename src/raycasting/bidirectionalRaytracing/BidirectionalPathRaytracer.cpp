#include <cstring>

#include "common/error.h"
#include "common/stratification.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/eyesampler.h"
#include "raycasting/bidirectionalRaytracing/LightSampler.h"
#include "raycasting/bidirectionalRaytracing/LightDirSampler.h"
#include "raycasting/raytracing/bsdfsampler.h"
#include "raycasting/raytracing/samplertools.h"
#include "raycasting/raytracing/screeniterate.h"
#include "raycasting/raytracing/densitybuffer.h"
#include "raycasting/raytracing/densitykernel.h"
#include "raycasting/bidirectionalRaytracing/spar.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracer.h"

// Persistent biDirPath state, contains actual GUI state and some other stuff
BIDIRPATH_STATE GLOBAL_rayTracing_biDirectionalPath;

/**
Bidirectional path tracing configuration structure.
non persistently used each time an image is rendered
*/
class BidirectionalPathTracingConfiguration {
  public:
    BP_BASECONFIG *baseConfig;

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
    SimpleRaytracingPathNode *eyePath;
    SimpleRaytracingPathNode *lightPath;

    // SPaR configuration
    SparConfig sparConfig;
    CSparList *sparList;
    bool deStoreHits;
    ScreenBuffer *ref;
    ScreenBuffer *dest;
    ScreenBuffer *ref2;
    ScreenBuffer *dest2;
    CKernel2D kernel;
    int scaleSamples;

    BidirectionalPathTracingConfiguration():
        baseConfig(),
        eyeConfig(),
        lightConfig(),
        screen(),
        fluxToRadFactor(),
        nx(),
        ny(),
        pdfLNE(),
        dBuffer(),
        dBuffer2(),
        xSample(),
        ySample(),
        eyePath(),
        lightPath(),
        sparConfig(),
        sparList(),
        deStoreHits(),
        ref(),
        dest(),
        ref2(),
        dest2(),
        kernel(),
        scaleSamples()
    {}
};

#define STRINGS_SIZE 300

static bool
spikeCheck(ColorRgb color) {
    double colAvg = color.average();

    if ( ISNAN(colAvg) ) {
        printf("COL ");
        color.print(stdout);
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

static void
addWithSpikeCheck(
    BidirectionalPathTracingConfiguration *config,
    CBiPath * /*path*/,
    int nx,
    int ny,
    float pix_x,
    float pix_y,
    ColorRgb f,
    bool radSample = false)
{
    if ( config->baseConfig->doDensityEstimation ) {
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
            db->add(pix_x, pix_y, f);
        } else {
            // Now splat directly into the dest screen
            Vector2D center;
            ColorRgb g;

            // Get the center:
            center.u = pix_x;
            center.v = pix_y;

            // Convert f into DE quantities
            float factor = rs->getPixXSize() * rs->getPixYSize()
                           * (float) config->baseConfig->totalSamples;

            if ( f.average() > EPSILON ) {
                g.scaledCopy(factor, f); // Undo part of flux to rad factor

                config->kernel.varCover(center, g, rs, ds,
                                        (int) config->baseConfig->totalSamples, config->scaleSamples,
                                        baseSize);
            }
            return;
        }
    }

    if ( config->baseConfig->eliminateSpikes ) {
        if ( !spikeCheck(f) ) {
            config->screen->add(nx, ny, f);
        } else {
            // Wanna see the spikes !
            //  config->screen->Add(config->nx, config->ny, f);
        }
    } else {
        config->screen->add(nx, ny, f);
    }
}

void
handlePathX0(
    Camera *camera,
    BidirectionalPathTracingConfiguration *config,
    CBiPath *path)
{
    PhongEmittanceDistributionFunction *endingEdf = path->m_eyeEndNode->m_hit.material ? path->m_eyeEndNode->m_hit.material->edf : nullptr;
    ColorRgb oldBsdfEval;
    ColorRgb f;
    ColorRgb fRad;
    BsdfComp oldBsdfComp;
    float factor;
    double pdfLNE;
    double oldPdfLNE;
    double oldPDFLightEval;
    double oldPDFDirEval;
    double oldRRPDFLightEval;
    double oldRRPDFDirEval;
    float pdf = 1.0f;
    float weight = 1.0;
    SimpleRaytracingPathNode *eyePrevNode;
    SimpleRaytracingPathNode*eyeEndNode;

    if ( path->m_eyeSize > config->baseConfig->maximumPathDepth ) {
        return;
    }

    if ( endingEdf != nullptr || config->baseConfig->useSpars ) {
        eyeEndNode = path->m_eyeEndNode;

        // Store the Bsdf and PDF evaluations that will be overwritten
        // by other values needed to compute the contribution

        eyePrevNode = eyeEndNode->previous(); // Always != nullptr

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
                        config->lightConfig.pointSampler->evalPDF(camera, nullptr, eyeEndNode);
                PNAN(eyeEndNode->m_pdfFromNext);

                eyeEndNode->m_rrPdfFromNext = 1.0; // Light point: no Russian R.
            } else {
                // Impossible to generate light point with a light sub path
                eyeEndNode->m_pdfFromNext = 0.0;
                eyeEndNode->m_rrPdfFromNext = 0.0;
            }

            if ( config->lightConfig.maxDepth > 1 ) {
                eyePrevNode->m_pdfFromNext =
                        config->lightConfig.dirSampler->evalPDF(camera, eyeEndNode, eyePrevNode);
                PNAN(eyePrevNode->m_pdfFromNext);

                eyePrevNode->m_rrPdfFromNext = 1.0;
            } else {
                eyePrevNode->m_pdfFromNext = 0.0;
                eyePrevNode->m_rrPdfFromNext = 0.0;
            }

            // Compute
            if ((config->baseConfig->sampleImportantLights) && (config->lightConfig.maxDepth > 0)
                && (path->m_eyeSize > 2) ) {
                pdfLNE = config->eyeConfig.neSampler->evalPDF(camera, eyePrevNode, eyeEndNode);
            } else {
                pdfLNE = eyeEndNode->m_pdfFromNext; // same sampling as light path
            }

            PNAN(pdfLNE);
            path->m_pdfLNE = pdfLNE;
        }

        // Path radiance evaluation
        path->m_geomConnect = 1.0; // Fake connection for X_0 paths

        if ( config->baseConfig->useSpars ) {
            config->sparList->handlePath(&config->sparConfig, path, &fRad, &f);
            factor = 1.0; // pdf and weight already taken into account
        } else {
            f = path->EvalRadiance();
            factor = (float)path->EvalPDFAndWeight(config->baseConfig, &pdf, &weight);
        }

        factor *= (float)config->fluxToRadFactor / (float)config->baseConfig->samplesPerPixel;

        if ( config->baseConfig->useSpars ) {
            fRad.scale(factor);
            addWithSpikeCheck(config, path, config->nx, config->ny,
                              config->xSample, config->ySample, fRad, true);
            f.scale(factor);
            addWithSpikeCheck(config, path, config->nx, config->ny,
                              config->xSample, config->ySample, f, false);
        } else {
            f.scale(factor);
            addWithSpikeCheck(config, path, config->nx, config->ny,
                              config->xSample, config->ySample, f);
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

ColorRgb
computeNeFluxEstimate(
    Camera *camera,
    BidirectionalPathTracingConfiguration *config,
    CBiPath *path,
    float *pPdf = nullptr,
    float *pWeight = nullptr,
    ColorRgb *fRad = nullptr)
{
    SimpleRaytracingPathNode *eyePrevNode;
    SimpleRaytracingPathNode *lightPrevNode;
    ColorRgb oldBsdfL;
    ColorRgb oldBsdfE;
    BsdfComp oldBsdfCompL;
    BsdfComp oldBsdfCompE;
    double oldPdfL;
    double oldPdfE;
    double oldRRPdfL;
    double oldRRPdfE;
    double oldPdfLP = 0.0;
    double oldPdfEP = 0.0;
    double oldRRPdfLP = 0.0;
    double oldRRPdfEP = 0.0;
    ColorRgb f;
    SimpleRaytracingPathNode *eyeEndNode;
    SimpleRaytracingPathNode *lightEndNode;

    // Store PDF and BSDF evaluations that will be overwritten
    eyeEndNode = path->m_eyeEndNode;
    lightEndNode = path->m_lightEndNode;
    eyePrevNode = eyeEndNode->previous();
    lightPrevNode = lightEndNode->previous();

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

    // Connect the sub-paths
    path->m_geomConnect =
            pathNodeConnect(camera, eyeEndNode, lightEndNode,
                            &config->eyeConfig, &config->lightConfig,
                            CONNECT_EL | CONNECT_LE | FILL_OTHER_PDF,
                            BSDF_ALL_COMPONENTS, BSDF_ALL_COMPONENTS, &path->m_dirEL);

    vectorScale(-1, path->m_dirEL, path->m_dirLE);

    // Evaluate radiance and pdf and weight
    if ( config->baseConfig->useSpars ) {
        // f = config->sparList->photonMapHandlePath(&config->sparConfig, path);
        config->sparList->handlePath(&config->sparConfig, path, fRad, &f);
    } else {
        f = path->EvalRadiance();

        float factor = path->EvalPDFAndWeight(config->baseConfig, pPdf, pWeight);
        f.scale(factor); // Flux estimate
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

/**
handlePathXx : handle a path with eyeSize >= 2 and
light size >= 1
*/
static void
handlePathXx(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    Background *sceneBackground,
    BidirectionalPathTracingConfiguration *config,
    CBiPath *path)
{
    ColorRgb f;
    ColorRgb fRad;
    double oldPdfLNE = 0.0;
    float pdf;
    float weight;
    bool doLNE;
    SimpleRaytracingPathNode newLightNode;
    SimpleRaytracingPathNode *oldLightPath = nullptr;
    SimpleRaytracingPathNode *oldLightEndNode = nullptr;
    int oldLightSize = 0;

    if ( (path->m_eyeSize + path->m_lightSize) > config->baseConfig->maximumPathDepth ) {
        return;
    }

    // Check if we need to sample another light point with
    // importance sampling.

    doLNE = (path->m_lightSize == 1) && config->baseConfig->sampleImportantLights;

    if ( doLNE ) {
        // Replace the current light path with new importance sampled path
        oldLightPath = path->m_lightPath;
        oldLightSize = path->m_lightSize;
        oldLightEndNode = path->m_lightEndNode;

        path->m_lightPath = &newLightNode;

        newLightNode.m_pdfFromPrev = 0.0;
        newLightNode.m_pdfFromNext = 0.0;

        if ( !config->eyeConfig.neSampler->sample(
            camera,
            sceneWorldVoxelGrid,
            sceneBackground,
            nullptr,
            path->m_eyeEndNode,
            &newLightNode,
            drand48(),
            drand48()) ) {
            // No light point sampled, no contribution possible

            path->m_lightPath = oldLightPath;
            return;
        }

        // Compute the normal sampling pdf for the new path/point
        oldPdfLNE = path->m_pdfLNE;
        path->m_pdfLNE = newLightNode.m_pdfFromPrev;
        newLightNode.m_pdfFromPrev =
                config->lightConfig.pointSampler->evalPDF(camera, nullptr, &newLightNode);

        PNAN(newLightNode.m_pdfFromPrev);
        PNAN(path->m_pdfLNE);

        path->m_lightEndNode = &newLightNode;
    }

    if ( pathNodesVisible(sceneWorldVoxelGrid, path->m_eyeEndNode, path->m_lightEndNode) ) {
        f = computeNeFluxEstimate(camera, config, path, &pdf, &weight, &fRad);

        float factor = (float)config->fluxToRadFactor / (float)config->baseConfig->samplesPerPixel;
        f.scale(factor);
        addWithSpikeCheck(config, path, config->nx, config->ny,
                          config->xSample, config->ySample, f);

        if ( config->baseConfig->useSpars ) {
            fRad.scale(factor);
            addWithSpikeCheck(config, path, config->nx, config->ny,
                              config->xSample, config->ySample, fRad, true);
        }
    }

    if ( doLNE ) {
        // Restore the current light path
        path->m_lightPath = oldLightPath;
        path->m_lightSize = oldLightSize;
        path->m_lightEndNode = oldLightEndNode;
        path->m_pdfLNE = oldPdfLNE;
    }
}

static void
handlePath1X(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    BidirectionalPathTracingConfiguration *config,
    CBiPath *path)
{
    double oldPdfLNE;

    if ( (path->m_eyeSize + path->m_lightSize) > config->baseConfig->maximumPathDepth ) {
        return;
    }

    // We don't do light importance sampling for particle tracing paths
    oldPdfLNE = path->m_pdfLNE;
    path->m_pdfLNE = config->lightPath->m_pdfFromPrev;

    // First we need to determine if the lightEndNode can be seen from
    // the camera. At the same time the pixel hit is computed
    float pixX;
    float pixY;
    if ( eyeNodeVisible(
            camera,
            sceneVoxelGrid,
            path->m_eyeEndNode,
            path->m_lightEndNode,
            &pixX,
            &pixY) ) {
        int nx;
        int ny;
        ColorRgb f;
        ColorRgb fRad;
        float pdf;
        float weight;

        // Visible !
        f = computeNeFluxEstimate(camera, config, path, &pdf, &weight, &fRad);

        config->screen->getPixel(pixX, pixY, &nx, &ny);

        float factor = (computeFluxToRadFactor(camera, nx, ny) / (float) config->baseConfig->totalSamples);
        f.scale(factor);

        addWithSpikeCheck(config, path, nx, ny, pixX, pixY, f);

        if ( config->baseConfig->useSpars ) {
            fRad.scale(factor);
            addWithSpikeCheck(config, path, nx, ny, pixX, pixY, fRad, true);
        }
    }

    // Restore
    path->m_pdfLNE = oldPdfLNE;
}

static void
bpCombinePaths(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    BidirectionalPathTracingConfiguration *config)
{
    int eyeSize;
    int lightSize;
    bool eyeSubPathDone;
    bool lightSubPathDone;
    SimpleRaytracingPathNode *eyeEndNode;
    SimpleRaytracingPathNode *lightEndNode;
    SimpleRaytracingPathNode *eyePath;
    SimpleRaytracingPathNode *lightPath;
    CBiPath path;

    eyePath = config->eyePath;
    lightPath = config->lightPath;

    // Compute pdfLNE for the second point in the light path
    if ( config->lightPath ) {
        // Single light point get importance sampled, so compute
        // the pdf for it, in order to get correct weights.
        if ( config->baseConfig->sampleImportantLights && config->lightPath->next() ) {
            config->pdfLNE =
                    config->eyeConfig.neSampler->evalPDF(camera, lightPath->next(), lightPath);
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
    eyeSize = 1;  // Always include the eye point!
    eyeEndNode = eyePath;

    while ( !eyeSubPathDone ) {
        // Handle direct hit on light (lightSize = 0)

        if ( eyeSize > 1 ) {
            path.m_eyeSize = eyeSize;
            path.m_eyeEndNode = eyeEndNode;
            path.m_lightSize = 0;
            path.m_lightEndNode = nullptr;

            handlePathX0(camera, config, &path);
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
                handlePathXx(camera, sceneVoxelGrid, sceneBackground, config, &path);
            } else {
                path.m_eyeSize = eyeSize;
                path.m_eyeEndNode = eyeEndNode;
                path.m_lightSize = lightSize;
                path.m_lightEndNode = lightEndNode;
                handlePath1X(camera, sceneVoxelGrid, config, &path);
            }

            if ( lightEndNode->ends() ) {
                lightSubPathDone = true;
            } else {
                lightSize++;
                lightEndNode = lightEndNode->next();
            }
        }

        if ( eyeEndNode->ends() ) {
            eyeSubPathDone = true;
        } else {
            eyeSize++;
            eyeEndNode = eyeEndNode->next();
        }
    }
}

static ColorRgb
bpCalcPixel(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    int nx,
    int ny,
    BidirectionalPathTracingConfiguration *config)
{
    double x1;
    double x2;
    ColorRgb result;
    StratifiedSampling2D stratifiedSampling2D(config->baseConfig->samplesPerPixel);
    SimpleRaytracingPathNode *pixNode;
    SimpleRaytracingPathNode *nextNode;

    result.clear();

    // We sample the eye here since it's always the same point
    if ( config->eyePath == nullptr ) {
        config->eyePath = new SimpleRaytracingPathNode;
    }

    config->eyeConfig.pointSampler->sample(camera, sceneVoxelGrid, sceneBackground, nullptr, nullptr, config->eyePath, 0, 0);
    ((CPixelSampler *) config->eyeConfig.dirSampler)->SetPixel(camera, nx, ny);

    // Provide a node for the pixel sampling
    pixNode = config->eyePath->next();

    if ( pixNode == nullptr ) {
        pixNode = new SimpleRaytracingPathNode;
        config->eyePath->attach(pixNode);
    }

    nextNode = pixNode->next();
    if ( nextNode == nullptr ) {
        nextNode = new SimpleRaytracingPathNode;
        pixNode->attach(nextNode);
    }

    config->nx = nx;
    config->ny = ny;
    config->fluxToRadFactor = computeFluxToRadFactor(camera, nx, ny);

    for ( int i = 0; i < config->baseConfig->samplesPerPixel; i++ ) {
        if ( config->eyeConfig.maxDepth > 1 ) {
            // Generate an eye path
            stratifiedSampling2D.sample(&x1, &x2);

            config->eyePath->m_rayType = STARTS;

            Vector2D tmpVec2D = config->screen->getPixelCenter((int) (x1), (int) (x2));
            config->xSample = tmpVec2D.u; // pix_x + (camera.pixelWidth * x1);
            config->ySample = tmpVec2D.v; // pix_y + (camera.pixelHeight * x2);

            if ( config->eyeConfig.dirSampler->sample(camera, sceneVoxelGrid, sceneBackground, nullptr, config->eyePath, pixNode, x1, x2) ) {
                pixNode->assignBsdfAndNormal();
                config->eyeConfig.tracePath(camera, sceneVoxelGrid, sceneBackground, nextNode);
            }
        } else {
            config->eyePath->m_rayType = STOPS;
        }

        // Generate a light path
        if ( config->lightConfig.maxDepth > 0 ) {
            config->lightPath = config->lightConfig.tracePath(camera, sceneVoxelGrid, sceneBackground, config->lightPath);
        } else {
            // Normally this is already so, so no delete necessary ?!
            config->lightPath = nullptr;
        }

        // Connect all endpoints and compute contribution
        bpCombinePaths(camera, sceneVoxelGrid, sceneBackground, config);
    }

    // Radiance contributions are added to the screen buffer directly
    if ( config->baseConfig->doDensityEstimation ) {
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

static void
doBptAndSubsequentImages(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    BidirectionalPathTracingConfiguration *config)
{
    int maxSamples;
    int nrIterations;
    char *format1 = new char[STRINGS_SIZE];
    char *format2 = new char[STRINGS_SIZE];
    char *filename = new char[STRINGS_SIZE];

    // Do some trick to render several images, with different
    // number of samples per pixel.

    // Get the highest power of two < number of samples
    nrIterations = (int)(std::log((double) GLOBAL_rayTracing_biDirectionalPath.basecfg.samplesPerPixel) / std::log(2.0));
    maxSamples = (int)std::pow(2.0, nrIterations);

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
    // time. The screen buffer is scaled by 0.5 each iteration

    int currentSamples = 1;
    int totalSamples = 1;

    for ( int i = 0; i < nrIterations; i++ ) {
        // Halve the screen contributions
        if ( i > 0 ) {
            config->screen->scaleRadiance(0.5);
            config->screen->setAddScaleFactor(0.5);
        }

        config->baseConfig->samplesPerPixel = currentSamples;
        config->baseConfig->totalSamples = currentSamples * camera->xSize * camera->ySize;

        screenIterateSequential(
            camera,
            sceneVoxelGrid,
            sceneBackground,
            (ColorRgb(*)(Camera *, VoxelGrid *, Background *, int, int, void *))bpCalcPixel,
            config);

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

static void
doBptDensityEstimation(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    BidirectionalPathTracingConfiguration *config)
{
    char *fileName = new char[STRINGS_SIZE];

    // mainInitApplication the screens, one reference one destination
    config->ref = new ScreenBuffer(nullptr, camera);
    config->dest = new ScreenBuffer(nullptr, camera);

    config->ref->setFactor(1.0);
    config->dest->setFactor(1.0);

    config->dBuffer = new CDensityBuffer(config->ref, config->baseConfig);

    if ( config->baseConfig->useSpars ) {
        config->ref2 = new ScreenBuffer(nullptr, camera);
        config->dest2 = new ScreenBuffer(nullptr, camera);

        config->ref2->setFactor(1.0);
        config->dest2->setFactor(1.0);

        config->dBuffer2 = new CDensityBuffer(config->ref2, config->baseConfig);
    } else {
        config->ref2 = nullptr;
        config->dest2 = nullptr;
        config->dBuffer2 = nullptr;
    }

    // First make a run with D.E. to build ref
    config->baseConfig->samplesPerPixel = 1;
    config->baseConfig->totalSamples = (config->baseConfig->samplesPerPixel *
                                        config->ref->getHRes() * config->ref->getVRes());

    config->deStoreHits = true;

    // Do the run
    screenIterateSequential(
        camera,
        sceneVoxelGrid,
        sceneBackground,
        (ColorRgb(*)(Camera *, VoxelGrid *, Background *, int, int, void *))bpCalcPixel,
        config);

    // Now we have a noisy screen in dest and hits in double buffer

    // Density estimation

    config->dBuffer->reconstruct(); // Estimates ref with fixed kernel width
    config->ref->render();

    if ( config->dBuffer2 ) {
        config->dBuffer2->reconstruct(); // Estimates ref with fixed kernel width
        config->ref2->render();
    }

    // Now reconstruct the hits using variable kernels
    config->dBuffer->reconstructVariable(config->dest, 5.0);
    config->dest->render();

    if ( config->dBuffer2 ) {
        config->dBuffer2->reconstructVariable(config->dest2, 1.5);
        config->dest2->render();
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

    int nrIterations = (int)std::floor(log(GLOBAL_rayTracing_biDirectionalPath.basecfg.samplesPerPixel) / (log(2)));
    int maxSamples = (int)std::pow(2.0, nrIterations);

    printf("Doing %i iterations, thus %i samples per pixel\n", nrIterations,
           maxSamples);

    int oldSPP = config->baseConfig->samplesPerPixel; // These were stored
    int newSPP = oldSPP;
    int oldTotalSPP = oldSPP;
    int newTotalSPP = oldTotalSPP + newSPP;

    for ( int i = 1; i <= nrIterations; i++ ) {
        printf("Doing run with %i samples, %i samples already done\n", newSPP,
               oldTotalSPP);

        // copy dest to ref

        config->ref->copy(config->dest, camera);

        if ( config->ref2 ) {
            config->ref2->copy(config->dest2, camera);
        }


        // Rescale dest: nOld / nNew
        config->dest->scaleRadiance((float) oldTotalSPP / (float) newTotalSPP);

        // Set scale factor for added radiance in this run
        config->dest->setAddScaleFactor((float) newSPP / (float) newTotalSPP);

        if ( config->dest2 ) {
            // Rescale dest: nOld / nNew
            config->dest2->scaleRadiance((float) oldTotalSPP / (float) newTotalSPP);

            // Set scale factor for added radiance in this run
            config->dest2->setAddScaleFactor((float) newSPP / (float) newTotalSPP);
        }

        // Set current vars
        config->baseConfig->samplesPerPixel = newSPP;
        config->baseConfig->totalSamples = (config->baseConfig->samplesPerPixel *
                                            config->ref->getHRes() * config->ref->getVRes());

        config->scaleSamples = newTotalSPP; // Base kernel size on total #s/pix

        // Iterate screen : nNew - nOld, using an appropriate scale factor

        screenIterateSequential(
            camera,
            sceneVoxelGrid,
            sceneBackground,
            (ColorRgb(*)(Camera* , VoxelGrid *, Background *, int, int, void *))bpCalcPixel,
            config);

        // Render screen & write

        config->dest->render();
        snprintf(fileName, STRINGS_SIZE, "deScreen%i.ppm.gz", newTotalSPP);
        //    config->dest->WriteFile(fileName);

        if ( config->dest2 ) {
            config->dest2->render();
            snprintf(fileName, STRINGS_SIZE, "de2Screen%i.ppm.gz", newTotalSPP);
            //      config->dest2->WriteFile(fileName);

            // Merge two images (just add!) into screen

            config->screen->merge(config->dest, config->dest2, camera);
            config->screen->render();
            snprintf(fileName, STRINGS_SIZE, "deMRGScreen%i.ppm.gz", newTotalSPP);
            //      config->screen->WriteFile(fileName);
        } else {
            config->screen->copy(config->dest, camera);
        }

        // Update vars
        newSPP = newSPP * 2;
        oldTotalSPP = newTotalSPP;
        newTotalSPP = oldTotalSPP + newSPP;
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
is not a nullptr pointer, write the ray traced image to the file
pointed to by 'fp'
*/
static void
biDirPathTrace(
    ImageOutputHandle *ip,
    Scene *scene,
    RadianceMethod *context,
    RenderOptions * /*renderOptions*/) {
    // Install the samplers to be used in the state

    BidirectionalPathTracingConfiguration config;

    // Copy base config (so that rendering is independent of GUI)
    config.baseConfig = new BP_BASECONFIG;
    *(config.baseConfig) = GLOBAL_rayTracing_biDirectionalPath.basecfg;
    config.baseConfig->totalSamples = GLOBAL_rayTracing_biDirectionalPath.basecfg.samplesPerPixel
        * scene->camera->xSize * scene->camera->ySize;

    config.dBuffer = nullptr;

    // Eye and light path sampling config
    config.eyeConfig.pointSampler = new CEyeSampler;
    config.eyeConfig.dirSampler = new CPixelSampler;
    config.eyeConfig.surfaceSampler = new CBsdfSampler;
    config.eyeConfig.surfaceSampler->SetComputeFromNextPdf(true);
    config.eyeConfig.surfaceSampler->SetComputeBsdfComponents(GLOBAL_rayTracing_biDirectionalPath.basecfg.useSpars);

    if ( GLOBAL_rayTracing_biDirectionalPath.basecfg.sampleImportantLights ) {
        config.eyeConfig.neSampler = new ImportantLightSampler;
    } else {
        config.eyeConfig.neSampler = new UniformLightSampler;
    }

    config.eyeConfig.minDepth = GLOBAL_rayTracing_biDirectionalPath.basecfg.minimumPathDepth;

    if ( GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumEyePathDepth < 1 ) {
        fprintf(stderr, "Maximum Eye Path Length too small (<1), using 1\n");
        config.eyeConfig.maxDepth = 1;
    } else {
        config.eyeConfig.maxDepth = GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumEyePathDepth;
    }

    config.lightConfig.pointSampler = new UniformLightSampler;
    config.lightConfig.dirSampler = new LightDirSampler;
    config.lightConfig.surfaceSampler = new CBsdfSampler;
    config.lightConfig.surfaceSampler->SetComputeFromNextPdf(true);
    config.lightConfig.surfaceSampler->SetComputeBsdfComponents(GLOBAL_rayTracing_biDirectionalPath.basecfg.useSpars);

    config.lightConfig.minDepth = GLOBAL_rayTracing_biDirectionalPath.basecfg.minimumPathDepth;
    config.lightConfig.maxDepth = GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumLightPathDepth;
    config.lightConfig.neSampler = nullptr; // eyeSampler ?

    // config.maxCombinedLength = biDir.baseCfg.maximumPathDepth;
    config.screen = new ScreenBuffer(nullptr, scene->camera);
    config.screen->setFactor(1.0); // We're storing plain radiance

    config.eyePath = nullptr;
    config.lightPath = nullptr;

    // SPaR configuration

    LeSpar *leSpar = nullptr;
    LDSpar *ldSpar = nullptr;

    if ( GLOBAL_rayTracing_biDirectionalPath.basecfg.useSpars ) {
        SparConfig *sc = &config.sparConfig;

        sc->baseConfig = config.baseConfig; // Share base config options

        config.sparList = new CSparList;

        leSpar = new LeSpar;
        ldSpar = new LDSpar;

        sc->leSpar = leSpar;
        sc->ldSpar = ldSpar;

        leSpar->init(sc, context);
        ldSpar->init(sc, context);

        config.sparList->add(leSpar);
        config.sparList->add(ldSpar);
    }

    if ( GLOBAL_rayTracing_biDirectionalPath.saveSubsequentImages ) {
        doBptAndSubsequentImages(scene->camera, scene->voxelGrid, scene->background, &config);
    } else if ( config.baseConfig->doDensityEstimation ) {
        doBptDensityEstimation(scene->camera, scene->voxelGrid, scene->background, &config);
    } else if ( !GLOBAL_rayTracing_biDirectionalPath.basecfg.progressiveTracing ) {
        screenIterateSequential(
            scene->camera,
            scene->voxelGrid,
            scene->background,
            (ColorRgb(*)(Camera *, VoxelGrid *, Background *, int, int, void *))bpCalcPixel,
            &config);
    } else {
        screenIterateProgressive(
            scene->camera,
            scene->voxelGrid,
            scene->background,
            (ColorRgb(*)(Camera *, VoxelGrid *, Background *, int, int, void *))bpCalcPixel,
            &config);
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

    delete config.baseConfig;
}

static int
biDirPathReDisplay() {
    if ( GLOBAL_rayTracing_biDirectionalPath.lastscreen ) {
        GLOBAL_rayTracing_biDirectionalPath.lastscreen->render();
        return true;
    } else {
        return false;
    }
}

static int
biDirPathSaveImage(ImageOutputHandle *ip) {
    if ( ip && GLOBAL_rayTracing_biDirectionalPath.lastscreen ) {
        GLOBAL_rayTracing_biDirectionalPath.lastscreen->sync();
        GLOBAL_rayTracing_biDirectionalPath.lastscreen->writeFile(ip);
        return true;
    } else {
        return false;
    }
}

static void
biDirPathInit(java::ArrayList<Patch *> *lightPatches) {
    // mainInitApplication the light list
    if ( GLOBAL_lightList ) {
        delete GLOBAL_lightList;
    }
    GLOBAL_lightList = new LightList(lightPatches);
}

static void
biDirPathTerminate() {
    if ( GLOBAL_rayTracing_biDirectionalPath.lastscreen ) {
        delete GLOBAL_rayTracing_biDirectionalPath.lastscreen;
    }
    GLOBAL_rayTracing_biDirectionalPath.lastscreen = nullptr;
}

Raytracer
GLOBAL_raytracing_biDirectionalPathMethod = {
    "BidirectionalPathTracing",
    4,
    "Bidirectional Path Tracing",
    biDirectionalPathDefaults,
    biDirectionalPathParseOptions,
    biDirPathInit,
    biDirPathTrace,
    biDirPathReDisplay,
    biDirPathSaveImage,
    biDirPathTerminate
};
