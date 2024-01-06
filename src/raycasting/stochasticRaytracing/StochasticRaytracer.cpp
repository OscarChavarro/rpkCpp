#include <cstdlib>

#include "common/Ray.h"
#include "common/linealAlgebra/Float.h"
#include "scene/scene.h"
#include "scene/Background.h"
#include "skin/Patch.h"
#include "skin/radianceinterfaces.h"
#include "material/bsdf.h"
#include "shared/stratification.h"
#include "shared/lightlist.h"
#include "PHOTONMAP/PhotonMapRadiosity.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/raytracing/rtsoptions.h"
#include "raycasting/raytracing/pixelsampler.h"
#include "raycasting/raytracing/samplertools.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/screeniterate.h"
#include "raycasting/common/ScreenBuffer.h"
#include "raycasting/stochasticRaytracing/rtstochasticphotonmap.h"

/*** Defines ***/

// Heuristic for Multiple Importance sampling
#define MISFUNC(a) ((a)*(a))

// Heuristic minimum distance threshold for photon map readouts
// should be tuned and dependent on scene size, ...
// -- TODO Match with importance driven photon maps: gThreshold
const float PMAP_MIN_DIST = 0.02;
const float PMAP_MIN_DIST2 = PMAP_MIN_DIST * PMAP_MIN_DIST; // squared

RTStochastic_State rts;

/******* Get radiance routines for stochastic raytracing *******/

// Forward declaration
COLOR SR_GetRadiance(CPathNode *thisNode, SRCONFIG *config, SRREADOUT readout,
                     int usedScatterSamples);


COLOR SR_GetScatteredRadiance(CPathNode *thisNode, SRCONFIG *config,
                              SRREADOUT readout) {
    int siCurrent; // What scatterblock are we handling
    CScatterInfo *si;

    CPathNode newNode;
    thisNode->Attach(&newNode);

    COLOR result;
    colorClear(result);

    if ((config->samplerConfig.surfaceSampler == nullptr) ||
        (thisNode->m_depth >= config->samplerConfig.maxDepth)) {
        // No scattering
        return result;
    }

    if ((config->siStorage.flags != NO_COMPONENTS) &&
        (readout == SCATTER)) {
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

        if ( si->DoneSomePreviousBounce(thisNode)) {
            nrSamples = si->nrSamplesAfter;
        } else {
            nrSamples = si->nrSamplesBefore;
        } // First bounce of this kind

        // A small optimisation to prevent sampling surface that
        // don't have this scattering component.

        if ( nrSamples > 2 )  // Some bigger value may be more efficient
        {
            COLOR albedo = BsdfScatteredPower(thisNode->m_useBsdf,
                                              &thisNode->m_hit, &thisNode->m_normal,
                                              si->flags);
            if ( COLORAVERAGE(albedo) < EPSILON ) {
                nrSamples = 0;
            } // Skip, no contribution anyway
        }

        // Do we need to compute scattered radiance at all...
        if ((nrSamples > 0) && (thisNode->m_depth + 1 < config->samplerConfig.maxDepth)) {
            int i;
            double x_1, x_2, factor;
            CStrat2D strat(nrSamples);
            COLOR radiance;
            bool doRR = thisNode->m_depth >= config->samplerConfig.minDepth;

            for ( i = 0; i < nrSamples; i++ ) {
                strat.Sample(&x_1, &x_2);

                // Surface sampling

                if ( config->samplerConfig.surfaceSampler->Sample(thisNode->Previous(),
                                                                  thisNode,
                                                                  &newNode, x_1, x_2,
                                                                  doRR,
                                                                  si->flags)
                     && ((newNode.m_rayType != Environment) || (config->backgroundIndirect))) {
                    if ( newNode.m_rayType != Environment ) {
                        newNode.AssignBsdfAndNormal();
                    }

                    // Frame coherent & correlated sampling
                    if ( rts.doFrameCoherent || rts.doCorrelatedSampling ) {
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
                    if ( rts.doFrameCoherent || rts.doCorrelatedSampling ) {
                        config->seedConfig.Restore(newNode.m_depth);
                    }

                    // Collect outgoing radiance
                    factor = newNode.m_G / (newNode.m_pdfFromPrev * nrSamples);

                    COLORPRODSCALED(radiance, factor, thisNode->m_bsdfEval, radiance);
                    COLORADD(radiance, result, result);
                }
            } // for each sample
        } // if nrSamples > 0

        // Next scatter info block
        siCurrent++;
        if ( siCurrent < config->siOthersCount ) {
            si = &config->siOthers[siCurrent];
        }
    } // while not all si done

    thisNode->SetNext(nullptr);
    return result;
}

COLOR SR_GetDirectRadiance(CPathNode *prevNode, SRCONFIG *config,
                           SRREADOUT readout) {
    COLOR result, radiance;
    colorClear(result);
    Vector3D dirEL;

    if ((readout == READ_NOW) && (config->radMode == STORED_PHOTONMAP)) {
        return result;
    } // We're reading out D|G, specular not with direct light

    CNextEventSampler *nes = config->samplerConfig.neSampler;

    // Check if N.E.E. can give a contribution. I.e. not inside
    // a medium or just about to leave to vacuum
    /* -- TODO -- */

    if ((nes != nullptr) &&
        (config->nextEventSamples > 0) &&
        (prevNode->m_depth + 1 < config->samplerConfig.maxDepth)) {
        int i;
        CPathNode lightNode;
        double x_1, x_2, geom, weight, cl, cr, factor, nrs;
        bool lightsToDo = true;

        if ( config->lightMode == ALL_LIGHTS ) {
            lightsToDo = nes->ActivateFirstUnit();
        }


        while ( lightsToDo ) {
            CStrat2D strat(config->nextEventSamples);

            for ( i = 0; i < config->nextEventSamples; i++ ) {
                // Light sampling
                strat.Sample(&x_1, &x_2);

                if ( config->samplerConfig.neSampler->Sample(prevNode->Previous(),
                                                             prevNode,
                                                             &lightNode, x_1, x_2,
                                                             true, BSDF_ALL_COMPONENTS)) {

                    if ( PathNodesVisible(prevNode, &lightNode)) {
                        // Now connect for all applicable scatter-info's
                        // If no weighting between reflection sampling and
                        // next event estimation were used, only one connect
                        // using the union of different scatter info flags
                        // are necessary (=speedup).

                        int siCurrent;
                        CScatterInfo *si;

                        if ((config->siStorage.flags != NO_COMPONENTS) &&
                            (readout == SCATTER)) {
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

                            if ((config->reflectionSampling == PHOTONMAPSAMPLING)
                                // was (config->radMode == STORED_PHOTONMAP)
                                || (config->reflectionSampling == CLASSICALSAMPLING)) {
                                if ( si->flags & BSDF_SPECULAR_COMPONENT) {
                                    doSi = false;
                                } // Perfect mirror reflection, no n.e.e.
                            }

                            if ( doSi ) {
                                // Connect using correct flags
                                geom = PathNodeConnect(prevNode, &lightNode, &config->samplerConfig,
                                                       nullptr, // No light config
                                                       CONNECT_EL, si->flags,
                                                       BSDF_ALL_COMPONENTS,
                                                       &dirEL);

                                // Contribution of this sample (with Multiple Imp. S.)

                                if ( config->reflectionSampling == CLASSICALSAMPLING ) {
                                    weight = 1.0;
                                } else {
                                    // Ndirect * pdf  for the n.e.e.
                                    cl = MISFUNC(config->nextEventSamples * lightNode.m_pdfFromPrev);

                                    // Nscatter * pdf  for possible scattering
                                    if ( si->DoneSomePreviousBounce(prevNode)) {
                                        nrs = si->nrSamplesAfter;
                                    } else {
                                        nrs = si->nrSamplesBefore;
                                    }

                                    cr = MISFUNC(nrs * lightNode.m_pdfFromNext);

                                    // Are we deep enough to do russian roulette
                                    if ( lightNode.m_depth >= config->samplerConfig.minDepth ) {
                                        cr *= MISFUNC(lightNode.m_rrPdfFromNext);
                                    }

                                    weight = cl / (cl + cr);
                                }

                                // printf("cl %g : %i %g, cr %g : %i %g\n", cl,
                                //  config->nextEventSamples, lightNode.m_pdfFromPrev,
                                //  cr, config->scatterSamples, lightNode.m_pdfFromNext);

                                factor = weight * geom / (lightNode.m_pdfFromPrev *
                                                          config->nextEventSamples);
                                COLORPRODSCALED(prevNode->m_bsdfEval, factor, lightNode.m_bsdfEval,
                                                radiance);

                                // Collect outgoing radiance
                                COLORADD(result, radiance, result);
                            } // if not photonmap or no caustic path

                            // Next scatter info block
                            siCurrent++;
                            if ( siCurrent < config->siOthersCount ) {
                                si = &config->siOthers[siCurrent];
                            }

                        } // while not all si done
                    } // if pathnodes visible
                } // for each sample
            } // if light point sampled

            if ( config->lightMode == ALL_LIGHTS ) {
                lightsToDo = nes->ActivateNextUnit();
            } else {
                lightsToDo = false;
            }

        } // while(!lightsDone)
    }
    return result;
}

COLOR SR_GetRadiance(CPathNode *thisNode, SRCONFIG *config, SRREADOUT readout,
                     int usedScatterSamples) {
    COLOR result, radiance;
    //  BSDFFLAGS bsdfFlags = BSDF_ALL_COMPONENTS;
    XXDFFLAGS edfFlags = ALL_COMPONENTS;

    // handle background
    if ( thisNode->m_rayType == Environment ) {
        // check for  weighting
        double weight = 1, cr, cl;

        bool doWeight = true;

        if ( thisNode->m_depth <= 1 ) {   // don't weight direct light
            doWeight = false;
        }

        if ( config->reflectionSampling == CLASSICALSAMPLING ) {
            doWeight = false;
        }

        /*************************************************************/
        /* uncomment if environment light is added to the photon map */
        /*************************************************************
        if((config->radMode == STORED_PHOTONMAP) && (thisNode->m_depth > 1))
       {
           if(thisNode->Previous()->m_usedComponents & BSDF_SPECULAR_COMPONENT)
       oWeight = false;  // Perfect Specular scatter, no weighting
       }
        **************************************************************/

        if ( !(config->backgroundSampling)) {
            doWeight = false;
        }

        if ( doWeight ) {
            cl = config->nextEventSamples *
                 config->samplerConfig.neSampler->EvalPDF(thisNode->Previous(),
                                                          thisNode);
            cl = MISFUNC(cl);
            cr = usedScatterSamples * thisNode->m_pdfFromPrev;
            cr = MISFUNC(cr);

            weight = cr / (cr + cl);
        }

        result = BackgroundRadiance(GLOBAL_scene_background, &(thisNode->Previous()->m_hit.point), &(thisNode->m_inDirF), nullptr);

        colorScale(weight, result, result);
    }
        // handle non-background
    else {
        EDF *thisEdf = thisNode->m_hit.material->edf;

        colorClear(result);

        // Stored radiance

        if ((readout == READ_NOW) && (config->siStorage.flags != NO_COMPONENTS)) {
            /* add the stored radiance being emitted from the patch */

            if ( Radiance == &Pmap ) {
                if ( config->radMode == STORED_PHOTONMAP ) {
                    // Check if the distance to the previous point is big enough
                    // otherwise we need more scattering...

                    float dist2 = VECTORDIST2(thisNode->m_hit.point, thisNode->Previous()->m_hit.point);

                    if ( dist2 > PMAP_MIN_DIST2 ) {
                        radiance = GetPmapNodeGRadiance(thisNode);
                        // This does not include Le (self emitted light)
                    } else {
                        colorClear(radiance);
                        readout = SCATTER; // This ensures extra scattering, direct light and c-map
                    }
                } else {
                    radiance = GetPmapNodeGRadiance(thisNode);
                    // This does not include Le (self emitted light)
                }
            } else {
                // Other radiosity method

                double u, v;

                /* u,v coordinates of intersection point */
                PatchUV(thisNode->m_hit.patch,
                        &thisNode->m_hit.point, &u, &v);

                radiance = Radiance->GetRadiance(thisNode->m_hit.patch, u, v,
                                                 thisNode->m_inDirF);

                // This includes Le diffuse, subtract first and
                // handle total emitted later (possibly weighted)
                // -- Interface mechanism needed to determine what a
                // -- radiance method does...

                COLOR diffEmit;

                diffEmit = EdfEval(thisEdf, &thisNode->m_hit, &(thisNode->m_inDirF),
                                   BRDF_DIFFUSE_COMPONENT, (double *) 0);

                COLORSUBTRACT(radiance, diffEmit, radiance);
            }

            COLORADD(result, radiance, result);

        } // Done: Stored radiance, no self emitted light included!

        // Stored caustic maps

        if ((config->radMode == STORED_PHOTONMAP) && readout == SCATTER ) {
            radiance = GetPmapNodeCRadiance(thisNode);
            COLORADD(result, radiance, result);
        }

        radiance = SR_GetDirectRadiance(thisNode, config, readout);

        COLORADD(result, radiance, result);

        // Scattered light

        radiance = SR_GetScatteredRadiance(thisNode, config, readout);

        COLORADD(result, radiance, result);

        // Emitted Light

        if ((config->radMode == STORED_PHOTONMAP) && (Radiance == &Pmap)) {
            // Check if Le would contribute to a caustic

            if ((readout == READ_NOW) && !(config->siStorage.DoneThisBounce(thisNode->Previous()))) {
                // Caustic contribution:  (E...(D|G)...?L) with ? some specular bounce
                edfFlags = 0;
            }
        }

        if ((thisEdf != nullptr) && (edfFlags != 0)) {
            double weight, cr, cl;
            COLOR col;

            bool doWeight = true;

            if ( thisNode->m_depth <= 1 ) {
                doWeight = false;
            }

            if ( config->reflectionSampling == CLASSICALSAMPLING ) {
                doWeight = false;
            }

            if ((config->reflectionSampling == PHOTONMAPSAMPLING)
                /*(config->radMode == STORED_PHOTONMAP)*/ && (thisNode->m_depth > 1)) {
                if ( thisNode->Previous()->m_usedComponents & BSDF_SPECULAR_COMPONENT) {
                    doWeight = false;
                }  // Perfect Specular scatter, no weighting
            }

            if ( doWeight ) {
                cl = config->nextEventSamples *
                     config->samplerConfig.neSampler->EvalPDF(thisNode->Previous(),
                                                              thisNode);
                cl = MISFUNC(cl);
                cr = usedScatterSamples * thisNode->m_pdfFromPrev;
                cr = MISFUNC(cr);

                weight = cr / (cr + cl);
            } else {
                weight = 1;  // We don't do N.E.E. from the eye !
            }

            col = EdfEval(thisEdf, &thisNode->m_hit,
                          &(thisNode->m_inDirF),
                          edfFlags, (double *) 0);

            COLORADDSCALED(result, weight, col, result);
        }
    }

    return result;
}

static COLOR CalcPixel(int nx, int ny, SRCONFIG *config) {
    int i;
    CPathNode eyeNode, pixelNode;
    double x_1, x_2;
    COLOR col, result;
    CStrat2D strat(config->samplesPerPixel);

    colorClear(result);

    // Frame coherent & correlated sampling
    if ( rts.doFrameCoherent || rts.doCorrelatedSampling ) {
        if ( rts.doCorrelatedSampling ) {
            // Correlated : start each pixel with same seed
            srand48(rts.baseSeed);
        }
        drand48(); // (randomize seed, gives new seed for uncorrelated sampling)
        config->seedConfig.Save(0);
    }

    // Calc pixel data

    //  printf("Pix %i %i : ", nx, ny);

    // Sample eye node

    config->samplerConfig.pointSampler->Sample(nullptr, nullptr, &eyeNode, 0, 0);
    ((CPixelSampler *) config->samplerConfig.dirSampler)->SetPixel(nx, ny);


    eyeNode.Attach(&pixelNode);

    // Stratified sampling of the pixel

    for ( i = 0; i < config->samplesPerPixel; i++ ) {
        strat.Sample(&x_1, &x_2);

        if ( config->samplerConfig.dirSampler->Sample(nullptr, &eyeNode,
                                                      &pixelNode, x_1,
                                                      x_2)
             && ((pixelNode.m_rayType != Environment) || (config->backgroundDirect))) {
            pixelNode.AssignBsdfAndNormal();

            // Frame coherent & correlated sampling
            if ( rts.doFrameCoherent || rts.doCorrelatedSampling ) {
                config->seedConfig.Save(pixelNode.m_depth);
            }


            col = SR_GetRadiance(&pixelNode, config, config->initialReadout,
                                 config->samplesPerPixel);

            // Frame coherent & correlated sampling
            if ( rts.doFrameCoherent || rts.doCorrelatedSampling ) {
                config->seedConfig.Restore(pixelNode.m_depth);
            }

            // col represents the radiance reflected towards the eye
            // in the pixel sampled point.

            // Account for the eye sampling
            // -- Not neccessary yet ...

            // Account for pixel sampling
            colorScale(pixelNode.m_G / pixelNode.m_pdfFromPrev, col, col);
            COLORADD(result, col, result);
        }
    }

    // We have now the FLUX for the pixel (x N), convert it to radiance

    double factor = (ComputeFluxToRadFactor(nx, ny) /
                     config->samplesPerPixel);

    colorScale(factor, result, result);

    // printf("RESULT %g %g %g \n", result.r, result.g, result.b);

    config->screen->Add(nx, ny, result);


    // Frame coherent & correlated sampling
    if ( rts.doFrameCoherent || rts.doCorrelatedSampling ) {
        config->seedConfig.Restore(0);
    }


    return result;
}

/* Raytrace the current scene as seen with the current camera. If fp
 * is not a nullptr pointer, write the raytraced image to the file
 * pointed to by 'fp'. */

void RTStochastic_Trace(ImageOutputHandle *ip) {
    SRCONFIG config(rts); // config filled in by constructor

    // Frame Coherent sampling : init fixed seed
    if ( rts.doFrameCoherent ) {
        srand48(rts.baseSeed);
    }

    CPathNode::m_dmaxsize = 0; // No need for derivative structures

    if ( !rts.progressiveTracing ) {
        ScreenIterateSequential((COLOR(*)(int, int, void *)) CalcPixel, &config);
    } else {
        ScreenIterateProgressive((COLOR(*)(int, int, void *)) CalcPixel, &config);
    }

    config.screen->Render();

    if ( ip ) {
        config.screen->WriteFile(ip);
    }

    if ( rts.lastscreen ) {
        delete rts.lastscreen;
    }
    rts.lastscreen = config.screen;
    config.screen = nullptr;
}

int RTStochastic_Redisplay() {
    if ( rts.lastscreen ) {
        rts.lastscreen->Render();
        return true;
    } else {
        return false;
    }
}

int RTStochastic_SaveImage(ImageOutputHandle *ip) {
    if ( ip && rts.lastscreen ) {
        rts.lastscreen->Sync();
        rts.lastscreen->WriteFile(ip);
        return true;
    } else {
        return false;
    }
}

static void RTStochastic_Interrupt() {
    interrupt_raytracing = true;
}

static void RTStochastic_Init() {
    // Init the light list
    if ( gLightList ) {
        delete gLightList;
    }
    gLightList = new CLightList(GLOBAL_scene_lightSourcePatches);
}

void RTStochastic_Terminate() {
    if ( rts.lastscreen ) {
        delete rts.lastscreen;
    }
    rts.lastscreen = (ScreenBuffer *) 0;
}

Raytracer RT_StochasticMethod =
{
    (char *) "StochasticRaytracing",
    4,
    (char *) "Stochastic Raytracing & Final Gathers",
    RTStochasticDefaults,
    RTStochasticParseOptions,
    RTStochastic_Init,
    RTStochastic_Trace,
    RTStochastic_Redisplay,
    RTStochastic_SaveImage,
    RTStochastic_Interrupt,
    RTStochastic_Terminate
};
