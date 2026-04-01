#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED
#include "common/RenderOptions.h"
#include "common/Error.h"
#include "common/StratifiedSampling2D.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "photonMap/PhotonMapRadianceMethod.h"
#include "raycasting/common/Raytools.h"
#include "raycasting/raytracing/ScreenIterate.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracerCallbackData.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracer.h"

char StochasticRaytracer::name[38] = "Stochastic Raytracing & Final Gathers";

// Heuristic minimum distance threshold for photon map readouts
// should be tuned and dependent on scene size, ...
const float PHOTON_MAP_MIN_DIST = 0.02f;
const float PHOTON_MAP_MIN_DIST2 = PHOTON_MAP_MIN_DIST * PHOTON_MAP_MIN_DIST; // squared

StochasticRaytracer::StochasticRaytracer(
    LightList *&inLightList,
    StochasticRayTracingState &inRayTracingState):
    lightList(inLightList),
    rayTracingState(inRayTracingState)
{
}

StochasticRaytracer::~StochasticRaytracer() {
}

void
StochasticRaytracer::defaults() {
    // Defaults are owned by the caller-provided StochasticRayTracingState instance.
}

const char *
StochasticRaytracer::getName() const {
    return name;
}

void
StochasticRaytracer::initialize(const java::ArrayList<Patch *> *lightPatches) const {
    (void) lightPatches;
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
    ToneMappingContext *toneMapOptions = renderOptions == nullptr ? nullptr : renderOptions->toneMapOptions;
    if ( toneMapOptions == nullptr ) {
        Error::fatal(-1, "StochasticRaytracer::execute", "Tone mapping context not set in render options");
    }

    StochasticRaytracingConfiguration config(
        scene->camera,
        rayTracingState,
        scene->lightSourcePatchList,
        radianceMethod,
        toneMapOptions,
        lightList); // config filled in by constructor
    StochasticRaytracerCallbackData callbackData = {
        &config,
        radianceMethod,
        const_cast<RenderOptions *>(renderOptions)
    };

    // Frame Coherent sampling : init fixed seed
    if ( rayTracingState.doFrameCoherent ) {
        srand48(rayTracingState.baseSeed);
    }

    if ( !rayTracingState.progressiveTracing ) {
        ScreenIterate::sequential(
                scene->camera,
                scene->voxelGrid,
                scene->background,
                StochasticRaytracer::calcPixel,
                &callbackData,
                *toneMapOptions);
    } else {
        ScreenIterate::progressive(
                scene->camera,
                scene->voxelGrid,
                scene->background,
                StochasticRaytracer::calcPixel,
                &callbackData,
                *toneMapOptions);
    }

    config.screen->render();

    if ( ip ) {
        config.screen->writeFile(ip);
    }

    if ( rayTracingState.lastScreen ) {
        delete rayTracingState.lastScreen;
    }
    rayTracingState.lastScreen = config.screen;
    config.screen = nullptr;
}

bool
StochasticRaytracer::saveImage(ImageOutputHandle *imageOutputHandle) const {
    if ( imageOutputHandle && rayTracingState.lastScreen ) {
        rayTracingState.lastScreen->sync();
        rayTracingState.lastScreen->writeFile(imageOutputHandle);
        return true;
    } else {
        return false;
    }
}

void
StochasticRaytracer::terminate() const {
    if ( rayTracingState.lastScreen ) {
        delete rayTracingState.lastScreen;
    }
    rayTracingState.lastScreen = nullptr;
    if ( lightList != nullptr ) {
        delete lightList;
        lightList = nullptr;
    }
}

 ColorRgb
StochasticRaytracer::stochasticRaytracerGetScatteredRadiance(
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
        (readout == StorageReadout::SCATTER) ) {
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
                     && ((newNode.m_rayType != PathRayType::ENVIRONMENT) || (config->backgroundIndirect)) ) {
                    if ( newNode.m_rayType != PathRayType::ENVIRONMENT ) {
                        newNode.assignBsdfAndNormal();
                    }

                    // Frame coherent & correlated sampling
                    if ( config->doFrameCoherent || config->doCorrelatedSampling ) {
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
                                StorageReadout::READ_NOW,
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
                    if ( config->doFrameCoherent || config->doCorrelatedSampling ) {
                        config->seedConfig.Restore(newNode.m_depth);
                    }

                    // Collect outgoing radiance
                    factor = newNode.m_G / (newNode.m_pdfFromPrev * numberOfSamples);

                    radiance.scalarProductScaled(radiance, static_cast<float>(factor), thisNode->m_bsdfEval);
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

 ColorRgb
StochasticRaytracer::srGetDirectRadiance(
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

    if ( readout == StorageReadout::READ_NOW && config->radMode == RayTracingRadMode::STORED_PHOTON_MAP ) {
        // We're reading out D|G, specular not with direct light
        return result;
    }

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

        if ( config->lightMode == RayTracingLightMode::ALL_LIGHTS ) {
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
                    && ( RayTools::pathNodesVisible(sceneVoxelGrid, prevNode, &lightNode) ) ) {
                    // Now connect for all applicable scatter-info's
                    // If no weighting between reflection sampling and
                    // next event estimation were used, only one connect
                    // using the union of different scatter info flags
                    // are necessary (=speedup)
                    int siCurrent;
                    const CScatterInfo *si;

                    if ( (config->siStorage.flags != NO_COMPONENTS) && (readout == StorageReadout::SCATTER) ) {
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

                        if ( ((config->reflectionSampling == RayTracingSamplingMode::PHOTON_MAP_SAMPLING)
                            || (config->reflectionSampling == RayTracingSamplingMode::CLASSICAL_SAMPLING))
                            && ( si->flags & BSDF_SPECULAR_COMPONENT ) ) {
                            // Perfect mirror reflection, no n.e.e.
                            doSi = false;
                        }

                        if ( doSi ) {
                            // Connect using correct flags
                            geom = SamplerConfig::pathNodeConnect(
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

                            if ( config->reflectionSampling == RayTracingSamplingMode::CLASSICAL_SAMPLING ) {
                                weight = 1.0;
                            } else {
                                // N direct * pdf  for the n.e.e.
                                cl = SimpleRaytracingPathNode::multipleImportanceSampling(config->nextEventSamples * lightNode.m_pdfFromPrev);

                                // N scatter * pdf  for possible scattering
                                if ( si->DoneSomePreviousBounce(prevNode) ) {
                                    nrs = si->nrSamplesAfter;
                                } else {
                                    nrs = si->nrSamplesBefore;
                                }

                                cr = SimpleRaytracingPathNode::multipleImportanceSampling(nrs * lightNode.m_pdfFromNext);

                                // Are we deep enough to do russian roulette
                                if ( lightNode.m_depth >= config->samplerConfig.minDepth ) {
                                    cr *= SimpleRaytracingPathNode::multipleImportanceSampling(lightNode.m_rrPdfFromNext);
                                }

                                weight = cl / (cl + cr);
                            }

                            factor = weight * geom / (lightNode.m_pdfFromPrev *
                                                      config->nextEventSamples);
                            radiance.scalarProductScaled(prevNode->m_bsdfEval, static_cast<float>(factor), lightNode.m_bsdfEval);

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

            if ( config->lightMode == RayTracingLightMode::ALL_LIGHTS ) {
                lightsToDo = nes->ActivateNextUnit();
            } else {
                lightsToDo = false;
            }
        }
    }
    return result;
}

 ColorRgb
StochasticRaytracer::stochasticRaytracerGetRadiance(
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
    if ( thisNode->m_rayType == PathRayType::ENVIRONMENT ) {
        // Check for  weighting
        double weight = 1;
        double cr;
        double cl;
        bool doWeight = true;

        if ( thisNode->m_depth <= 1 ) {   // don't weight direct light
            doWeight = false;
        }

        if ( config->reflectionSampling == RayTracingSamplingMode::CLASSICAL_SAMPLING ) {
            doWeight = false;
        }

        if ( !(config->backgroundSampling) ) {
            doWeight = false;
        }

        if ( doWeight ) {
            cl = config->nextEventSamples *
                 config->samplerConfig.neSampler->evalPDF(camera, thisNode->previous(), thisNode);
            cl = SimpleRaytracingPathNode::multipleImportanceSampling(cl);
            cr = usedScatterSamples * thisNode->m_pdfFromPrev;
            cr = SimpleRaytracingPathNode::multipleImportanceSampling(cr);
            weight = cr / (cr + cl);
        }

        Vector3D position = thisNode->previous()->m_hit.getPoint();
        result = Background::backgroundRadiance(sceneBackground, &position, &(thisNode->m_inDirF), nullptr);

        result.scale(static_cast<float>(weight));
    } else {
        // Handle non-background
        const PhongEmittanceDistributionFunction *thisEdf = thisNode->m_hit.getMaterial()->getEdf();

        result.clear();

        // Stored radiance
        if ( (readout == StorageReadout::READ_NOW) && (config->siStorage.flags != NO_COMPONENTS) ) {
            // Add the stored radiance being emitted from the patch
            if ( radianceMethod->className == PHOTON_MAP ) {
                const PhotonMapRadianceMethod *photonMapMethod = static_cast<const PhotonMapRadianceMethod *>(radianceMethod);
                if ( config->radMode == RayTracingRadMode::STORED_PHOTON_MAP ) {
                    // Check if the distance to the previous point is big enough
                    // otherwise we need more scattering...
                    float dist2 = thisNode->m_hit.getPoint().distance2(thisNode->previous()->m_hit.getPoint());

                    if ( dist2 > PHOTON_MAP_MIN_DIST2 ) {
                        radiance = photonMapMethod->getNodeGRadiance(thisNode);
                        // This does not include Le (self emitted light)
                    } else {
                        radiance.clear();
                        readout = StorageReadout::SCATTER; // This ensures extra scattering, direct light and c-map
                    }
                } else {
                    radiance = photonMapMethod->getNodeGRadiance(thisNode);
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
        if ( (config->radMode == RayTracingRadMode::STORED_PHOTON_MAP) && readout == StorageReadout::SCATTER ) {
            const PhotonMapRadianceMethod *photonMapMethod = static_cast<const PhotonMapRadianceMethod *>(radianceMethod);
            radiance = photonMapMethod->getNodeCRadiance(thisNode);
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
        if ( config->radMode == RayTracingRadMode::STORED_PHOTON_MAP
            && radianceMethod->className == PHOTON_MAP
            && (readout == StorageReadout::READ_NOW)
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

            if ( config->reflectionSampling == RayTracingSamplingMode::CLASSICAL_SAMPLING ) {
                doWeight = false;
            }

            if ( config->reflectionSampling == RayTracingSamplingMode::PHOTON_MAP_SAMPLING
              && thisNode->m_depth > 1
              && ( thisNode->previous()->m_usedComponents & BSDF_SPECULAR_COMPONENT) ) {
                // Perfect Specular scatter, no weighting
                doWeight = false;
            }

            if ( doWeight ) {
                cl = config->nextEventSamples *
                     config->samplerConfig.neSampler->evalPDF(camera, thisNode->previous(), thisNode);
                cl = SimpleRaytracingPathNode::multipleImportanceSampling(cl);
                cr = usedScatterSamples * thisNode->m_pdfFromPrev;
                cr = SimpleRaytracingPathNode::multipleImportanceSampling(cr);

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

            result.addScaled(result, static_cast<float>(weight), col);
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
    void *data)
{
    auto *callbackData = static_cast<StochasticRaytracerCallbackData *>(data);
    StochasticRaytracingConfiguration *config = callbackData->config;
    RadianceMethod *radianceMethod = callbackData->radianceMethod;
    RenderOptions *renderOptions = callbackData->renderOptions;
    SimpleRaytracingPathNode eyeNode;
    SimpleRaytracingPathNode pixelNode;
    double x1;
    double x2;
    ColorRgb col;
    ColorRgb result;
    StratifiedSampling2D stratified(config->samplesPerPixel);

    result.clear();

    // Frame coherent & correlated sampling
    if ( config->doFrameCoherent || config->doCorrelatedSampling ) {
        if ( config->doCorrelatedSampling ) {
            // Correlated : start each pixel with same seed
            srand48(config->baseSeed);
        }
        drand48(); // (randomize seed, gives new seed for uncorrelated sampling)
        config->seedConfig.save(0);
    }

    // Calc pixel data

    // Sample eye node
    config->samplerConfig.pointSampler->sample(camera, sceneVoxelGrid, sceneBackground, nullptr, nullptr, &eyeNode, 0, 0);
    static_cast<PixelSampler *>(config->samplerConfig.dirSampler)->SetPixel(camera, nx, ny, nullptr);

    eyeNode.attach(&pixelNode);

    // Stratified sampling of the pixel
    for ( int i = 0; i < config->samplesPerPixel; i++ ) {
        stratified.sample(&x1, &x2);

        if ( config->samplerConfig.dirSampler->sample(camera, sceneVoxelGrid, sceneBackground, nullptr, &eyeNode, &pixelNode, x1, x2)
             && ((pixelNode.m_rayType != PathRayType::ENVIRONMENT) || (config->backgroundDirect)) ) {
            pixelNode.assignBsdfAndNormal();

            // Frame coherent & correlated sampling
            if ( config->doFrameCoherent || config->doCorrelatedSampling ) {
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
            if ( config->doFrameCoherent || config->doCorrelatedSampling ) {
                config->seedConfig.Restore(pixelNode.m_depth);
            }

            // col represents the radiance reflected towards the eye
            // in the pixel sampled point.

            // Account for the eye sampling
            // -- Not needed yet ...

            // Account for pixel sampling
            col.scale(static_cast<float>(pixelNode.m_G / pixelNode.m_pdfFromPrev));
            result.add(result, col);
        }
    }

    // We have now the FLUX for the pixel (x N), convert it to radiance
    double factor = (ScreenBuffer::computeFluxToRadFactor(camera, nx, ny) / static_cast<float>(config->samplesPerPixel));

    result.scale(static_cast<float>(factor));
    config->screen->add(nx, ny, result);

    // Frame coherent & correlated sampling
    if ( config->doFrameCoherent || config->doCorrelatedSampling ) {
        config->seedConfig.Restore(0);
    }

    return result;
}

#endif
