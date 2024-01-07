#include <cstdlib>
#include <ctime>

#include "material/color.h"
#include "common/error.h"
#include "scene/scene.h"
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

// To adjust GetPmapRadiance returns
static bool s_doingLocalRaycast = false;

/* Initializes the rendering methods 'state' structure. Don't forget to
 * update radiance.c to call this routine! */
static
void PmapDefaults() {
    pmapstate.Defaults();
}

/* for counting how much CPU time was used for the computations */
static void
UpdateCpuSecs() {
    clock_t t;

    t = clock();
    pmapstate.cpu_secs += (float) (t - pmapstate.lastclock) / (float) CLOCKS_PER_SEC;
    pmapstate.lastclock = t;
}

static void *
CreatePatchData(PATCH *patch) {
    return patch->radiance_data = nullptr;
}

static void
PrintPatchData(FILE *out, PATCH *patch) {
    fprintf(out, "No data\n");
}

static void
DestroyPatchData(PATCH *patch) {
    patch->radiance_data = (void *) nullptr;
}

/* compute new color for the patch */
static void
PatchComputeNewColor(PATCH *patch) {
    COLOR Rd = patchAverageNormalAlbedo(patch, BRDF_DIFFUSE_COMPONENT);
    convertColorToRGB(Rd, &patch->color);
    patchComputeVertexColors(patch);
}

static void
ChooseSurfaceSampler(CSurfaceSampler **samplerPtr) {
    if ( *samplerPtr ) {
        delete *samplerPtr;
    }

    if ( pmapstate.usePhotonMapSampler ) {
        *samplerPtr = new CPhotonMapSampler;
    } else {
        *samplerPtr = new CBsdfSampler;
    }
}

/* Initializes the computations for the current scene (if any). */
static void
InitPmap() {
    fprintf(stderr, "Photonmap activated\n");

    pmapstate.lastclock = clock();
    pmapstate.cpu_secs = 0.;
    pmapstate.g_iteration_nr = 0;
    pmapstate.c_iteration_nr = 0;
    pmapstate.i_iteration_nr = 0;
    pmapstate.iteration_nr = 0;
    pmapstate.runstop_nr = 0;
    pmapstate.total_gpaths = 0;
    pmapstate.total_cpaths = 0;
    pmapstate.total_ipaths = 0;
    pmapstate.total_rays = 0;

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

    ChooseSurfaceSampler(&cfg->surfaceSampler);
    // cfg->surfaceSampler = new CPhotonMapSampler;
    // cfg->surfaceSampler = new CBsdfSampler;
    cfg->surfaceSampler->SetComputeFromNextPdf(false);
    cfg->neSampler = nullptr;

    cfg->minDepth = 1;
    cfg->maxDepth = 1;  // Only eye point needed, for Particle tracing test

    cfg = &GLOBAL_photonMap_config.lightConfig;

    cfg->pointSampler = new CUniformLightSampler;
    cfg->dirSampler = new CLightDirSampler;
    ChooseSurfaceSampler(&cfg->surfaceSampler);
    // cfg->surfaceSampler = new CPhotonMapSampler; //new CBsdfSampler;
    cfg->surfaceSampler->SetComputeFromNextPdf(false);  // Only 1 pdf

    cfg->minDepth = pmapstate.minimumLightPathDepth;
    cfg->maxDepth = pmapstate.maximumLightPathDepth;

    Global_Raytracer_rayCount = 0;

    // mainInit the photonmap

    if ( GLOBAL_photonMap_config.globalMap ) {
        delete GLOBAL_photonMap_config.globalMap;
    }
    GLOBAL_photonMap_config.globalMap = new CPhotonMap(&pmapstate.reconGPhotons,
                                                       pmapstate.precomputeGIrradiance);

    if ( GLOBAL_photonMap_config.importanceMap ) {
        delete GLOBAL_photonMap_config.importanceMap;
    }
    GLOBAL_photonMap_config.importanceMap = new CImportanceMap(&pmapstate.reconIPhotons,
                                                               &pmapstate.gImpScale);

    if ( GLOBAL_photonMap_config.importanceCMap ) {
        delete GLOBAL_photonMap_config.importanceCMap;
    }
    GLOBAL_photonMap_config.importanceCMap = new CImportanceMap(&pmapstate.reconIPhotons,
                                                                &pmapstate.cImpScale);

    if ( GLOBAL_photonMap_config.causticMap ) {
        delete GLOBAL_photonMap_config.causticMap;
    }
    GLOBAL_photonMap_config.causticMap = new CPhotonMap(&pmapstate.reconCPhotons);
}

/***********************************************************************/

/* Adapted from bidirpath.C, this is a bit overkill for here... */
static COLOR
DoComputePixelFluxEstimate(PMAPCONFIG *config) {
    CBiPath *bp = &config->bipath;
    CPathNode *eyePrevNode, *lightPrevNode;
    COLOR oldBsdfL, oldBsdfE;
    CBsdfComp oldBsdfCompL, oldBsdfCompE;
    double oldPdfL, oldPdfE, oldRRPdfL, oldRRPdfE;
    double oldPdfLP = 0., oldPdfEP = 0., oldRRPdfLP = 0., oldRRPdfEP = 0.;
    COLOR f;
    CPathNode *eyeEndNode, *lightEndNode;

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

    // Connect the subpaths

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
DoScreenNEE(PMAPCONFIG *config) {
    int nx, ny;
    float pix_x, pix_y;
    COLOR f;
    CBiPath *bp = &config->bipath;

    if ( config->currentMap == config->importanceMap ) {
        return;
    }

    // First we need to determine if the lightEndNode can be seen from
    // the camera. At the same time the pixel hit is computed
    if ( EyeNodeVisible(bp->m_eyeEndNode, bp->m_lightEndNode,
                        &pix_x, &pix_y)) {
        // Visible !
        f = DoComputePixelFluxEstimate(config);

        config->screen->GetPixel(pix_x, pix_y, &nx, &ny);

        float factor;

        if ( config->currentMap == config->globalMap ) {
            factor = (ComputeFluxToRadFactor(nx, ny)
                      / (float) pmapstate.total_gpaths);
        } else {
            factor = (ComputeFluxToRadFactor(nx, ny)
                      / (float) pmapstate.total_cpaths);
        }

        colorScale(factor, f, f);

        config->screen->Add(nx, ny, f);
    }
}


/**
Store a photon. Some acceptance tests are performed first
*/
bool
DoPhotonStore(CPathNode *node, COLOR power) {
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

            if ( pmapstate.densityControl == NO_DENSITY_CONTROL ) {
                return GLOBAL_photonMap_config.currentMap->AddPhoton(photon, node->m_hit.normal,
                                                                     flags);
            } else {
                float reqDensity;
                if ( pmapstate.densityControl == CONSTANT_RD ) {
                    reqDensity = pmapstate.constantRD;
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
HandlePath(PMAPCONFIG *config) {
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

                if ( DoPhotonStore(currentNode, accPower)) {
                    // Screen next event estimation for testing

                    bp->m_lightEndNode = currentNode;
                    DoScreenNEE(config);
                }
            }
        } else {
            // Caustic map...
            if ( bp->m_lightSize > 2 ) {
                // Store

                if ( DoPhotonStore(currentNode, accPower)) {
                    // Screen next event estimation for testing

                    bp->m_lightEndNode = currentNode;
                    DoScreenNEE(config);
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

/********* ONE iteration ********/
static void
TracePath(PMAPCONFIG *config, BSDFFLAGS bsdfFlags) {
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
TracePaths(int nrPaths, BSDFFLAGS bsdfFlags = BSDF_ALL_COMPONENTS) {
    int i;

    // Fill in config structures
    for ( i = 0; i < nrPaths; i++ ) {
        TracePath(&GLOBAL_photonMap_config, bsdfFlags);
        HandlePath(&GLOBAL_photonMap_config);
    }
}

static void
BRRealIteration() {
    pmapstate.iteration_nr++;

    fprintf(stderr, "Pmap Iteration %li\n", (long) pmapstate.iteration_nr);

    if ((pmapstate.iteration_nr > 1) && (pmapstate.doGlobalMap || pmapstate.doCausticMap)) {
        float scaleFactor = (pmapstate.iteration_nr - 1.0) / (float) pmapstate.iteration_nr;
        GLOBAL_photonMap_config.screen->ScaleRadiance(scaleFactor);
    }

    if ((pmapstate.densityControl == IMPORTANCE_RD) && pmapstate.doImportanceMap ) {
        pmapstate.i_iteration_nr++;
        GLOBAL_photonMap_config.currentMap = GLOBAL_photonMap_config.importanceMap;
        pmapstate.total_ipaths = pmapstate.i_iteration_nr * pmapstate.ipaths_per_iteration;
        GLOBAL_photonMap_config.currentMap->SetTotalPaths(pmapstate.total_ipaths);
        GLOBAL_photonMap_config.importanceCMap->SetTotalPaths(pmapstate.total_ipaths);

        TracePotentialPaths(pmapstate.ipaths_per_iteration);

        fprintf(stderr, "Total potential paths : %li, Total rays %li\n",
                pmapstate.total_ipaths,
                Global_Raytracer_rayCount);
        //GLOBAL_photonMap_config.importanceMap->BalanceAnalysis();
    }

    // Global map
    if ( pmapstate.doGlobalMap ) {
        pmapstate.g_iteration_nr++;
        GLOBAL_photonMap_config.currentMap = GLOBAL_photonMap_config.globalMap;
        pmapstate.total_gpaths = pmapstate.g_iteration_nr * pmapstate.gpaths_per_iteration;
        GLOBAL_photonMap_config.currentMap->SetTotalPaths(pmapstate.total_gpaths);

        // Set correct importance map: indirect importance
        GLOBAL_photonMap_config.currentImpMap = GLOBAL_photonMap_config.importanceMap;

        TracePaths(pmapstate.gpaths_per_iteration);

        fprintf(stderr, "Global map: ");
        GLOBAL_photonMap_config.globalMap->PrintStats(stderr);
    }

    // Caustic map
    if ( pmapstate.doCausticMap ) {
        pmapstate.c_iteration_nr++;
        GLOBAL_photonMap_config.currentMap = GLOBAL_photonMap_config.causticMap;
        pmapstate.total_cpaths = pmapstate.c_iteration_nr * pmapstate.cpaths_per_iteration;
        GLOBAL_photonMap_config.currentMap->SetTotalPaths(pmapstate.total_cpaths);

        // Set correct importance map: direct importance
        GLOBAL_photonMap_config.currentImpMap = GLOBAL_photonMap_config.importanceCMap;

        TracePaths(pmapstate.cpaths_per_iteration, BSDF_SPECULAR_COMPONENT);

        fprintf(stderr, "Caustic map: ");
        GLOBAL_photonMap_config.causticMap->PrintStats(stderr);
    }
}

/* Performs one step of the radiance computations. The goal most often is
 * to fill in a RGB color for display of each patch and/or vertex. These
 * colors are used for hardware rendering if the default hardware rendering
 * method is not superceeded in this file. */
static int
DoPmapStep() {
    pmapstate.wake_up = false;
    pmapstate.lastclock = clock();

    /* do some real work now */

    BRRealIteration();

    UpdateCpuSecs();

    pmapstate.runstop_nr++;

    return false;    /* done. Return false if you want the computations to
		 * continue. */
}

/* undoes the effect of mainInit() and all side-effects of Step() */
static void
TerminatePmap() {
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

// Returns the radiance emitted in the node related direction
COLOR
GetPmapNodeGRadiance(CPathNode *node) {
    COLOR col;

    GLOBAL_photonMap_config.globalMap->DoBalancing(pmapstate.balanceKDTree);
    col = GLOBAL_photonMap_config.globalMap->Reconstruct(&node->m_hit, node->m_inDirF,
                                                         node->m_useBsdf,
                                                         node->m_inBsdf, node->m_outBsdf);
    return col;
}

// Returns the radiance emitted in the node related direction
COLOR
GetPmapNodeCRadiance(CPathNode *node) {
    COLOR col;

    GLOBAL_photonMap_config.causticMap->DoBalancing(pmapstate.balanceKDTree);

    col = GLOBAL_photonMap_config.causticMap->Reconstruct(&node->m_hit, node->m_inDirF,
                                                          node->m_useBsdf,
                                                          node->m_inBsdf, node->m_outBsdf);
    return col;
}

static COLOR
GetPmapRadiance(PATCH *patch,
                             double u, double v,
                             Vector3D dir) {
    HITREC hit;
    Vector3D point;
    BSDF *bsdf = patch->surface->material->bsdf;
    COLOR col;
    float density;

    patchPoint(patch, u, v, &point);
    InitHit(&hit, patch, nullptr, &point, &patch->normal, patch->surface->material, 0.);
    HitShadingNormal(&hit, &hit.normal);

    if ( ZeroAlbedo(bsdf, &hit, BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT)) {
        colorClear(col);
        return col;
    }

    RADRETURN_OPTION radreturn = GLOBAL_RADIANCE;

    if ( s_doingLocalRaycast ) {
        radreturn = pmapstate.radianceReturn;
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
            GLOBAL_photonMap_config.importanceCMap->DoBalancing(pmapstate.balanceKDTree);
            density = GLOBAL_photonMap_config.importanceCMap->GetRequiredDensity(hit.point, hit.normal);
            col = GetFalseColor(density);
            break;
        case REC_GDENSITY:
            GLOBAL_photonMap_config.importanceMap->DoBalancing(pmapstate.balanceKDTree);
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
            logError("GetPmapRadiance", "Unknown radiance return");
    }

    return col;
}


static void
PmapRenderScreen() {
    if ( GLOBAL_photonMap_config.screen && pmapstate.renderImage ) {
        GLOBAL_photonMap_config.screen->Render();
    } else {
        PatchListIterate(GLOBAL_scene_patches, RenderPatch);
    }
}

static char *
GetPmapStats() {
    static char stats[1000];
    char *p;
    int n;

    p = stats;
    sprintf(p, "PMAP Statistics:\n\n%n", &n);
    p += n;
    sprintf(p, "Ray count %li\n%n", Global_Raytracer_rayCount, &n);
    p += n;
    sprintf(p, "Time %g\n%n", pmapstate.cpu_secs, &n);
    p += n;

    if ( GLOBAL_photonMap_config.globalMap ) {
        sprintf(p, "Global Map: %n", &n);
        p += n;
        GLOBAL_photonMap_config.globalMap->GetStats(p);
        p += strlen(p);
        sprintf(p, "\n%n", &n);
        p += n;
    }
    if ( GLOBAL_photonMap_config.causticMap ) {
        sprintf(p, "Caustic Map: %n", &n);
        p += n;
        GLOBAL_photonMap_config.causticMap->GetStats(p);
        p += strlen(p);
        sprintf(p, "\n%n", &n);
        p += n;
    }
    if ( GLOBAL_photonMap_config.importanceMap ) {
        sprintf(p, "Global Importance Map: %n", &n);
        p += n;
        GLOBAL_photonMap_config.importanceMap->GetStats(p);
        p += strlen(p);
        sprintf(p, "\n%n", &n);
        p += n;
    }
    if ( GLOBAL_photonMap_config.importanceCMap ) {
        sprintf(p, "Caustic Importance Map: %n", &n);
        p += n;
        GLOBAL_photonMap_config.importanceCMap->GetStats(p);
        p += strlen(p);
        sprintf(p, "\n%n", &n);
        p += n;
    }

    return stats;
}

static void
PmapRecomputeDisplayColors() {
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    PatchComputeNewColor(P);
                }
    EndForAll;
}

static void
PmapUpdateMaterial(Material */*oldmaterial*/,
                   Material */*newmaterial*/) {
    logError("PmapUpdateMaterial", "Not yet implemented");
}

RADIANCEMETHOD Pmap = {
    "PMAP",
    4,
    "PhotonMap",
    PmapDefaults,
    ParsePmapOptions,
    PrintPmapOptions,
    InitPmap,
    DoPmapStep,
    TerminatePmap,
    GetPmapRadiance,
    CreatePatchData,
    PrintPatchData,
    DestroyPatchData,
    GetPmapStats,
    PmapRenderScreen,
    PmapRecomputeDisplayColors,
    PmapUpdateMaterial,
    (void (*)(FILE *)) nullptr // use default VRML model saver
};
