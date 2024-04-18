#include <cstdlib>
#include <ctime>

#include "java/util/ArrayList.h"
#include "common/ColorRgb.h"
#include "common/error.h"
#include "skin/Patch.h"
#include "render/opengl.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/common/raytools.h"
#include "render/ScreenBuffer.h"
#include "raycasting/common/pathnode.h"
#include "raycasting/raytracing/bipath.h"
#include "raycasting/raytracing/eyesampler.h"
#include "raycasting/bidirectionalRaytracing/LightSampler.h"
#include "raycasting/bidirectionalRaytracing/LightDirSampler.h"
#include "raycasting/raytracing/bsdfsampler.h"
#include "raycasting/raytracing/samplertools.h"
#include "PHOTONMAP/photonmapsampler.h"
#include "PHOTONMAP/screensampler.h"
#include "PHOTONMAP/photonmap.h"
#include "PHOTONMAP/importancemap.h"
#include "PHOTONMAP/PhotonMapRadianceMethod.h"
#include "PHOTONMAP/pmapoptions.h"
#include "PHOTONMAP/pmapconfig.h"
#include "PHOTONMAP/pmapimportance.h"

PhotonMapConfig GLOBAL_photonMap_config;

// To adjust photonMapGetRadiance returns
static bool globalDoingLocalRayCasting = false;

#define STRING_LENGTH 1000

PhotonMapRadianceMethod::PhotonMapRadianceMethod() {
    GLOBAL_photonMap_state.defaults();
    className = PHOTON_MAP;
}

PhotonMapRadianceMethod::~PhotonMapRadianceMethod() {
}

const char *
PhotonMapRadianceMethod::getRadianceMethodName() const {
    return "Photon map";
}

void
PhotonMapRadianceMethod::parseOptions(int *argc, char **argv) {
    photonMapParseOptions(argc, argv);
}

void
PhotonMapRadianceMethod::writeVRML(Camera *camera, FILE *fp){
}

/**
For counting how much CPU time was used for the computations
*/
static void
photonMapRadiosityUpdateCpuSecs() {
    clock_t t;

    t = clock();
    GLOBAL_photonMap_state.cpuSecs += (float) (t - GLOBAL_photonMap_state.lastClock) / (float) CLOCKS_PER_SEC;
    GLOBAL_photonMap_state.lastClock = t;
}

Element *
PhotonMapRadianceMethod::createPatchData(Patch *patch) {
    return patch->radianceData = nullptr;
}

void
PhotonMapRadianceMethod::destroyPatchData(Patch *patch) {
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
void
PhotonMapRadianceMethod::initialize(
    const java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry)
{
    fprintf(stderr, "Photon map activated\n");

    GLOBAL_photonMap_state.lastClock = clock();
    GLOBAL_photonMap_state.cpuSecs = 0.0;
    GLOBAL_photonMap_state.gIterationNumber = 0;
    GLOBAL_photonMap_state.cIterationNumber = 0;
    GLOBAL_photonMap_state.i_iteration_nr = 0;
    GLOBAL_photonMap_state.iterationNumber = 0;
    GLOBAL_photonMap_state.runStopNumber = 0;
    GLOBAL_photonMap_state.totalGPaths = 0;
    GLOBAL_photonMap_state.totalCPaths = 0;
    GLOBAL_photonMap_state.totalIPaths = 0;

    if ( GLOBAL_photonMap_config.screen ) {
        delete GLOBAL_photonMap_config.screen;
    }
    GLOBAL_photonMap_config.screen = new ScreenBuffer(nullptr);

    // mainInit samplers

    GLOBAL_photonMap_config.lightConfig.releaseVars();
    GLOBAL_photonMap_config.eyeConfig.releaseVars();

    CSamplerConfig *cfg = &GLOBAL_photonMap_config.eyeConfig;

    cfg->pointSampler = new CEyeSampler;

    cfg->dirSampler = new ScreenSampler;  // ps;

    photonMapChooseSurfaceSampler(&cfg->surfaceSampler);
    // cfg->surfaceSampler = new CPhotonMapSampler;
    // cfg->surfaceSampler = new CBsdfSampler;
    cfg->surfaceSampler->SetComputeFromNextPdf(false);
    cfg->neSampler = nullptr;

    cfg->minDepth = 1;
    cfg->maxDepth = 1;  // Only eye point needed, for Particle tracing test

    cfg = &GLOBAL_photonMap_config.lightConfig;

    cfg->pointSampler = new UniformLightSampler;
    cfg->dirSampler = new LightDirSampler;
    photonMapChooseSurfaceSampler(&cfg->surfaceSampler);
    // cfg->surfaceSampler = new CPhotonMapSampler; //new CBsdfSampler;
    cfg->surfaceSampler->SetComputeFromNextPdf(false);  // Only 1 pdf

    cfg->minDepth = GLOBAL_photonMap_state.minimumLightPathDepth;
    cfg->maxDepth = GLOBAL_photonMap_state.maximumLightPathDepth;

    GLOBAL_raytracer_rayCount = 0;

    // mainInit the photon map

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
Adapted from bi-directional path, this is a bit overkill for here
*/
static ColorRgb
photonMapDoComputePixelFluxEstimate(PhotonMapConfig *config, RadianceMethod * /*context*/) {
    CBiPath *bp = &config->biPath;
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
    eyeEndNode = bp->m_eyeEndNode;
    lightEndNode = bp->m_lightEndNode;
    eyePrevNode = eyeEndNode->previous();
    lightPrevNode = lightEndNode->previous();

    oldBsdfL = lightEndNode->m_bsdfEval;
    oldBsdfCompL = lightEndNode->m_bsdfComp;

    oldBsdfE = eyeEndNode->m_bsdfEval;
    oldBsdfCompE = eyeEndNode->m_bsdfComp;

    oldPdfL = lightEndNode->m_pdfFromNext;

    oldRRPdfL = lightEndNode->m_rrPdfFromNext;

    if ( lightPrevNode ) {
        oldPdfLP = lightPrevNode->m_pdfFromNext;
        oldRRPdfLP = lightPrevNode->m_rrPdfFromNext;
    }

    oldPdfE = eyeEndNode->m_pdfFromNext;
    oldRRPdfE = eyeEndNode->m_rrPdfFromNext;

    if ( eyePrevNode ) {
        oldPdfEP = eyePrevNode->m_pdfFromNext;
        oldRRPdfEP = eyePrevNode->m_rrPdfFromNext;
    }

    // Connect the sub-paths
    bp->m_geomConnect =
            pathNodeConnect(eyeEndNode, lightEndNode,
                            &config->eyeConfig, &config->lightConfig,
                            CONNECT_EL | CONNECT_LE,
                            BSDF_ALL_COMPONENTS, BSDF_ALL_COMPONENTS, &bp->m_dirEL);

    vectorScale(-1, bp->m_dirEL, bp->m_dirLE);

    // Evaluate radiance and probabilityDensityFunction and weight
    f = bp->EvalRadiance();

    float factor = 1.0f / (float)bp->EvalPDFAcc();

    f.scale(factor); // Flux estimate

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
static void
photonMapDoScreenNEE(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    PhotonMapConfig *config,
    RadianceMethod *context)
{
    int nx;
    int ny;
    float pixX;
    float pixY;
    ColorRgb f;
    CBiPath *bp = &config->biPath;

    if ( config->currentMap == config->importanceMap ) {
        return;
    }

    // First we need to determine if the lightEndNode can be seen from
    // the camera. At the same time the pixel hit is computed
    if ( eyeNodeVisible(
            sceneWorldVoxelGrid,
            bp->m_eyeEndNode,
            bp->m_lightEndNode,
            &pixX,
            &pixY) ) {
        // Visible !
        f = photonMapDoComputePixelFluxEstimate(config, context);

        config->screen->getPixel(pixX, pixY, &nx, &ny);

        float factor;

        if ( config->currentMap == config->globalMap ) {
            factor = (computeFluxToRadFactor(camera, nx, ny)
                      / (float) GLOBAL_photonMap_state.totalGPaths);
        } else {
            factor = (computeFluxToRadFactor(camera, nx, ny)
                      / (float) GLOBAL_photonMap_state.totalCPaths);
        }

        f.scale(factor);

        config->screen->add(nx, ny, f);
    }
}


/**
Store a photon. Some acceptance tests are performed first
*/
bool
photonMapDoPhotonStore(SimpleRaytracingPathNode *node, ColorRgb power) {
    //float scatteredPower;
    //COLOR col;
    if ( node->m_hit.patch && node->m_hit.patch->material ) {
        // Only add photons on surfaces with a certain reflection
        // coefficient

        BSDF *bsdf = node->m_hit.patch->material->bsdf;

        if ( !zeroAlbedo(bsdf, &node->m_hit, BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT) ) {
            CPhoton photon(node->m_hit.point, power, node->m_inDirF);

            // Determine photon flags
            short flags = 0;

            if ( node->m_depth == 1 ) {
                // Direct light photon
                flags |= DIRECT_LIGHT_PHOTON;
            }

            if ( GLOBAL_photonMap_state.densityControl == NO_DENSITY_CONTROL ) {
                return GLOBAL_photonMap_config.currentMap->AddPhoton(photon, node->m_hit.normal, flags);
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
static void
photonMapHandlePath(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    PhotonMapConfig *config,
    RadianceMethod *context)
{
    bool lDone;
    CBiPath *bp = &config->biPath;
    ColorRgb accPower;
    float factor;

    // Iterate over all light nodes

    bp->m_lightSize = 1;
    SimpleRaytracingPathNode *currentNode = bp->m_lightPath;

    bp->m_eyeSize = 1;
    bp->m_eyeEndNode = bp->m_eyePath;
    bp->m_geomConnect = 1.0; // No connection yet

    lDone = false;
    accPower.setMonochrome(1.0);

    while ( !lDone ) {
        // Adjust accPower
        factor = (float)(currentNode->m_G / currentNode->m_pdfFromPrev);
        accPower.scale(factor);

        // Store photon, but not emitted light
        if ( config->currentMap == config->globalMap ) {
            if ( bp->m_lightSize > 1 ) {
                // Store
                if ( photonMapDoPhotonStore(currentNode, accPower) ) {
                    // Screen next event estimation for testing

                    bp->m_lightEndNode = currentNode;
                    photonMapDoScreenNEE(camera, sceneWorldVoxelGrid, config, context);
                }
            }
        } else {
            // Caustic map...
            if ( bp->m_lightSize > 2 ) {
                // Store

                if ( photonMapDoPhotonStore(currentNode, accPower) ) {
                    // Screen next event estimation for testing

                    bp->m_lightEndNode = currentNode;
                    photonMapDoScreenNEE(camera, sceneWorldVoxelGrid, config, context);
                }
            }
        }

        // Account for bsdf, node that for the first node, this accounts
        // for the emitted radiance.
        if ( !(currentNode->ends()) ) {
            accPower.selfScalarProduct(currentNode->m_bsdfEval);

            currentNode = currentNode->next();
            bp->m_lightSize++;
        } else {
            lDone = true;
        }
    }
}

static void
photonMapTracePath(
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    PhotonMapConfig *config,
    BSDF_FLAGS bsdfFlags) {
    config->biPath.m_eyePath = config->eyeConfig.tracePath(sceneVoxelGrid, sceneBackground, config->biPath.m_eyePath);

    // Use qmc for light sampling
    SimpleRaytracingPathNode *path = config->biPath.m_lightPath;

    // First node
    double x1 = drand48(); //nrs[0] * RECIP;
    double x2 = drand48(); //nrs[1] * RECIP;

    path = config->lightConfig.traceNode(sceneVoxelGrid, sceneBackground, path, x1, x2, bsdfFlags);
    if ( path == nullptr ) {
        return;
    }

    config->biPath.m_lightPath = path;  // In case no nodes were present

    path->ensureNext();

    // Second node
    SimpleRaytracingPathNode *node = path->next();
    x1 = drand48(); // nrs[2] * RECIP;
    x2 = drand48(); // nrs[3] * RECIP; // 4D Niederreiter...

    if ( config->lightConfig.traceNode(sceneVoxelGrid, sceneBackground, node, x1, x2, bsdfFlags) ) {
        // Successful trace
        node->ensureNext();
        config->lightConfig.tracePath(sceneVoxelGrid, sceneBackground, node->next(), bsdfFlags);
    }
}

static void
photonMapTracePaths(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    Background *sceneBackground,
    int numberOfPaths,
    BSDF_FLAGS bsdfFlags = BSDF_ALL_COMPONENTS,
    RadianceMethod *context = nullptr) {
    int i;

    // Fill in config structures
    for ( i = 0; i < numberOfPaths; i++ ) {
        photonMapTracePath(sceneWorldVoxelGrid, sceneBackground, &GLOBAL_photonMap_config, bsdfFlags);
        photonMapHandlePath(camera, sceneWorldVoxelGrid, &GLOBAL_photonMap_config, context);
    }
}

static void
photonMapBRRealIteration(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    Background *sceneBackground,
    RadianceMethod *context)
{
    GLOBAL_photonMap_state.iterationNumber++;

    fprintf(stderr, "GLOBAL_photonMapMethods Iteration %li\n", (long) GLOBAL_photonMap_state.iterationNumber);

    if ( (GLOBAL_photonMap_state.iterationNumber > 1) && (GLOBAL_photonMap_state.doGlobalMap || GLOBAL_photonMap_state.doCausticMap) ) {
        float scaleFactor = ((float)GLOBAL_photonMap_state.iterationNumber - 1.0f) / (float) GLOBAL_photonMap_state.iterationNumber;
        GLOBAL_photonMap_config.screen->scaleRadiance(scaleFactor);
    }

    if ( (GLOBAL_photonMap_state.densityControl == IMPORTANCE_RD) && GLOBAL_photonMap_state.doImportanceMap ) {
        GLOBAL_photonMap_state.i_iteration_nr++;
        GLOBAL_photonMap_config.currentMap = GLOBAL_photonMap_config.importanceMap;
        GLOBAL_photonMap_state.totalIPaths = GLOBAL_photonMap_state.i_iteration_nr * GLOBAL_photonMap_state.iPathsPerIteration;
        GLOBAL_photonMap_config.currentMap->SetTotalPaths(GLOBAL_photonMap_state.totalIPaths);
        GLOBAL_photonMap_config.importanceCMap->SetTotalPaths(GLOBAL_photonMap_state.totalIPaths);

        tracePotentialPaths(sceneWorldVoxelGrid, sceneBackground, (int)GLOBAL_photonMap_state.iPathsPerIteration);

        fprintf(stderr, "Total potential paths : %li, Total rays %li\n",
                GLOBAL_photonMap_state.totalIPaths,
                GLOBAL_raytracer_rayCount);
    }

    // Global map
    if ( GLOBAL_photonMap_state.doGlobalMap ) {
        GLOBAL_photonMap_state.gIterationNumber++;
        GLOBAL_photonMap_config.currentMap = GLOBAL_photonMap_config.globalMap;
        GLOBAL_photonMap_state.totalGPaths = GLOBAL_photonMap_state.gIterationNumber * GLOBAL_photonMap_state.gPathsPerIteration;
        GLOBAL_photonMap_config.currentMap->SetTotalPaths(GLOBAL_photonMap_state.totalGPaths);

        // Set correct importance map: indirect importance
        GLOBAL_photonMap_config.currentImpMap = GLOBAL_photonMap_config.importanceMap;

        photonMapTracePaths(
            camera,
            sceneWorldVoxelGrid,
            sceneBackground,
            (int)GLOBAL_photonMap_state.gPathsPerIteration,
            BSDF_ALL_COMPONENTS,
            context);

        fprintf(stderr, "Global map: ");
        GLOBAL_photonMap_config.globalMap->PrintStats(stderr);
    }

    // Caustic map
    if ( GLOBAL_photonMap_state.doCausticMap ) {
        GLOBAL_photonMap_state.cIterationNumber++;
        GLOBAL_photonMap_config.currentMap = GLOBAL_photonMap_config.causticMap;
        GLOBAL_photonMap_state.totalCPaths = GLOBAL_photonMap_state.cIterationNumber * GLOBAL_photonMap_state.cPathsPerIteration;
        GLOBAL_photonMap_config.currentMap->SetTotalPaths(GLOBAL_photonMap_state.totalCPaths);

        // Set correct importance map: direct importance
        GLOBAL_photonMap_config.currentImpMap = GLOBAL_photonMap_config.importanceCMap;

        photonMapTracePaths(
            camera,
            sceneWorldVoxelGrid,
            sceneBackground,
            (int)GLOBAL_photonMap_state.cPathsPerIteration,
            BSDF_SPECULAR_COMPONENT);

        fprintf(stderr, "Caustic map: ");
        GLOBAL_photonMap_config.causticMap->PrintStats(stderr);
    }
}

/**
Performs one step of the radiance computations. The goal most often is
to fill in a RGB color for display of each patch and/or vertex. These
colors are used for hardware rendering if the default hardware rendering
method is not updated in this file
*/
int
PhotonMapRadianceMethod::doStep(
    Camera *camera,
    Background *sceneBackground,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries,
    java::ArrayList<Patch *> *lightPatches,
    Geometry *clusteredWorldGeometry,
    VoxelGrid *sceneWorldVoxelGrid)
{
    GLOBAL_photonMap_state.lastClock = clock();

    photonMapBRRealIteration(camera, sceneWorldVoxelGrid, sceneBackground, this);
    photonMapRadiosityUpdateCpuSecs();

    GLOBAL_photonMap_state.runStopNumber++;

    return false; // Done. Return false if you want the computations to continue
}

/**
Undoes the effect of mainInit() and all side-effects of Step()
*/
void
PhotonMapRadianceMethod::terminate(java::ArrayList<Patch *> *scenePatches) {
    if ( GLOBAL_photonMap_config.screen ) {
        delete GLOBAL_photonMap_config.screen;
        GLOBAL_photonMap_config.screen = nullptr;
    }

    GLOBAL_photonMap_config.lightConfig.releaseVars();
    GLOBAL_photonMap_config.eyeConfig.releaseVars();

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
ColorRgb
photonMapGetNodeGRadiance(SimpleRaytracingPathNode *node) {
    ColorRgb col;

    GLOBAL_photonMap_config.globalMap->DoBalancing(GLOBAL_photonMap_state.balanceKDTree);
    col = GLOBAL_photonMap_config.globalMap->Reconstruct(&node->m_hit, node->m_inDirF,
                                                         node->m_useBsdf,
                                                         node->m_inBsdf, node->m_outBsdf);
    return col;
}

/**
Returns the radiance emitted in the node related direction
*/
ColorRgb
photonMapGetNodeCRadiance(SimpleRaytracingPathNode *node) {
    ColorRgb col;

    GLOBAL_photonMap_config.causticMap->DoBalancing(GLOBAL_photonMap_state.balanceKDTree);

    col = GLOBAL_photonMap_config.causticMap->Reconstruct(&node->m_hit, node->m_inDirF,
                                                          node->m_useBsdf,
                                                          node->m_inBsdf, node->m_outBsdf);
    return col;
}

ColorRgb
PhotonMapRadianceMethod::getRadiance(Patch *patch, double u, double v, Vector3D dir) {
    RayHit hit;
    Vector3D point;
    BSDF *bsdf = patch->material->bsdf;
    ColorRgb col;
    float density;

    patch->pointBarycentricMapping(u, v, &point);
    hit.init(patch, nullptr, &point, &patch->normal, patch->material, 0.0);
    hit.shadingNormal(&hit.normal);

    if ( zeroAlbedo(bsdf, &hit, BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT) ) {
        col.clear();
        return col;
    }

    RAD_RETURN_OPTION radiosityReturn = GLOBAL_RADIANCE;

    if ( globalDoingLocalRayCasting ) {
        radiosityReturn = GLOBAL_photonMap_state.radianceReturn;
    }

    switch ( radiosityReturn ) {
        case GLOBAL_DENSITY:
            col = GLOBAL_photonMap_config.globalMap->GetDensityColor(hit);
            break;
        case CAUSTIC_DENSITY:
            col = GLOBAL_photonMap_config.causticMap->GetDensityColor(hit);
            break;
        case IMPORTANCE_C_DENSITY:
            col = GLOBAL_photonMap_config.importanceCMap->GetDensityColor(hit);
            break;
        case IMPORTANCE_G_DENSITY:
            col = GLOBAL_photonMap_config.importanceMap->GetDensityColor(hit);
            break;
        case REC_C_DENSITY:
            GLOBAL_photonMap_config.importanceCMap->DoBalancing(GLOBAL_photonMap_state.balanceKDTree);
            density = GLOBAL_photonMap_config.importanceCMap->GetRequiredDensity(hit.point, hit.normal);
            col = getFalseColor(density);
            break;
        case REC_G_DENSITY:
            GLOBAL_photonMap_config.importanceMap->DoBalancing(GLOBAL_photonMap_state.balanceKDTree);
            density = GLOBAL_photonMap_config.importanceMap->GetRequiredDensity(hit.point, hit.normal);
            col = getFalseColor(density);
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
        default:
            col.clear();
            logError("photonMapGetRadiance", "Unknown radiance return");
    }

    return col;
}

void
PhotonMapRadianceMethod::renderScene(Camera *camera, java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry) {
    if ( GLOBAL_photonMap_config.screen && GLOBAL_photonMap_state.renderImage ) {
        GLOBAL_photonMap_config.screen->render();
    } else {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            openGlRenderPatch(scenePatches->get(i), camera);
        }
    }
}

char *
PhotonMapRadianceMethod::getStats() {
    static char stats[STRING_LENGTH];
    char *p;
    int n;

    p = stats;
    snprintf(p, STRING_LENGTH, "Photon map Statistics:\n\n%n", &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Ray count %li\n%n", GLOBAL_raytracer_rayCount, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Time %g\n%n", GLOBAL_photonMap_state.cpuSecs, &n);
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
