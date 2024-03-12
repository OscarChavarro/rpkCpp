#include "scene/scene.h"
#include "common/stratification.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "PHOTONMAP/PhotonMapRadianceMethod.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/screeniterate.h"
#include "raycasting/stochasticRaytracing/rtstochasticphotonmap.h"

// Heuristic minimum distance threshold for photon map readouts
// should be tuned and dependent on scene size, ...
const float PHOTON_MAP_MIN_DIST = 0.02;
const float PHOTON_MAP_MIN_DIST2 = PHOTON_MAP_MIN_DIST * PHOTON_MAP_MIN_DIST; // squared

RayTracingStochasticState GLOBAL_raytracing_state;

/******* get radiance routines for stochastic raytracing *******/

// Forward declaration
COLOR
SR_GetRadiance(
        SimpleRaytracingPathNode *thisNode,
        StochasticRaytracingConfiguration *config,
        StorageReadout readout,
        int usedScatterSamples);

COLOR
SR_GetScatteredRadiance(
        SimpleRaytracingPathNode *thisNode,
        StochasticRaytracingConfiguration *config,
        StorageReadout readout)
{
    int siCurrent; // What scatter block are we handling
    CScatterInfo *si;

    SimpleRaytracingPathNode newNode;
    thisNode->attach(&newNode);

    COLOR result;
    colorClear(result);

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
        int nrSamples;

        if ( si->DoneSomePreviousBounce(thisNode) ) {
            nrSamples = si->nrSamplesAfter;
        } else {
            nrSamples = si->nrSamplesBefore;
        } // First bounce of this kind

        // A small optimisation to prevent sampling surface that
        // don't have this scattering component.

        if ( nrSamples > 2 ) {
            // Some bigger value may be more efficient
            COLOR albedo = bsdfScatteredPower(
                thisNode->m_useBsdf,
                  &thisNode->m_hit,
                  &thisNode->m_normal,
                  si->flags);
            if ( colorAverage(albedo) < EPSILON ) {
                // Skip, no contribution anyway
                nrSamples = 0;
            }
        }

        // Do we need to compute scattered radiance at all...
        if ( (nrSamples > 0) && (thisNode->m_depth + 1 < config->samplerConfig.maxDepth) ) {
            int i;
            double x_1;
            double x_2;
            double factor;
            StratifiedSampling2D stratified(nrSamples);
            COLOR radiance;
            bool doRR = thisNode->m_depth >= config->samplerConfig.minDepth;

            for ( i = 0; i < nrSamples; i++ ) {
                stratified.sample(&x_1, &x_2);

                // Surface sampling
                if ( config->samplerConfig.surfaceSampler->sample(
                        thisNode->previous(),
                        thisNode,
                        &newNode, x_1, x_2,
                        doRR,
                        si->flags)
                     && ((newNode.m_rayType != ENVIRONMENT) || (config->backgroundIndirect)) ) {
                    if ( newNode.m_rayType != ENVIRONMENT ) {
                        newNode.assignBsdfAndNormal();
                    }

                    // Frame coherent & correlated sampling
                    if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
                        config->seedConfig.Save(newNode.m_depth);
                    }

                    // Get the incoming radiance
                    if ( siCurrent == -1 ) {
                        // Storage bounce
                        radiance = SR_GetRadiance(&newNode, config, READ_NOW, nrSamples);
                    } else {
                        radiance = SR_GetRadiance(&newNode, config, readout, nrSamples);
                    }

                    // Frame coherent & correlated sampling
                    if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
                        config->seedConfig.Restore(newNode.m_depth);
                    }

                    // Collect outgoing radiance
                    factor = newNode.m_G / (newNode.m_pdfFromPrev * nrSamples);

                    colorProductScaled(radiance, (float)factor, thisNode->m_bsdfEval, radiance);
                    colorAdd(radiance, result, result);
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

COLOR
SR_GetDirectRadiance(
        SimpleRaytracingPathNode *prevNode,
        StochasticRaytracingConfiguration *config,
        StorageReadout readout)
{
    COLOR result;
    COLOR radiance;
    colorClear(result);
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
        int i;
        SimpleRaytracingPathNode lightNode;
        double x_1;
        double x_2;
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

            for ( i = 0; i < config->nextEventSamples; i++ ) {
                // Light sampling
                stratified.sample(&x_1, &x_2);

                if ( config->samplerConfig.neSampler->sample(
                        prevNode->previous(),
                        prevNode,
                        &lightNode,
                        x_1,
                        x_2,
                        true,
                        BSDF_ALL_COMPONENTS) ) {

                    if ( pathNodesVisible(prevNode, &lightNode) ) {
                        // Now connect for all applicable scatter-info's
                        // If no weighting between reflection sampling and
                        // next event estimation were used, only one connect
                        // using the union of different scatter info flags
                        // are necessary (=speedup)
                        int siCurrent;
                        CScatterInfo *si;

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

                            if ( (config->reflectionSampling == PHOTON_MAP_SAMPLING)
                                || (config->reflectionSampling == CLASSICAL_SAMPLING) ) {
                                if ( si->flags & BSDF_SPECULAR_COMPONENT ) {
                                    // Perfect mirror reflection, no n.e.e.
                                    doSi = false;
                                }
                            }

                            if ( doSi ) {
                                // Connect using correct flags
                                geom = pathNodeConnect(
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
                                    if ( si->DoneSomePreviousBounce(prevNode)) {
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
                                colorProductScaled(prevNode->m_bsdfEval, (float)factor, lightNode.m_bsdfEval,
                                                   radiance);

                                // Collect outgoing radiance
                                colorAdd(result, radiance, result);
                            } // if not photon map or no caustic path

                            // Next scatter info block
                            siCurrent++;
                            if ( siCurrent < config->siOthersCount ) {
                                si = &config->siOthers[siCurrent];
                            }

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

COLOR
SR_GetRadiance(
        SimpleRaytracingPathNode *thisNode,
        StochasticRaytracingConfiguration *config,
        StorageReadout readout,
        int usedScatterSamples)
{
    COLOR result;
    COLOR radiance;
    XXDFFLAGS edfFlags = ALL_COMPONENTS;

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
                 config->samplerConfig.neSampler->evalPDF(thisNode->previous(), thisNode);
            cl = multipleImportanceSampling(cl);
            cr = usedScatterSamples * thisNode->m_pdfFromPrev;
            cr = multipleImportanceSampling(cr);
            weight = cr / (cr + cl);
        }

        result = backgroundRadiance(GLOBAL_scene_background, &(thisNode->previous()->m_hit.point),
                                    &(thisNode->m_inDirF), nullptr);

        colorScale((float)weight, result, result);
    } else {
        // Handle non-background
        EDF *thisEdf = thisNode->m_hit.material->edf;

        colorClear(result);

        // Stored radiance
        if ( (readout == READ_NOW) && (config->siStorage.flags != NO_COMPONENTS) ) {
            // Add the stored radiance being emitted from the patch
            if ( GLOBAL_radiance_currentRadianceMethodHandle == &GLOBAL_photonMapMethods ) {
                if ( config->radMode == STORED_PHOTON_MAP ) {
                    // Check if the distance to the previous point is big enough
                    // otherwise we need more scattering...
                    float dist2 = vectorDist2(thisNode->m_hit.point, thisNode->previous()->m_hit.point);

                    if ( dist2 > PHOTON_MAP_MIN_DIST2 ) {
                        radiance = photonMapGetNodeGRadiance(thisNode);
                        // This does not include Le (self emitted light)
                    } else {
                        colorClear(radiance);
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
                thisNode->m_hit.patch->uv(&thisNode->m_hit.point, &u, &v);

                radiance = GLOBAL_radiance_selectedRadianceMethod->getRadiance(
                    thisNode->m_hit.patch, u, v, thisNode->m_inDirF);

                // This includes Le diffuse, subtract first and handle total emitted later (possibly weighted)
                // -- Interface mechanism needed to determine what a
                // -- radiance method does...
                COLOR diffEmit;

                diffEmit = edfEval(thisEdf, &thisNode->m_hit, &(thisNode->m_inDirF),
                                   BRDF_DIFFUSE_COMPONENT, nullptr);

                colorSubtract(radiance, diffEmit, radiance);
            }

            colorAdd(result, radiance, result);

        } // Done: Stored radiance, no self emitted light included!

        // Stored caustic maps
        if ( (config->radMode == STORED_PHOTON_MAP) && readout == SCATTER ) {
            radiance = photonMapGetNodeCRadiance(thisNode);
            colorAdd(result, radiance, result);
        }

        radiance = SR_GetDirectRadiance(thisNode, config, readout);
        colorAdd(result, radiance, result);

        // Scattered light
        radiance = SR_GetScatteredRadiance(thisNode, config, readout);
        colorAdd(result, radiance, result);

        // Emitted Light
        if ((config->radMode == STORED_PHOTON_MAP) && (GLOBAL_radiance_currentRadianceMethodHandle == &GLOBAL_photonMapMethods) ) {
            // Check if Le would contribute to a caustic
            if ( (readout == READ_NOW) && !(config->siStorage.DoneThisBounce(thisNode->previous())) ) {
                // Caustic contribution:  (E...(D|G)...?L) with ? some specular bounce
                edfFlags = 0;
            }
        }

        if ( (thisEdf != nullptr) && (edfFlags != 0) ) {
            double weight;
            double cr;
            double cl;
            COLOR col;
            bool doWeight = true;

            if ( thisNode->m_depth <= 1 ) {
                doWeight = false;
            }

            if ( config->reflectionSampling == CLASSICAL_SAMPLING ) {
                doWeight = false;
            }

            if ((config->reflectionSampling == PHOTON_MAP_SAMPLING) && (thisNode->m_depth > 1) ) {
                if ( thisNode->previous()->m_usedComponents & BSDF_SPECULAR_COMPONENT) {
                    // Perfect Specular scatter, no weighting
                    doWeight = false;
                }
            }

            if ( doWeight ) {
                cl = config->nextEventSamples *
                     config->samplerConfig.neSampler->evalPDF(thisNode->previous(),
                                                              thisNode);
                cl = multipleImportanceSampling(cl);
                cr = usedScatterSamples * thisNode->m_pdfFromPrev;
                cr = multipleImportanceSampling(cr);

                weight = cr / (cr + cl);
            } else {
                // We don't do N.E.E. from the eye !
                weight = 1;
            }

            col = edfEval(thisEdf, &thisNode->m_hit,
                          &(thisNode->m_inDirF),
                          edfFlags, nullptr);

            colorAddScaled(result, (float)weight, col, result);
        }
    }

    return result;
}

static COLOR
CalcPixel(int nx, int ny, StochasticRaytracingConfiguration *config) {
    int i;
    SimpleRaytracingPathNode eyeNode, pixelNode;
    double x1;
    double x2;
    COLOR col, result;
    StratifiedSampling2D stratified(config->samplesPerPixel);

    colorClear(result);

    // Frame coherent & correlated sampling
    if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
        if ( GLOBAL_raytracing_state.doCorrelatedSampling ) {
            // Correlated : start each pixel with same seed
            srand48(GLOBAL_raytracing_state.baseSeed);
        }
        drand48(); // (randomize seed, gives new seed for uncorrelated sampling)
        config->seedConfig.Save(0);
    }

    // Calc pixel data

    // Sample eye node
    config->samplerConfig.pointSampler->sample(nullptr, nullptr, &eyeNode, 0, 0);
    ((CPixelSampler *) config->samplerConfig.dirSampler)->SetPixel(nx, ny);

    eyeNode.attach(&pixelNode);

    // Stratified sampling of the pixel
    for ( i = 0; i < config->samplesPerPixel; i++ ) {
        stratified.sample(&x1, &x2);

        if ( config->samplerConfig.dirSampler->sample(nullptr, &eyeNode, &pixelNode, x1, x2)
             && ((pixelNode.m_rayType != ENVIRONMENT) || (config->backgroundDirect))) {
            pixelNode.assignBsdfAndNormal();

            // Frame coherent & correlated sampling
            if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
                config->seedConfig.Save(pixelNode.m_depth);
            }

            col = SR_GetRadiance(&pixelNode, config, config->initialReadout,
                                 config->samplesPerPixel);

            // Frame coherent & correlated sampling
            if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
                config->seedConfig.Restore(pixelNode.m_depth);
            }

            // col represents the radiance reflected towards the eye
            // in the pixel sampled point.

            // Account for the eye sampling
            // -- Not needed yet ...

            // Account for pixel sampling
            colorScale((float)(pixelNode.m_G / pixelNode.m_pdfFromPrev), col, col);
            colorAdd(result, col, result);
        }
    }

    // We have now the FLUX for the pixel (x N), convert it to radiance
    double factor = (computeFluxToRadFactor(nx, ny) /
            (float)config->samplesPerPixel);

    colorScale((float)factor, result, result);
    config->screen->add(nx, ny, result);

    // Frame coherent & correlated sampling
    if ( GLOBAL_raytracing_state.doFrameCoherent || GLOBAL_raytracing_state.doCorrelatedSampling ) {
        config->seedConfig.Restore(0);
    }

    return result;
}

/**
Raytrace the current scene as seen with the current camera. If fp
is not a nullptr pointer, write the ray-traced image to the file
pointed to by 'fp'
*/
void
RTStochastic_Trace(
    ImageOutputHandle *ip,
    java::ArrayList<Patch *> * /*scenePatches*/,
    java::ArrayList<Patch *> *lightPatches)
{
    StochasticRaytracingConfiguration config(GLOBAL_raytracing_state, lightPatches); // config filled in by constructor

    // Frame Coherent sampling : init fixed seed
    if ( GLOBAL_raytracing_state.doFrameCoherent ) {
        srand48(GLOBAL_raytracing_state.baseSeed);
    }

    if ( !GLOBAL_raytracing_state.progressiveTracing ) {
        ScreenIterateSequential((COLOR(*)(int, int, void *))CalcPixel, &config);
    } else {
        ScreenIterateProgressive((COLOR(*)(int, int, void *))CalcPixel, &config);
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

static void
RTStochastic_Init(java::ArrayList<Patch *> *lightPatches) {
    // mainInit the light list
    if ( GLOBAL_lightList ) {
        delete GLOBAL_lightList;
    }
    GLOBAL_lightList = new LightList(lightPatches);
}

void
RTStochastic_Terminate() {
    if ( GLOBAL_raytracing_state.lastScreen ) {
        delete GLOBAL_raytracing_state.lastScreen;
    }
    GLOBAL_raytracing_state.lastScreen = nullptr;
}

Raytracer
GLOBAL_raytracing_stochasticMethod =
{
    (char *)"StochasticRaytracing",
    4,
    (char *)"Stochastic Raytracing & Final Gathers",
    stochasticRayTracerDefaults,
    RTStochasticParseOptions,
    RTStochastic_Init,
    RTStochastic_Trace,
    RTStochastic_Redisplay,
    RTStochastic_SaveImage,
    RTStochastic_Terminate
};
