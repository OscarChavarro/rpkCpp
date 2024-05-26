#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "common/stratification.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "PHOTONMAP/PhotonMapRadianceMethod.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/screeniterate.h"
#include "raycasting/stochasticRaytracing/rtstochasticphotonmap.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracer.h"

char StochasticRaytracer::name[38] = "Stochastic Raytracing & Final Gathers";

// Heuristic minimum distance threshold for photon map readouts
// should be tuned and dependent on scene size, ...
const float PHOTON_MAP_MIN_DIST = 0.02f;
const float PHOTON_MAP_MIN_DIST2 = PHOTON_MAP_MIN_DIST * PHOTON_MAP_MIN_DIST; // squared

StochasticRayTracingState GLOBAL_raytracing_state;

StochasticRaytracer::StochasticRaytracer() {
}

StochasticRaytracer::~StochasticRaytracer() {
}

void
StochasticRaytracer::defaults() {
    // Normal
    if ( GLOBAL_raytracing_state.samplesPerPixel == 0 ) {
        GLOBAL_raytracing_state.samplesPerPixel = 1;
    }
    GLOBAL_raytracing_state.progressiveTracing = true;

    GLOBAL_raytracing_state.doFrameCoherent = false;
    GLOBAL_raytracing_state.doCorrelatedSampling = false;
    GLOBAL_raytracing_state.baseSeed = 0xFE062134;

    GLOBAL_raytracing_state.radMode = STORED_NONE;

    GLOBAL_raytracing_state.nextEvent = true;
    GLOBAL_raytracing_state.nextEventSamples = 1;
    GLOBAL_raytracing_state.lightMode = ALL_LIGHTS;

    GLOBAL_raytracing_state.backgroundDirect = false;
    GLOBAL_raytracing_state.backgroundIndirect = true;
    GLOBAL_raytracing_state.backgroundSampling = false;

    GLOBAL_raytracing_state.scatterSamples = 1;
    GLOBAL_raytracing_state.differentFirstDG = false;
    GLOBAL_raytracing_state.firstDGSamples = 36;
    GLOBAL_raytracing_state.separateSpecular = false;

    GLOBAL_raytracing_state.reflectionSampling = BRDF_SAMPLING;

    GLOBAL_raytracing_state.minPathDepth = 5;
    GLOBAL_raytracing_state.maxPathDepth = 7;

    // Common
    GLOBAL_raytracing_state.lastScreen = nullptr;
}

const char *
StochasticRaytracer::getName() const {
    return name;
}

void
StochasticRaytracer::initialize(const java::ArrayList<Patch *> *lightPatches) const {
    // mainInitApplication the light list
    if ( GLOBAL_lightList ) {
        delete GLOBAL_lightList;
    }
    GLOBAL_lightList = new LightList(lightPatches);
}

/**
Raytrace the current scene as seen with the current camera. If fp
is not a nullptr pointer, write the ray-traced image to the file
pointed to by 'fp'
*/
void
StochasticRaytracer::execute(
    ImageOutputHandle *ip,
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions) const
{
    StochasticRaytracingConfiguration config(scene->camera, GLOBAL_raytracing_state, scene->lightSourcePatchList, radianceMethod); // config filled in by constructor

    // Frame Coherent sampling : init fixed seed
    if ( GLOBAL_raytracing_state.doFrameCoherent ) {
        srand48(GLOBAL_raytracing_state.baseSeed);
    }

    if ( !GLOBAL_raytracing_state.progressiveTracing ) {
        screenIterateSequential(
                scene->camera,
                scene->voxelGrid,
                scene->background,
                (ColorRgb(*)(Camera *, VoxelGrid *, Background *, int, int, void *))StochasticRaytracer::calcPixel,
                &config);
    } else {
        screenIterateProgressive(
                scene->camera,
                scene->voxelGrid,
                scene->background,
                (ColorRgb(*)(Camera *, VoxelGrid *, Background *, int, int, void *))StochasticRaytracer::calcPixel,
                &config);
    }

    config.screen->render();

    if ( ip ) {
        config.screen->writeFile(ip);
    }

    if ( GLOBAL_raytracing_state.lastScreen ) {
        delete GLOBAL_raytracing_state.lastScreen;
    }
    GLOBAL_raytracing_state.lastScreen = config.screen;
    config.screen = nullptr;
}

static ColorRgb
stochasticRaytracerGetRadiance(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *thisNode,
    StochasticRaytracingConfiguration *config,
    StorageReadout readout,
    int usedScatterSamples,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions);

static ColorRgb
stochasticRaytracerGetScatteredRadiance(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background * sceneBackground,
    SimpleRaytracingPathNode *thisNode,
    StochasticRaytracingConfiguration *config,
    StorageReadout readout,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions)
{
    int siCurrent; // What scatter block are we handling
    const CScatterInfo *si;

    SimpleRaytracingPathNode newNode;
    thisNode->attach(&newNode);

    ColorRgb result;
    result.clear();

    if ( (config->samplerConfig.surfaceSampler == nullptr) ||
        (thisNode->m_depth >= config->samplerConfig.maxDepth) ) {
        // No scattering
        return result;
    }

    if ( (config->siStorage.flags != NO_COMPONENTS) &&
        (readout == SCATTER) ) {
        // Do storage components
        si = &config->siStorage;
        siCurrent = -1;
    } else {
        // No direct light using storage components
        si = &config->siOthers[0];
        siCurrent = 0;
    }

    while ( siCurrent < config->siOthersCount ) {
        int numberOfSamples;

        if ( si->DoneSomePreviousBounce(thisNode) ) {
            numberOfSamples = si->nrSamplesAfter;
        } else {
            numberOfSamples = si->nrSamplesBefore;
        } // First bounce of this kind

        // A small optimisation to prevent sampling surface that
        // don't have this scattering component.

        if ( numberOfSamples > 2 ) {
            // Some bigger value may be more efficient
            ColorRgb albedo;
            albedo.clear();
            if ( thisNode->m_useBsdf != nullptr ) {
                albedo = thisNode->m_useBsdf->splitBsdfScatteredPower(&thisNode->m_hit, si->flags);
            }
            if ( albedo.average() < Numeric::EPSILON ) {
                // Skip, no contribution anyway
                numberOfSamples = 0;
            }
        }

        // Do we need to compute scattered radiance at all...
        if ((numberOfSamples > 0) && (thisNode->m_depth + 1 < config->samplerConfig.maxDepth) ) {
            double x1;
            double x2;
            double factor;
            StratifiedSampling2D stratified(numberOfSamples);
            ColorRgb radiance;
            bool doRR = thisNode->m_depth >= config->samplerConfig.minDepth;

            for ( int i = 0; i < numberOfSamples; i++ ) {
                stratified.sample(&x1, &x2);

                // Surface sampling
                if ( config->samplerConfig.surfaceSampler->sample(
                        camera,
                        sceneVoxelGrid,
                        sceneBackground,
                        thisNode->previous(),
                        thisNode,
                        &newNode, x1, x2,
                        doRR,
                        si->flags)
                     && ((newNode.m_rayType != ENVIRONMENT) || (config->backgroundIndirect)) ) {
                    if ( newNode.m_rayType != ENVIRONMENT ) {
                        newNode.assignBsdfAndNormal();
                    }

                    // Frame coherent & correlated sampling
                    if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
                        config->seedConfig.save(newNode.m_depth);
                    }

                    // Get the incoming radiance
                    if ( siCurrent == -1 ) {
                        // Storage bounce
                        radiance = stochasticRaytracerGetRadiance(
                                camera,
                                sceneVoxelGrid,
                                sceneBackground,
                                &newNode,
                                config,
                                READ_NOW,
                                numberOfSamples,
                                radianceMethod,
                                renderOptions);
                    } else {
                        radiance = stochasticRaytracerGetRadiance(
                                camera,
                                sceneVoxelGrid,
                                sceneBackground,
                                &newNode,
                                config,
                                readout,
                                numberOfSamples,
                                radianceMethod,
                                renderOptions);
                    }

                    // Frame coherent & correlated sampling
                    if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
                        config->seedConfig.Restore(newNode.m_depth);
                    }

                    // Collect outgoing radiance
                    factor = newNode.m_G / (newNode.m_pdfFromPrev * numberOfSamples);

                    radiance.scalarProductScaled(radiance, (float) factor, thisNode->m_bsdfEval);
                    result.add(radiance, result);
                }
            }
        }

        // Next scatter info block
        siCurrent++;
        if ( siCurrent < config->siOthersCount ) {
            si = &config->siOthers[siCurrent];
        }
    }

    thisNode->setNext(nullptr);
    return result;
}

static ColorRgb
srGetDirectRadiance(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *prevNode,
    StochasticRaytracingConfiguration *config,
    StorageReadout readout)
{
    ColorRgb result;
    ColorRgb radiance;
    result.clear();
    Vector3D dirEL;

    if ( (readout == READ_NOW) && (config->radMode == STORED_PHOTON_MAP) ) {
        return result;
    } // We're reading out D|G, specular not with direct light

    CNextEventSampler *nes = config->samplerConfig.neSampler;

    // Check if N.E.E. can give a contribution. I.e. not inside
    // a medium or just about to leave to vacuum
    if ( (nes != nullptr) &&
        (config->nextEventSamples > 0) &&
        (prevNode->m_depth + 1 < config->samplerConfig.maxDepth) ) {
        SimpleRaytracingPathNode lightNode;
        double x1;
        double x2;
        double geom;
        double weight;
        double cl;
        double cr;
        double factor;
        double nrs;
        bool lightsToDo = true;

        if ( config->lightMode == ALL_LIGHTS ) {
            lightsToDo = nes->ActivateFirstUnit();
        }


        while ( lightsToDo ) {
            StratifiedSampling2D stratified(config->nextEventSamples);

            for ( int i = 0; i < config->nextEventSamples; i++ ) {
                // Light sampling
                stratified.sample(&x1, &x2);

                if ( config->samplerConfig.neSampler->sample(
                    camera,
                    sceneVoxelGrid,
                    sceneBackground,
                    prevNode->previous(),
                    prevNode,
                    &lightNode,
                    x1,
                    x2,
                    true,
                    BSDF_ALL_COMPONENTS)
                    && ( pathNodesVisible(sceneVoxelGrid, prevNode, &lightNode) ) ) {
                    // Now connect for all applicable scatter-info's
                    // If no weighting between reflection sampling and
                    // next event estimation were used, only one connect
                    // using the union of different scatter info flags
                    // are necessary (=speedup)
                    int siCurrent;
                    const CScatterInfo *si;

                    if ( (config->siStorage.flags != NO_COMPONENTS) && (readout == SCATTER) ) {
                        // Do storage components
                        si = &config->siStorage;
                        siCurrent = -1;
                    } else {
                        // No direct light using storage components
                        si = &config->siOthers[0];
                        siCurrent = 0;
                    }

                    while ( siCurrent < config->siOthersCount ) {
                        bool doSi = true;

                        if ( ((config->reflectionSampling == PHOTON_MAP_SAMPLING)
                            || (config->reflectionSampling == CLASSICAL_SAMPLING))
                            && ( si->flags & BSDF_SPECULAR_COMPONENT ) ) {
                            // Perfect mirror reflection, no n.e.e.
                            doSi = false;
                        }

                        if ( doSi ) {
                            // Connect using correct flags
                            geom = pathNodeConnect(
                                camera,
                            prevNode,
                            &lightNode,
                            &config->samplerConfig,
                            nullptr, // No light config
                            CONNECT_EL,
                            si->flags,
                            BSDF_ALL_COMPONENTS,
                            &dirEL);

                            // Contribution of this sample (with Multiple Imp. S.)

                            if ( config->reflectionSampling == CLASSICAL_SAMPLING ) {
                                weight = 1.0;
                            } else {
                                // N direct * pdf  for the n.e.e.
                                cl = multipleImportanceSampling(config->nextEventSamples * lightNode.m_pdfFromPrev);

                                // N scatter * pdf  for possible scattering
                                if ( si->DoneSomePreviousBounce(prevNode) ) {
                                    nrs = si->nrSamplesAfter;
                                } else {
                                    nrs = si->nrSamplesBefore;
                                }

                                cr = multipleImportanceSampling(nrs * lightNode.m_pdfFromNext);

                                // Are we deep enough to do russian roulette
                                if ( lightNode.m_depth >= config->samplerConfig.minDepth ) {
                                    cr *= multipleImportanceSampling(lightNode.m_rrPdfFromNext);
                                }

                                weight = cl / (cl + cr);
                            }

                            factor = weight * geom / (lightNode.m_pdfFromPrev *
                                                      config->nextEventSamples);
                            radiance.scalarProductScaled(prevNode->m_bsdfEval, (float) factor, lightNode.m_bsdfEval);

                            // Collect outgoing radiance
                            result.add(result, radiance);
                        } // if not photon map or no caustic path

                        // Next scatter info block
                        siCurrent++;
                        if ( siCurrent < config->siOthersCount ) {
                            si = &config->siOthers[siCurrent];
                        }

                    }
                }
            }

            if ( config->lightMode == ALL_LIGHTS ) {
                lightsToDo = nes->ActivateNextUnit();
            } else {
                lightsToDo = false;
            }
        }
    }
    return result;
}

static ColorRgb
stochasticRaytracerGetRadiance(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *thisNode,
    StochasticRaytracingConfiguration *config,
    StorageReadout readout,
    int usedScatterSamples,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions)
{
    ColorRgb result;
    ColorRgb radiance;
    char edfFlags = ALL_COMPONENTS;

    // Handle background
    if ( thisNode->m_rayType == ENVIRONMENT ) {
        // Check for  weighting
        double weight = 1;
        double cr;
        double cl;
        bool doWeight = true;

        if ( thisNode->m_depth <= 1 ) {   // don't weight direct light
            doWeight = false;
        }

        if ( config->reflectionSampling == CLASSICAL_SAMPLING ) {
            doWeight = false;
        }

        if ( !(config->backgroundSampling) ) {
            doWeight = false;
        }

        if ( doWeight ) {
            cl = config->nextEventSamples *
                 config->samplerConfig.neSampler->evalPDF(camera, thisNode->previous(), thisNode);
            cl = multipleImportanceSampling(cl);
            cr = usedScatterSamples * thisNode->m_pdfFromPrev;
            cr = multipleImportanceSampling(cr);
            weight = cr / (cr + cl);
        }

        Vector3D position = thisNode->previous()->m_hit.getPoint();
        result = backgroundRadiance(sceneBackground, &position, &(thisNode->m_inDirF), nullptr);

        result.scale((float)weight);
    } else {
        // Handle non-background
        const PhongEmittanceDistributionFunction *thisEdf = thisNode->m_hit.getMaterial()->getEdf();

        result.clear();

        // Stored radiance
        if ( (readout == READ_NOW) && (config->siStorage.flags != NO_COMPONENTS) ) {
            // Add the stored radiance being emitted from the patch
            if ( radianceMethod->className == PHOTON_MAP ) {
                if ( config->radMode == STORED_PHOTON_MAP ) {
                    // Check if the distance to the previous point is big enough
                    // otherwise we need more scattering...
                    float dist2 = thisNode->m_hit.getPoint().distance2(thisNode->previous()->m_hit.getPoint());

                    if ( dist2 > PHOTON_MAP_MIN_DIST2 ) {
                        radiance = photonMapGetNodeGRadiance(thisNode);
                        // This does not include Le (self emitted light)
                    } else {
                        radiance.clear();
                        readout = SCATTER; // This ensures extra scattering, direct light and c-map
                    }
                } else {
                    radiance = photonMapGetNodeGRadiance(thisNode);
                    // This does not include Le (self emitted light)
                }
            } else {
                // Other radiosity method
                double u;
                double v;

                // (u, v) coordinates of intersection point
                Vector3D position = thisNode->m_hit.getPoint();
                thisNode->m_hit.getPatch()->uv(&position, &u, &v);

                radiance = radianceMethod->getRadiance(
                    camera, thisNode->m_hit.getPatch(), u, v, thisNode->m_inDirF, renderOptions);

                // This includes Le diffuse, subtraction first and handle total emitted later (possibly weighted)
                // -- Interface mechanism needed to determine what a
                // -- radiance method does...
                ColorRgb diffEmit;

                if ( thisEdf == nullptr ) {
                    diffEmit.clear();
                } else {
                    diffEmit = thisEdf->phongEdfEval(
                        &thisNode->m_hit, &(thisNode->m_inDirF), BRDF_DIFFUSE_COMPONENT, nullptr);
                }

                radiance.subtract(radiance, diffEmit);
            }

            result.add(result, radiance);

        } // Done: Stored radiance, no self emitted light included!

        // Stored caustic maps
        if ( (config->radMode == STORED_PHOTON_MAP) && readout == SCATTER ) {
            radiance = photonMapGetNodeCRadiance(thisNode);
            result.add(result, radiance);
        }

        radiance = srGetDirectRadiance(camera, sceneVoxelGrid, sceneBackground, thisNode, config, readout);
        result.add(result, radiance);

        // Scattered light
        radiance = stochasticRaytracerGetScatteredRadiance(
                camera,
                sceneVoxelGrid,
                sceneBackground,
                thisNode,
                config,
                readout,
                radianceMethod,
                renderOptions);
        result.add(result, radiance);

        // Emitted Light
        if ( config->radMode == STORED_PHOTON_MAP
            && radianceMethod->className == PHOTON_MAP
            && (readout == READ_NOW)
            && !(config->siStorage.DoneThisBounce(thisNode->previous())) ) {
            // Check if Le would contribute to a caustic
            // Caustic contribution: (E...(D|G)...?L) with ? some specular bounce
            edfFlags = 0;
        }

        if ( (thisEdf != nullptr) && (edfFlags != 0) ) {
            double weight;
            double cr;
            double cl;
            ColorRgb col;
            bool doWeight = true;

            if ( thisNode->m_depth <= 1 ) {
                doWeight = false;
            }

            if ( config->reflectionSampling == CLASSICAL_SAMPLING ) {
                doWeight = false;
            }

            if ( config->reflectionSampling == PHOTON_MAP_SAMPLING
              && thisNode->m_depth > 1
              && ( thisNode->previous()->m_usedComponents & BSDF_SPECULAR_COMPONENT) ) {
                // Perfect Specular scatter, no weighting
                doWeight = false;
            }

            if ( doWeight ) {
                cl = config->nextEventSamples *
                     config->samplerConfig.neSampler->evalPDF(camera, thisNode->previous(), thisNode);
                cl = multipleImportanceSampling(cl);
                cr = usedScatterSamples * thisNode->m_pdfFromPrev;
                cr = multipleImportanceSampling(cr);

                weight = cr / (cr + cl);
            } else {
                // We don't do N.E.E. from the eye !
                weight = 1;
            }

            if ( thisEdf == nullptr ) {
                col.clear();
            } else {
                col = thisEdf->phongEdfEval(&thisNode->m_hit, &(thisNode->m_inDirF), edfFlags, nullptr);
            }

            result.addScaled(result, (float) weight, col);
        }
    }

    return result;
}

ColorRgb
StochasticRaytracer::calcPixel(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    int nx,
    int ny,
    StochasticRaytracingConfiguration *config,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions)
{
    SimpleRaytracingPathNode eyeNode;
    SimpleRaytracingPathNode pixelNode;
    double x1;
    double x2;
    ColorRgb col;
    ColorRgb result;
    StratifiedSampling2D stratified(config->samplesPerPixel);

    result.clear();

    // Frame coherent & correlated sampling
    if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
        if ( GLOBAL_raytracing_state.doCorrelatedSampling ) {
            // Correlated : start each pixel with same seed
            srand48(GLOBAL_raytracing_state.baseSeed);
        }
        drand48(); // (randomize seed, gives new seed for uncorrelated sampling)
        config->seedConfig.save(0);
    }

    // Calc pixel data

    // Sample eye node
    config->samplerConfig.pointSampler->sample(camera, sceneVoxelGrid, sceneBackground, nullptr, nullptr, &eyeNode, 0, 0);
    ((CPixelSampler *) config->samplerConfig.dirSampler)->SetPixel(camera, nx, ny, nullptr);

    eyeNode.attach(&pixelNode);

    // Stratified sampling of the pixel
    for ( int i = 0; i < config->samplesPerPixel; i++ ) {
        stratified.sample(&x1, &x2);

        if ( config->samplerConfig.dirSampler->sample(camera, sceneVoxelGrid, sceneBackground, nullptr, &eyeNode, &pixelNode, x1, x2)
             && ((pixelNode.m_rayType != ENVIRONMENT) || (config->backgroundDirect)) ) {
            pixelNode.assignBsdfAndNormal();

            // Frame coherent & correlated sampling
            if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
                config->seedConfig.save(pixelNode.m_depth);
            }

            col = stochasticRaytracerGetRadiance(
                    camera,
                    sceneVoxelGrid,
                    sceneBackground,
                    &pixelNode,
                    config,
                    config->initialReadout,
                    config->samplesPerPixel,
                    radianceMethod,
                    renderOptions);

            // Frame coherent & correlated sampling
            if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
                config->seedConfig.Restore(pixelNode.m_depth);
            }

            // col represents the radiance reflected towards the eye
            // in the pixel sampled point.

            // Account for the eye sampling
            // -- Not needed yet ...

            // Account for pixel sampling
            col.scale((float) (pixelNode.m_G / pixelNode.m_pdfFromPrev));
            result.add(result, col);
        }
    }

    // We have now the FLUX for the pixel (x N), convert it to radiance
    double factor = (computeFluxToRadFactor(camera, nx, ny) / (float)config->samplesPerPixel);

    result.scale((float)factor);
    config->screen->add(nx, ny, result);

    // Frame coherent & correlated sampling
    if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
        config->seedConfig.Restore(0);
    }

    return result;
}

int
RTStochastic_Redisplay() {
    if ( GLOBAL_raytracing_state.lastScreen ) {
        GLOBAL_raytracing_state.lastScreen->render();
        return true;
    } else {
        return false;
    }
}

int
RTStochastic_SaveImage(ImageOutputHandle *ip) {
    if ( ip && GLOBAL_raytracing_state.lastScreen ) {
        GLOBAL_raytracing_state.lastScreen->sync();
        GLOBAL_raytracing_state.lastScreen->writeFile(ip);
        return true;
    } else {
        return false;
    }
}

void
stochasticRayTracerTerminate() {
    if ( GLOBAL_raytracing_state.lastScreen ) {
        delete GLOBAL_raytracing_state.lastScreen;
    }
    GLOBAL_raytracing_state.lastScreen = nullptr;
    if ( GLOBAL_lightList != nullptr ) {
        delete GLOBAL_lightList;
        GLOBAL_lightList = nullptr;
    }
}

Raytracer GLOBAL_raytracing_stochasticMethod =
{
    "StochasticRaytracing",
    4,
    RTStochastic_Redisplay,
    RTStochastic_SaveImage,
    stochasticRayTracerTerminate
};

#endif
