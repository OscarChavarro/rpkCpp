#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "material/RendererConfiguration.h"
#include "common/logging/Logger.h"
#include "raycasting/common/StratifiedSampling2D.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/photonMap/PhotonMapRadianceMethod.h"
#include "raycasting/common/Raytools.h"
#include "raycasting/raytracing/ScreenIterate.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracerCallbackData.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracer.h"

char StochasticRaytracer::name[38] = "Stochastic Raytracing & Final Gathers";

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
StochasticRaytracer::initialize(const ArrayList<Patch *> *lightPatches) const {
    (void) lightPatches;
}

/**
Raytrace the current scene as seen with the current camera. If fp
is not a NULL pointer, write the ray-traced image to the file
pointed to by 'fp'
*/
void
StochasticRaytracer::execute(
    ImageOutputHandle *ip,
    Scene *scene,
    RadianceMethod *radianceMethod,
    ToneMappingContext *toneMapOptions,
    const RenderOptions *renderOptions) const
{
    if ( toneMapOptions == NULL ) {
        Logger::fatal(-1, "StochasticRaytracer::execute", "Tone mapping context not provided");
    }

    StochRaytrConfig config(
        scene->camera,
        rayTracingState,
        scene->lightSourcePatchList,
        radianceMethod,
        toneMapOptions,
        lightList); // config filled in by constructor
    StochasticRaytracerCallbackData callbackData;
    callbackData.config = &config;
    callbackData.radianceMethod = radianceMethod;
    callbackData.renderOptions = ((RenderOptions *)(renderOptions));

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
    config.screen = NULL;
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
    rayTracingState.lastScreen = NULL;
    if ( lightList != NULL ) {
        delete lightList;
        lightList = NULL;
    }
}

 ColorRgb
StochasticRaytracer::stchsRaytrcGetScttrRadn(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background * sceneBackground,
    SimpleRaytracingPathNode *thisNode,
    StochRaytrConfig *config,
    StorageReadout readout,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions)
{
    int siCurrent; // What scatter block are we handling
    const ScatterInfo *si;

    SimpleRaytracingPathNode newNode;
    thisNode->attach(&newNode);

    ColorRgb result(0.0f, 0.0f, 0.0f);

    if ( (config->samplerConfig.surfaceSampler == NULL) ||
        (thisNode->m_depth >= config->samplerConfig.maxDepth) ) {
        // No scattering
        return result;
    }

    if ( (config->siStorage.flags != XxdfComponentFlagInfo::NO_COMPONENTS) &&
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
            ColorRgb albedo(0.0f, 0.0f, 0.0f);
            if ( thisNode->m_useBsdf != NULL ) {
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
                    if ( config->doFrameCoherent || config->doCorrelatedSampling ) {
                        config->seedConfig.Restore(newNode.m_depth);
                    }

                    // Collect outgoing radiance
                    factor = newNode.m_G / (newNode.m_pdfFromPrev * numberOfSamples);

                    radiance = ColorRgb(
                        radiance.getR() * ((float)(factor)) * thisNode->m_bsdfEval.getR(),
                        radiance.getG() * ((float)(factor)) * thisNode->m_bsdfEval.getG(),
                        radiance.getB() * ((float)(factor)) * thisNode->m_bsdfEval.getB());
                    result = ColorRgb(
                        result.getR() + radiance.getR(),
                        result.getG() + radiance.getG(),
                        result.getB() + radiance.getB());
                }
            }
        }

        // Next scatter info block
        siCurrent++;
        if ( siCurrent < config->siOthersCount ) {
            si = &config->siOthers[siCurrent];
        }
    }

    thisNode->setNext(NULL);
    return result;
}

 ColorRgb
StochasticRaytracer::srGetDirectRadiance(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *prevNode,
    StochRaytrConfig *config,
    StorageReadout readout)
{
    ColorRgb result;
    ColorRgb radiance;
    result = ColorRgb(0.0f, 0.0f, 0.0f);
    Vector3D dirEL;

    if ( readout == READ_NOW && config->radMode == STORED_PHOTON_MAP ) {
        // We're reading out D|G, specular not with direct light
        return result;
    }

    NextEventSampler *nes = config->samplerConfig.neSampler;

    // Check if N.E.E. can give a contribution. I.e. not inside
    // a medium or just about to leave to vacuum
    if ( (nes != NULL) &&
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
                    BsdfComponentInfo::BSDF_ALL_COMPONENTS)
                    && ( RayTools::pathNodesVisible(sceneVoxelGrid, prevNode, &lightNode) ) ) {
                    // Now connect for all applicable scatter-info's
                    // If no weighting between reflection sampling and
                    // next event estimation were used, only one connect
                    // using the union of different scatter info flags
                    // are necessary (=speedup)
                    int siCurrent;
                    const ScatterInfo *si;

                    if ( (config->siStorage.flags != XxdfComponentFlagInfo::NO_COMPONENTS) && (readout == SCATTER) ) {
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
                            && ( si->flags & BsdfComponentInfo::BSDF_SPECULAR_COMPONENT ) ) {
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
                            NULL, // No light config
                            CONNECT_EL,
                            si->flags,
                            BsdfComponentInfo::BSDF_ALL_COMPONENTS,
                            &dirEL);

                            // Contribution of this sample (with Multiple Imp. S.)

                            if ( config->reflectionSampling == CLASSICAL_SAMPLING ) {
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
                            radiance = ColorRgb(
                                prevNode->m_bsdfEval.getR() * ((float)(factor)) * lightNode.m_bsdfEval.getR(),
                                prevNode->m_bsdfEval.getG() * ((float)(factor)) * lightNode.m_bsdfEval.getG(),
                                prevNode->m_bsdfEval.getB() * ((float)(factor)) * lightNode.m_bsdfEval.getB());

                            // Collect outgoing radiance
                            result = ColorRgb(
                                result.getR() + radiance.getR(),
                                result.getG() + radiance.getG(),
                                result.getB() + radiance.getB());
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

 ColorRgb
StochasticRaytracer::stochasticRaytracerGetRadiance(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *thisNode,
    StochRaytrConfig *config,
    StorageReadout readout,
    int usedScatterSamples,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions)
{
    ColorRgb result;
    ColorRgb radiance;
    char edfFlags = XxdfComponentFlagInfo::ALL_COMPONENTS;

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
            cl = SimpleRaytracingPathNode::multipleImportanceSampling(cl);
            cr = usedScatterSamples * thisNode->m_pdfFromPrev;
            cr = SimpleRaytracingPathNode::multipleImportanceSampling(cr);
            weight = cr / (cr + cl);
        }

        Vector3D position = thisNode->previous()->m_hit.getPoint();
        result = ColorRgb(Background::backgroundRadiance(sceneBackground, &position, &(thisNode->m_inDirF), NULL));
        result = ColorRgb(
            result.getR() * ((float)(weight)),
            result.getG() * ((float)(weight)),
            result.getB() * ((float)(weight)));
    } else {
        // Handle non-background
        const PhongEmitDistFunc *thisEdf = thisNode->m_hit.getMaterial()->getEdf();

        result = ColorRgb(0.0f, 0.0f, 0.0f);

        // Stored radiance
        if ( (readout == READ_NOW) && (config->siStorage.flags != XxdfComponentFlagInfo::NO_COMPONENTS) ) {
            // Add the stored radiance being emitted from the patch
            if ( radianceMethod->className == PHOTON_MAP ) {
                const PhotonMapRadianceMethod *photonMapMethod = ((const PhotonMapRadianceMethod *)(radianceMethod));
                if ( config->radMode == STORED_PHOTON_MAP ) {
                    // Check if the distance to the previous point is big enough
                    // otherwise we need more scattering...
                    float dist2 = thisNode->m_hit.getPoint().distance2(thisNode->previous()->m_hit.getPoint());

                    if ( dist2 > PHOTON_MAP_MIN_DIST2 ) {
                        radiance = photonMapMethod->getNodeGRadiance(thisNode);
                        // This does not include Le (self emitted light)
                    } else {
                        radiance = ColorRgb(0.0f, 0.0f, 0.0f);
                        readout = SCATTER; // This ensures extra scattering, direct light and c-map
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

                radiance = ColorRgb(radianceMethod->getRadiance(
                    camera, thisNode->m_hit.getPatch(), u, v, thisNode->m_inDirF, renderOptions));

                // This includes Le diffuse, subtraction first and handle total emitted later (possibly weighted)
                // -- Interface mechanism needed to determine what a
                // -- radiance method does...
                ColorRgb diffEmit;

                if ( thisEdf == NULL ) {
                    diffEmit = ColorRgb(0.0f, 0.0f, 0.0f);
                } else {
                    diffEmit = thisEdf->phongEdfEval(
                        &thisNode->m_hit, &(thisNode->m_inDirF), BRDF_DIFFUSE_COMPONENT, NULL);
                }

                radiance = ColorRgb(
                    radiance.getR() - diffEmit.getR(),
                    radiance.getG() - diffEmit.getG(),
                    radiance.getB() - diffEmit.getB());
            }

            result = ColorRgb(
                result.getR() + radiance.getR(),
                result.getG() + radiance.getG(),
                result.getB() + radiance.getB());

        } // Done: Stored radiance, no self emitted light included!

        // Stored caustic maps
        if ( (config->radMode == STORED_PHOTON_MAP) && readout == SCATTER ) {
            const PhotonMapRadianceMethod *photonMapMethod = ((const PhotonMapRadianceMethod *)(radianceMethod));
            radiance = photonMapMethod->getNodeCRadiance(thisNode);
            result = ColorRgb(
                result.getR() + radiance.getR(),
                result.getG() + radiance.getG(),
                result.getB() + radiance.getB());
        }

        radiance = srGetDirectRadiance(camera, sceneVoxelGrid, sceneBackground, thisNode, config, readout);
        result = ColorRgb(
            result.getR() + radiance.getR(),
            result.getG() + radiance.getG(),
            result.getB() + radiance.getB());

        // Scattered light
        radiance = stchsRaytrcGetScttrRadn(
                camera,
                sceneVoxelGrid,
                sceneBackground,
                thisNode,
                config,
                readout,
                radianceMethod,
                renderOptions);
        result = ColorRgb(
            result.getR() + radiance.getR(),
            result.getG() + radiance.getG(),
            result.getB() + radiance.getB());

        // Emitted Light
        if ( config->radMode == STORED_PHOTON_MAP
            && radianceMethod->className == PHOTON_MAP
            && (readout == READ_NOW)
            && !(config->siStorage.DoneThisBounce(thisNode->previous())) ) {
            // Check if Le would contribute to a caustic
            // Caustic contribution: (E...(D|G)...?L) with ? some specular bounce
            edfFlags = 0;
        }

        if ( (thisEdf != NULL) && (edfFlags != 0) ) {
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
              && ( thisNode->previous()->m_usedComponents & BsdfComponentInfo::BSDF_SPECULAR_COMPONENT) ) {
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

            if ( thisEdf == NULL ) {
                col = ColorRgb(0.0f, 0.0f, 0.0f);
            } else {
                col = thisEdf->phongEdfEval(&thisNode->m_hit, &(thisNode->m_inDirF), edfFlags, NULL);
            }

            result = ColorRgb(
                result.getR() + ((float)(weight)) * col.getR(),
                result.getG() + ((float)(weight)) * col.getG(),
                result.getB() + ((float)(weight)) * col.getB());
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
    StochasticRaytracerCallbackData *callbackData = ((StochasticRaytracerCallbackData *)(data));
    StochRaytrConfig *config = callbackData->config;
    RadianceMethod *radianceMethod = callbackData->radianceMethod;
    RenderOptions *renderOptions = callbackData->renderOptions;
    SimpleRaytracingPathNode eyeNode;
    SimpleRaytracingPathNode pixelNode;
    double x1;
    double x2;
    ColorRgb col;
    ColorRgb result;
    StratifiedSampling2D stratified(config->samplesPerPixel);

    result = ColorRgb(0.0f, 0.0f, 0.0f);

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
    config->samplerConfig.pointSampler->sample(camera, sceneVoxelGrid, sceneBackground, NULL, NULL, &eyeNode, 0, 0);
    ((PixelSampler *)(config->samplerConfig.dirSampler))->SetPixel(camera, nx, ny, NULL);

    eyeNode.attach(&pixelNode);

    // Stratified sampling of the pixel
    for ( int i = 0; i < config->samplesPerPixel; i++ ) {
        stratified.sample(&x1, &x2);

        if ( config->samplerConfig.dirSampler->sample(camera, sceneVoxelGrid, sceneBackground, NULL, &eyeNode, &pixelNode, x1, x2)
             && ((pixelNode.m_rayType != ENVIRONMENT) || (config->backgroundDirect)) ) {
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
            col = ColorRgb(
                col.getR() * ((float)(pixelNode.m_G / pixelNode.m_pdfFromPrev)),
                col.getG() * ((float)(pixelNode.m_G / pixelNode.m_pdfFromPrev)),
                col.getB() * ((float)(pixelNode.m_G / pixelNode.m_pdfFromPrev)));
            result = ColorRgb(
                result.getR() + col.getR(),
                result.getG() + col.getG(),
                result.getB() + col.getB());
        }
    }

    // We have now the FLUX for the pixel (x N), convert it to radiance
    double factor = (ScreenBuffer::computeFluxToRadFactor(camera, nx, ny) / ((float)(config->samplesPerPixel)));

    result = ColorRgb(
        result.getR() * ((float)(factor)),
        result.getG() * ((float)(factor)),
        result.getB() * ((float)(factor)));
    config->screen->add(nx, ny, result);

    // Frame coherent & correlated sampling
    if ( config->doFrameCoherent || config->doCorrelatedSampling ) {
        config->seedConfig.Restore(0);
    }

    return result;
}

#endif
