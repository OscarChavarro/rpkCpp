#include <cstdlib>
#include <ctime>

#include "java/util/ArrayList.h"
#include "material/color.h"
#include "common/error.h"
#include "skin/Vertex.h"
#include "skin/Patch.h"
#include "shared/render.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/common/raytools.h"
#include "raycasting/common/ScreenBuffer.h"
#include "raycasting/common/pathnode.h"
#include "raycasting/simple/RayCaster.h"
#include "raycasting/raytracing/bipath.h"
#include "raycasting/raytracing/eyesampler.h"
#include "raycasting/raytracing/lightsampler.h"
#include "raycasting/raytracing/lightdirsampler.h"
#include "raycasting/raytracing/bsdfsampler.h"
#include "raycasting/raytracing/samplertools.h"
#include "PHOTONMAP/photonmapsampler.h"
#include "PHOTONMAP/screensampler.h"
#include "PHOTONMAP/photonmap.h"
#include "PHOTONMAP/importancemap.h"
#include "PHOTONMAP/PhotonMapRadiosity.h"
#include "PHOTONMAP/pmapoptions.h"
#include "PHOTONMAP/pmapconfig.h"
#include "PHOTONMAP/pmapimportance.h"

PMAPCONFIG GLOBAL_photonMap_config;

// To adjust photonMapGetRadiance returns
static bool s_doingLocalRaycast = false;

#define STRING_LENGTH 1000

/**
Initializes the rendering methods 'state' structure. Don't forget to
update radiance.c to call this routine!
*/
static
void photonMapDefaults() {
    GLOBAL_photonMap_state.Defaults();
}

/**
For counting how much CPU time was used for the computations
*/
static void
photonMapRadiosityUpdateCpuSecs() {
    clock_t t;

    t = clock();
    GLOBAL_photonMap_state.cpu_secs += (float) (t - GLOBAL_photonMap_state.lastclock) / (float) CLOCKS_PER_SEC;
    GLOBAL_photonMap_state.lastclock = t;
}

static Element *
photonMapCreatePatchData(Patch *patch) {
    return patch->radianceData = nullptr;
}

static void
photonMapPrintPatchData(FILE *out, Patch *patch) {
    fprintf(out, "No data\n");
}

static void
photonMapDestroyPatchData(Patch *patch) {
    patch->radianceData = nullptr;
}

static void
photonMapChooseSurfaceSampler(CSurfaceSampler **samplerPtr) {
    if ( *samplerPtr ) {
        delete *samplerPtr;
    }

    if ( GLOBAL_photonMap_state.usePhotonMapSampler ) {
        *samplerPtr = new CPhotonMapSampler;
    } else {
        *samplerPtr = new CBsdfSampler;
    }
}

/**
Initializes the computations for the current scene (if any)
*/
static void
photonMapInitPmap(java::ArrayList<Patch *> * /*scenePatches*/) {
    fprintf(stderr, "Photonmap activated\n");

    GLOBAL_photonMap_state.lastclock = clock();
    GLOBAL_photonMap_state.cpu_secs = 0.;
    GLOBAL_photonMap_state.g_iteration_nr = 0;
    GLOBAL_photonMap_state.c_iteration_nr = 0;
    GLOBAL_photonMap_state.i_iteration_nr = 0;
    GLOBAL_photonMap_state.iteration_nr = 0;
    GLOBAL_photonMap_state.runstop_nr = 0;
    GLOBAL_photonMap_state.total_gpaths = 0;
    GLOBAL_photonMap_state.total_cpaths = 0;
    GLOBAL_photonMap_state.total_ipaths = 0;
    GLOBAL_photonMap_state.total_rays = 0;

    if ( GLOBAL_photonMap_config.screen ) {
        delete GLOBAL_photonMap_config.screen;
    }
    GLOBAL_photonMap_config.screen = new ScreenBuffer(nullptr);

    // mainInit samplers

    GLOBAL_photonMap_config.lightConfig.ReleaseVars();
    GLOBAL_photonMap_config.eyeConfig.ReleaseVars();

    CSamplerConfig *cfg = &GLOBAL_photonMap_config.eyeConfig;

    cfg->pointSampler = new CEyeSampler;

    cfg->dirSampler = new CScreenSampler;  // ps;

    photonMapChooseSurfaceSampler(&cfg->surfaceSampler);
    // cfg->surfaceSampler = new CPhotonMapSampler;
    // cfg->surfaceSampler = new CBsdfSampler;
    cfg->surfaceSampler->SetComputeFromNextPdf(false);
    cfg->neSampler = nullptr;

    cfg->minDepth = 1;
    cfg->maxDepth = 1;  // Only eye point needed, for Particle tracing test

    cfg = &GLOBAL_photonMap_config.lightConfig;

    cfg->pointSampler = new CUniformLightSampler;
    cfg->dirSampler = new CLightDirSampler;
    photonMapChooseSurfaceSampler(&cfg->surfaceSampler);
    // cfg->surfaceSampler = new CPhotonMapSampler; //new CBsdfSampler;
    cfg->surfaceSampler->SetComputeFromNextPdf(false);  // Only 1 pdf

    cfg->minDepth = GLOBAL_photonMap_state.minimumLightPathDepth;
    cfg->maxDepth = GLOBAL_photonMap_state.maximumLightPathDepth;

    GLOBAL_raytracer_rayCount = 0;

    // mainInit the photonmap

    if ( GLOBAL_photonMap_config.globalMap ) {
        delete GLOBAL_photonMap_config.globalMap;
    }
    GLOBAL_photonMap_config.globalMap = new CPhotonMap(&GLOBAL_photonMap_state.reconGPhotons,
                                                       GLOBAL_photonMap_state.precomputeGIrradiance);

    if ( GLOBAL_photonMap_config.importanceMap ) {
        delete GLOBAL_photonMap_config.importanceMap;
    }
    GLOBAL_photonMap_config.importanceMap = new CImportanceMap(&GLOBAL_photonMap_state.reconIPhotons,
                                                               &GLOBAL_photonMap_state.gImpScale);

    if ( GLOBAL_photonMap_config.importanceCMap ) {
        delete GLOBAL_photonMap_config.importanceCMap;
    }
    GLOBAL_photonMap_config.importanceCMap = new CImportanceMap(&GLOBAL_photonMap_state.reconIPhotons,
                                                                &GLOBAL_photonMap_state.cImpScale);

    if ( GLOBAL_photonMap_config.causticMap ) {
        delete GLOBAL_photonMap_config.causticMap;
    }
    GLOBAL_photonMap_config.causticMap = new CPhotonMap(&GLOBAL_photonMap_state.reconCPhotons);
}

/**
Adapted from bidirpath, this is a bit overkill for here
*/
static COLOR
photonMapDoComputePixelFluxEstimate(PMAPCONFIG *config) {
    CBiPath *bp = &config->bipath;
    CPathNode *eyePrevNode;
    CPathNode *lightPrevNode;
    COLOR oldBsdfL;
    COLOR oldBsdfE;
    CBsdfComp oldBsdfCompL;
    CBsdfComp oldBsdfCompE;
    double oldPdfL;
    double oldPdfE;
    double oldRRPdfL;
    double oldRRPdfE;
    double oldPdfLP = 0.0;
    double oldPdfEP = 0.0;
    double oldRRPdfLP = 0.0;
    double oldRRPdfEP = 0.0;
    COLOR f;
    CPathNode *eyeEndNode;
    CPathNode *lightEndNode;

    // Store PDF and BSDF evals that will be overwritten

    eyeEndNode = bp->m_eyeEndNode;
    lightEndNode = bp->m_lightEndNode;
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

    // Connect the sub-paths
    bp->m_geomConnect =
            PathNodeConnect(eyeEndNode, lightEndNode,
                            &config->eyeConfig, &config->lightConfig,
                            CONNECT_EL | CONNECT_LE,
                            BSDF_ALL_COMPONENTS, BSDF_ALL_COMPONENTS, &bp->m_dirEL);

    VECTORSCALE(-1, bp->m_dirEL, bp->m_dirLE);

    // Evaluate radiance and pdf and weight
    f = bp->EvalRadiance();

    double factor = 1.0 / bp->EvalPDFAcc();

    colorScale(factor, f, f); // Flux estimate

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
Test next event estimator to the screen. The result is standard
particle tracing, although constructing global & caustic together
does not give correct display
*/
void
photonMapDoScreenNEE(PMAPCONFIG *config) {
    int nx, ny;
    float pix_x, pix_y;
    COLOR f;
    CBiPath *bp = &config->bipath;

    if ( config->currentMap == config->importanceMap ) {
        return;
    }

    // First we need to determine if the lightEndNode can be seen from
    // the camera. At the same time the pixel hit is computed
    if ( eyeNodeVisible(bp->m_eyeEndNode, bp->m_lightEndNode,
                        &pix_x, &pix_y)) {
        // Visible !
        f = photonMapDoComputePixelFluxEstimate(config);

        config->screen->getPixel(pix_x, pix_y, &nx, &ny);

        float factor;

        if ( config->currentMap == config->globalMap ) {
            factor = (computeFluxToRadFactor(nx, ny)
                      / (float) GLOBAL_photonMap_state.total_gpaths);
        } else {
            factor = (computeFluxToRadFactor(nx, ny)
                      / (float) GLOBAL_photonMap_state.total_cpaths);
        }

        colorScale(factor, f, f);

        config->screen->add(nx, ny, f);
    }
}


/**
Store a photon. Some acceptance tests are performed first
*/
bool
photonMapDoPhotonStore(CPathNode *node, COLOR power) {
    //float scatteredPower;
    //COLOR col;
    BSDF *bsdf;

    if ( node->m_hit.patch && node->m_hit.patch->surface->material ) {
        // Only add photons on surfaces with a certain reflection
        // coefficient.

        bsdf = node->m_hit.patch->surface->material->bsdf;

        if ( !ZeroAlbedo(bsdf, &node->m_hit, BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT)) {
            CPhoton photon(node->m_hit.point, power, node->m_inDirF);

            // Determine photon flags
            short flags = 0;

            if ( node->m_depth == 1 ) {
                // Direct light photon
                flags |= DIRECT_LIGHT_PHOTON;
            }

            if ( GLOBAL_photonMap_state.densityControl == NO_DENSITY_CONTROL ) {
                return GLOBAL_photonMap_config.currentMap->AddPhoton(photon, node->m_hit.normal,
                                                                     flags);
            } else {
                float reqDensity;
                if ( GLOBAL_photonMap_state.densityControl == CONSTANT_RD ) {
                    reqDensity = GLOBAL_photonMap_state.constantRD;
                } else {
                    reqDensity = GLOBAL_photonMap_config.currentImpMap->GetRequiredDensity(node->m_hit.point,
                                                                                           node->m_hit.normal);
                }

                return GLOBAL_photonMap_config.currentMap->DC_AddPhoton(photon, node->m_hit,
                                                                        reqDensity, flags);
            }
        }
    }
    return false;
}

/**
Handle one path : store at all end positions and for testing, connect to the eye
*/
void
photonMapHandlePath(PMAPCONFIG *config) {
    bool ldone;
    CBiPath *bp = &config->bipath;
    COLOR accPower;
    float factor;


    // Iterate over all light nodes

    bp->m_lightSize = 1;
    CPathNode *currentNode = bp->m_lightPath;

    bp->m_eyeSize = 1;
    bp->m_eyeEndNode = bp->m_eyePath;
    bp->m_geomConnect = 1.0; // No connection yet

    ldone = false;
    colorSetMonochrome(accPower, 1.0);

    while ( !ldone ) {
        // Adjust accPower

        factor = currentNode->m_G / currentNode->m_pdfFromPrev;
        colorScale(factor, accPower, accPower);

        // Store photon, but not emitted light

        if ( config->currentMap == config->globalMap ) {
            if ( bp->m_lightSize > 1 ) {
                // Store

                if ( photonMapDoPhotonStore(currentNode, accPower)) {
                    // Screen next event estimation for testing

                    bp->m_lightEndNode = currentNode;
                    photonMapDoScreenNEE(config);
                }
            }
        } else {
            // Caustic map...
            if ( bp->m_lightSize > 2 ) {
                // Store

                if ( photonMapDoPhotonStore(currentNode, accPower)) {
                    // Screen next event estimation for testing

                    bp->m_lightEndNode = currentNode;
                    photonMapDoScreenNEE(config);
                }
            }
        }

        // Account for bsdf, node that for the first node, this accounts
        // for the emitted radiance.
        if ( !(currentNode->Ends())) {
            colorProduct(currentNode->m_bsdfEval, accPower, accPower);

            currentNode = currentNode->Next();
            bp->m_lightSize++;
        } else {
            ldone = true;
        }
    }
}

static void
photonMapTracePath(PMAPCONFIG *config, BSDFFLAGS bsdfFlags) {
    config->bipath.m_eyePath = config->eyeConfig.TracePath(config->bipath.m_eyePath);

    // Use qmc for light sampling

    double x_1, x_2;
    // unsigned *nrs = Nied31(pmapQMCseed_s++);

    CPathNode *path = config->bipath.m_lightPath;

    // First node
    x_1 = drand48(); //nrs[0] * RECIP;
    x_2 = drand48(); //nrs[1] * RECIP;

    path = config->lightConfig.TraceNode(path, x_1, x_2, bsdfFlags);
    if ( path == nullptr ) {
        return;
    }

    config->bipath.m_lightPath = path;  // In case no nodes were present

    path->EnsureNext();

    // Second node
    CPathNode *node = path->Next();
    x_1 = drand48(); // nrs[2] * RECIP;
    x_2 = drand48(); // nrs[3] * RECIP; // 4D Niederreiter...

    if ( config->lightConfig.TraceNode(node, x_1, x_2, bsdfFlags)) {
        // Succesful trace...
        node->EnsureNext();
        config->lightConfig.TracePath(node->Next(), bsdfFlags);
    }
}

static void
photonMapTracePaths(int nrPaths, BSDFFLAGS bsdfFlags = BSDF_ALL_COMPONENTS) {
    int i;

    // Fill in config structures
    for ( i = 0; i < nrPaths; i++ ) {
        photonMapTracePath(&GLOBAL_photonMap_config, bsdfFlags);
        photonMapHandlePath(&GLOBAL_photonMap_config);
    }
}

static void
photonMapBRRealIteration() {
    GLOBAL_photonMap_state.iteration_nr++;

    fprintf(stderr, "GLOBAL_photonMapMethods Iteration %li\n", (long) GLOBAL_photonMap_state.iteration_nr);

    if ((GLOBAL_photonMap_state.iteration_nr > 1) && (GLOBAL_photonMap_state.doGlobalMap || GLOBAL_photonMap_state.doCausticMap)) {
        float scaleFactor = (GLOBAL_photonMap_state.iteration_nr - 1.0) / (float) GLOBAL_photonMap_state.iteration_nr;
        GLOBAL_photonMap_config.screen->scaleRadiance(scaleFactor);
    }

    if ((GLOBAL_photonMap_state.densityControl == IMPORTANCE_RD) && GLOBAL_photonMap_state.doImportanceMap ) {
        GLOBAL_photonMap_state.i_iteration_nr++;
        GLOBAL_photonMap_config.currentMap = GLOBAL_photonMap_config.importanceMap;
        GLOBAL_photonMap_state.total_ipaths = GLOBAL_photonMap_state.i_iteration_nr * GLOBAL_photonMap_state.ipaths_per_iteration;
        GLOBAL_photonMap_config.currentMap->SetTotalPaths(GLOBAL_photonMap_state.total_ipaths);
        GLOBAL_photonMap_config.importanceCMap->SetTotalPaths(GLOBAL_photonMap_state.total_ipaths);

        TracePotentialPaths(GLOBAL_photonMap_state.ipaths_per_iteration);

        fprintf(stderr, "Total potential paths : %li, Total rays %li\n",
                GLOBAL_photonMap_state.total_ipaths,
                GLOBAL_raytracer_rayCount);
    }

    // Global map
    if ( GLOBAL_photonMap_state.doGlobalMap ) {
        GLOBAL_photonMap_state.g_iteration_nr++;
        GLOBAL_photonMap_config.currentMap = GLOBAL_photonMap_config.globalMap;
        GLOBAL_photonMap_state.total_gpaths = GLOBAL_photonMap_state.g_iteration_nr * GLOBAL_photonMap_state.gpaths_per_iteration;
        GLOBAL_photonMap_config.currentMap->SetTotalPaths(GLOBAL_photonMap_state.total_gpaths);

        // Set correct importance map: indirect importance
        GLOBAL_photonMap_config.currentImpMap = GLOBAL_photonMap_config.importanceMap;

        photonMapTracePaths(GLOBAL_photonMap_state.gpaths_per_iteration);

        fprintf(stderr, "Global map: ");
        GLOBAL_photonMap_config.globalMap->PrintStats(stderr);
    }

    // Caustic map
    if ( GLOBAL_photonMap_state.doCausticMap ) {
        GLOBAL_photonMap_state.c_iteration_nr++;
        GLOBAL_photonMap_config.currentMap = GLOBAL_photonMap_config.causticMap;
        GLOBAL_photonMap_state.total_cpaths = GLOBAL_photonMap_state.c_iteration_nr * GLOBAL_photonMap_state.cpaths_per_iteration;
        GLOBAL_photonMap_config.currentMap->SetTotalPaths(GLOBAL_photonMap_state.total_cpaths);

        // Set correct importance map: direct importance
        GLOBAL_photonMap_config.currentImpMap = GLOBAL_photonMap_config.importanceCMap;

        photonMapTracePaths(GLOBAL_photonMap_state.cpaths_per_iteration, BSDF_SPECULAR_COMPONENT);

        fprintf(stderr, "Caustic map: ");
        GLOBAL_photonMap_config.causticMap->PrintStats(stderr);
    }
}

/**
Performs one step of the radiance computations. The goal most often is
to fill in a RGB color for display of each patch and/or vertex. These
colors are used for hardware rendering if the default hardware rendering
method is not superceeded in this file
*/
static int
photonMapDoStep(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> * /*lightPatches*/) {
    GLOBAL_photonMap_state.wake_up = false;
    GLOBAL_photonMap_state.lastclock = clock();

    photonMapBRRealIteration();
    photonMapRadiosityUpdateCpuSecs();

    GLOBAL_photonMap_state.runstop_nr++;

    return false; // Done. Return false if you want the computations to continue
}

/**
Undoes the effect of mainInit() and all side-effects of Step()
*/
static void
photonMapTerminate(java::ArrayList<Patch *> * /*scenePatches*/) {
    if ( GLOBAL_photonMap_config.screen ) {
        delete GLOBAL_photonMap_config.screen;
        GLOBAL_photonMap_config.screen = nullptr;
    }

    GLOBAL_photonMap_config.lightConfig.ReleaseVars();
    GLOBAL_photonMap_config.eyeConfig.ReleaseVars();

    if ( GLOBAL_photonMap_config.globalMap ) {
        delete GLOBAL_photonMap_config.globalMap;
        GLOBAL_photonMap_config.globalMap = nullptr;
    }

    if ( GLOBAL_photonMap_config.importanceMap ) {
        delete GLOBAL_photonMap_config.importanceMap;
        GLOBAL_photonMap_config.importanceMap = nullptr;
    }

    if ( GLOBAL_photonMap_config.importanceCMap ) {
        delete GLOBAL_photonMap_config.importanceCMap;
        GLOBAL_photonMap_config.importanceCMap = nullptr;
    }

    if ( GLOBAL_photonMap_config.causticMap ) {
        delete GLOBAL_photonMap_config.causticMap;
        GLOBAL_photonMap_config.causticMap = nullptr;
    }
}

/**
Returns the radiance emitted in the node related direction
*/
COLOR
photonMapGetNodeGRadiance(CPathNode *node) {
    COLOR col;

    GLOBAL_photonMap_config.globalMap->DoBalancing(GLOBAL_photonMap_state.balanceKDTree);
    col = GLOBAL_photonMap_config.globalMap->Reconstruct(&node->m_hit, node->m_inDirF,
                                                         node->m_useBsdf,
                                                         node->m_inBsdf, node->m_outBsdf);
    return col;
}

/**
Returns the radiance emitted in the node related direction
*/
COLOR
photonMapGetNodeCRadiance(CPathNode *node) {
    COLOR col;

    GLOBAL_photonMap_config.causticMap->DoBalancing(GLOBAL_photonMap_state.balanceKDTree);

    col = GLOBAL_photonMap_config.causticMap->Reconstruct(&node->m_hit, node->m_inDirF,
                                                          node->m_useBsdf,
                                                          node->m_inBsdf, node->m_outBsdf);
    return col;
}

static COLOR
photonMapGetRadiance(Patch *patch,
                     double u, double v,
                     Vector3D dir) {
    RayHit hit;
    Vector3D point;
    BSDF *bsdf = patch->surface->material->bsdf;
    COLOR col;
    float density;

    patchPoint(patch, u, v, &point);
    hitInit(&hit, patch, nullptr, &point, &patch->normal, patch->surface->material, 0.);
    hitShadingNormal(&hit, &hit.normal);

    if ( ZeroAlbedo(bsdf, &hit, BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT)) {
        colorClear(col);
        return col;
    }

    RADRETURN_OPTION radreturn = GLOBAL_RADIANCE;

    if ( s_doingLocalRaycast ) {
        radreturn = GLOBAL_photonMap_state.radianceReturn;
    }

    switch ( radreturn ) {
        case GLOBAL_DENSITY:
            col = GLOBAL_photonMap_config.globalMap->GetDensityColor(hit);
            break;
        case CAUSTIC_DENSITY:
            col = GLOBAL_photonMap_config.causticMap->GetDensityColor(hit);
            break;
        case IMPORTANCE_CDENSITY:
            col = GLOBAL_photonMap_config.importanceCMap->GetDensityColor(hit);
            break;
        case IMPORTANCE_GDENSITY:
            col = GLOBAL_photonMap_config.importanceMap->GetDensityColor(hit);
            break;
        case REC_CDENSITY:
            GLOBAL_photonMap_config.importanceCMap->DoBalancing(GLOBAL_photonMap_state.balanceKDTree);
            density = GLOBAL_photonMap_config.importanceCMap->GetRequiredDensity(hit.point, hit.normal);
            col = GetFalseColor(density);
            break;
        case REC_GDENSITY:
            GLOBAL_photonMap_config.importanceMap->DoBalancing(GLOBAL_photonMap_state.balanceKDTree);
            density = GLOBAL_photonMap_config.importanceMap->GetRequiredDensity(hit.point, hit.normal);
            col = GetFalseColor(density);
            break;
        case GLOBAL_RADIANCE:
            col = GLOBAL_photonMap_config.globalMap->Reconstruct(&hit, dir,
                                                                 bsdf,
                                                                 nullptr, bsdf);
            break;
        case CAUSTIC_RADIANCE:
            col = GLOBAL_photonMap_config.causticMap->Reconstruct(&hit, dir,
                                                                  bsdf,
                                                                  nullptr, bsdf);
            break;
        default: colorClear(col);
            logError("photonMapGetRadiance", "Unknown radiance return");
    }

    return col;
}

static void
photonMapRenderScreen(java::ArrayList<Patch *> *scenePatches) {
    if ( GLOBAL_photonMap_config.screen && GLOBAL_photonMap_state.renderImage ) {
        GLOBAL_photonMap_config.screen->render();
    } else {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            openGlRenderPatch(scenePatches->get(i));
        }
    }
}

static char *
photonMapGetStats() {
    static char stats[STRING_LENGTH];
    char *p;
    int n;

    p = stats;
    snprintf(p, STRING_LENGTH, "PMAP Statistics:\n\n%n", &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Ray count %li\n%n", GLOBAL_raytracer_rayCount, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Time %g\n%n", GLOBAL_photonMap_state.cpu_secs, &n);
    p += n;

    if ( GLOBAL_photonMap_config.globalMap ) {
        snprintf(p, STRING_LENGTH, "Global Map: %n", &n);
        p += n;
        GLOBAL_photonMap_config.globalMap->getStats(p, STRING_LENGTH);
        p += strlen(p);
        snprintf(p, STRING_LENGTH, "\n%n", &n);
        p += n;
    }
    if ( GLOBAL_photonMap_config.causticMap ) {
        snprintf(p, STRING_LENGTH, "Caustic Map: %n", &n);
        p += n;
        GLOBAL_photonMap_config.causticMap->getStats(p, STRING_LENGTH);
        p += strlen(p);
        snprintf(p, STRING_LENGTH, "\n%n", &n);
        p += n;
    }
    if ( GLOBAL_photonMap_config.importanceMap ) {
        snprintf(p, STRING_LENGTH, "Global Importance Map: %n", &n);
        p += n;
        GLOBAL_photonMap_config.importanceMap->getStats(p, STRING_LENGTH);
        p += strlen(p);
        snprintf(p, STRING_LENGTH, "\n%n", &n);
        p += n;
    }
    if ( GLOBAL_photonMap_config.importanceCMap ) {
        snprintf(p, STRING_LENGTH, "Caustic Importance Map: %n", &n);
        p += n;
        GLOBAL_photonMap_config.importanceCMap->getStats(p, STRING_LENGTH);
        p += strlen(p);
        snprintf(p, STRING_LENGTH, "\n%n", &n);
    }

    return stats;
}

RADIANCEMETHOD GLOBAL_photonMapMethods = {
    "PMAP",
    4,
    "PhotonMap",
    photonMapDefaults,
    photonMapParseOptions,
    photonMapInitPmap,
    photonMapDoStep,
    photonMapTerminate,
    photonMapGetRadiance,
    photonMapCreatePatchData,
    photonMapPrintPatchData,
    photonMapDestroyPatchData,
    photonMapGetStats,
    photonMapRenderScreen,
    nullptr
};
