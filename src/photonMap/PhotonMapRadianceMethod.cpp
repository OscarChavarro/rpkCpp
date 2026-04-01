#include <cstdlib>

#include "java/util/Formatter.h"
#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "common/Error.h"
#include "common/statistics/Statistics.h"
#include "raycasting/common/Raytools.h"
#include "raycasting/raytracing/EyeSampler.h"
#include "raycasting/bidirectionalRaytracing/LightSampler.h"
#include "raycasting/bidirectionalRaytracing/LightDirSampler.h"
#include "photonMap/PhotonMapSampler.h"
#include "photonMap/ScreenSampler.h"
#include "photonMap/Photon.h"
#include "photonMap/PhotonMapRadianceMethod.h"
#include "photonMap/PhotonMapConfig.h"
#include "photonMap/PhotonMapImportance.h"

// To adjust photonMapGetRadiance returns
static bool globalDoingLocalRayCasting = false;

static constexpr int STRING_LENGTH = 1000;

void
PhotonMapRadianceMethod::appendStatsText(char *buffer, int *offset, const char *format, ...) {
    if ( *offset >= STRING_LENGTH - 1 ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    const int available = STRING_LENGTH - *offset;
    const int written = java::Formatter::vformat(&buffer[*offset], available, format, arguments);
    va_end(arguments);

    if ( written <= 0 ) {
        return;
    }
    if ( written >= available ) {
        *offset = STRING_LENGTH - 1;
    } else {
        *offset += written;
    }
}

PhotonMapRadianceMethod::PhotonMapRadianceMethod(
    PhotonMapState &inPhotonMapState,
    PhotonMapConfig &inPhotonMapConfig):
    photonMapState(inPhotonMapState),
    photonMapConfig(inPhotonMapConfig)
{
    photonMapState.setDefaults();
    className = PHOTON_MAP;
}

PhotonMapRadianceMethod::~PhotonMapRadianceMethod() {
}

const char *
PhotonMapRadianceMethod::getRadianceMethodName() const {
    return "Photon map";
}

void
PhotonMapRadianceMethod::parseOptions(int */*argc*/, char **/*argv*/) {
}

void
PhotonMapRadianceMethod::writeVRML(
    const Camera */*camera*/,
    java::OutputStream */*outputStream*/,
    const RenderOptions */*renderOptions*/) const
{
}

/**
For counting how much CPU time was used for the computations
*/
void
PhotonMapRadianceMethod::photonMapRadiosityUpdateCpuSecs() {
    const long long t = java::System::nanoTime();
    photonMapState.cpuSecs += static_cast<float>(
        static_cast<double>(t - photonMapState.lastClock) / 1000000000.0);
    photonMapState.lastClock = t;
}

Element *
PhotonMapRadianceMethod::createPatchData(Patch *patch) {
    return patch->radianceData = nullptr;
}

void
PhotonMapRadianceMethod::destroyPatchData(Patch *patch) {
    patch->radianceData = nullptr;
}

void
PhotonMapRadianceMethod::photonMapChooseSurfaceSampler(SurfaceSampler **samplerPtr) {
    if ( *samplerPtr != nullptr ) {
        delete *samplerPtr;
    }

    if ( photonMapState.usePhotonMapSampler ) {
        *samplerPtr = new PhotonMapSampler;
    } else {
        *samplerPtr = new BsdfSampler;
    }
}

/**
Initializes the computations for the current scene (if any)
*/
void
PhotonMapRadianceMethod::initialize(Scene *scene) {
    java::System::err.printf("Photon map activated\n");

    photonMapState.lastClock = java::System::nanoTime();
    photonMapState.cpuSecs = 0.0;
    photonMapState.gIterationNumber = 0;
    photonMapState.cIterationNumber = 0;
    photonMapState.i_iteration_nr = 0;
    photonMapState.iterationNumber = 0;
    photonMapState.runStopNumber = 0;
    photonMapState.totalGPaths = 0;
    photonMapState.totalCPaths = 0;
    photonMapState.totalIPaths = 0;

    if ( photonMapConfig.screen ) {
        delete photonMapConfig.screen;
    }
    photonMapConfig.screen = new ScreenBuffer(nullptr, scene->camera, scene->toneMapOptions);

    if ( photonMapConfig.lightList ) {
        delete photonMapConfig.lightList;
        photonMapConfig.lightList = nullptr;
    }
    photonMapConfig.lightList = new LightList(scene->lightSourcePatchList);

    // mainInitApplication samplers

    photonMapConfig.lightConfig.releaseVars();
    photonMapConfig.eyeConfig.releaseVars();

    SamplerConfig *cfg = &photonMapConfig.eyeConfig;

    cfg->pointSampler = new EyeSampler;

    cfg->dirSampler = new ScreenSampler;

    photonMapChooseSurfaceSampler(&cfg->surfaceSampler);
    cfg->surfaceSampler->SetComputeFromNextPdf(false);
    cfg->neSampler = nullptr;

    cfg->minDepth = 1;
    cfg->maxDepth = 1;  // Only eye point needed, for Particle tracing test

    cfg = &photonMapConfig.lightConfig;

    cfg->pointSampler = new UniformLightSampler(photonMapConfig.lightList);
    cfg->dirSampler = new LightDirSampler;
    photonMapChooseSurfaceSampler(&cfg->surfaceSampler);
    // cfg->surfaceSampler = new PhotonMapSampler; //new BsdfSampler;
    cfg->surfaceSampler->SetComputeFromNextPdf(false);  // Only 1 pdf

    cfg->minDepth = photonMapState.minimumLightPathDepth;
    cfg->maxDepth = photonMapState.maximumLightPathDepth;

    Statistics::instance().rayTracer.rayCount = 0;

    // mainInitApplication the photon map

    if ( photonMapConfig.globalMap ) {
        delete photonMapConfig.globalMap;
    }
    photonMapConfig.globalMap = new PhotonMap(
        photonMapState,
        &photonMapState.reconGPhotons,
        photonMapState.precomputeGIrradiance);

    if ( photonMapConfig.importanceMap ) {
        delete photonMapConfig.importanceMap;
    }
    photonMapConfig.importanceMap = new ImportanceMap(
        photonMapState,
        &photonMapState.reconIPhotons,
        &photonMapState.gImpScale);

    if ( photonMapConfig.importanceCMap ) {
        delete photonMapConfig.importanceCMap;
    }
    photonMapConfig.importanceCMap = new ImportanceMap(
        photonMapState,
        &photonMapState.reconIPhotons,
        &photonMapState.cImpScale);

    if ( photonMapConfig.causticMap ) {
        delete photonMapConfig.causticMap;
    }
    photonMapConfig.causticMap = new PhotonMap(photonMapState, &photonMapState.reconCPhotons);
}

/**
Adapted from bi-directional path, this is a bit overkill for here
*/
ColorRgb
PhotonMapRadianceMethod::photonMapDoComputePixelFluxEstimate(
    Camera *camera,
    PhotonMapConfig *config,
    const RadianceMethod * /*radianceMethod*/)
{
    BiPath *bp = &config->biPath;
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
            SamplerConfig::pathNodeConnect(camera, eyeEndNode, lightEndNode,
                            &config->eyeConfig, &config->lightConfig,
                            CONNECT_EL | CONNECT_LE,
                            BSDF_ALL_COMPONENTS, BSDF_ALL_COMPONENTS, &bp->m_dirEL);

    bp->m_dirLE.scaledCopy(-1, bp->m_dirEL);

    // Evaluate radiance and probabilityDensityFunction and weight
    f = bp->evalRadiance();

    float factor = 1.0f / static_cast<float>(bp->evalPdfAcc());

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
void
PhotonMapRadianceMethod::photonMapDoScreenNEE(
    Camera *camera,
    const VoxelGrid *sceneWorldVoxelGrid,
    PhotonMapConfig *config,
    const RadianceMethod *radianceMethod)
{
    int nx;
    int ny;
    float pixX;
    float pixY;
    ColorRgb f;
    const BiPath *bp = &config->biPath;

    if ( config->currentMap == config->importanceMap ) {
        return;
    }

    // First we need to determine if the lightEndNode can be seen from
    // the camera. At the same time the pixel hit is computed
    if ( RayTools::eyeNodeVisible(
            camera,
            sceneWorldVoxelGrid,
            bp->m_eyeEndNode,
            bp->m_lightEndNode,
            &pixX,
            &pixY) ) {
        // Visible !
        f = photonMapDoComputePixelFluxEstimate(camera, config, radianceMethod);

        config->screen->getPixel(pixX, pixY, &nx, &ny);

        float factor;

        if ( config->currentMap == config->globalMap ) {
            factor = (ScreenBuffer::computeFluxToRadFactor(camera, nx, ny)
                      / static_cast<float>(photonMapState.totalGPaths));
        } else {
            factor = (ScreenBuffer::computeFluxToRadFactor(camera, nx, ny)
                      / static_cast<float>(photonMapState.totalCPaths));
        }

        f.scale(factor);

        config->screen->add(nx, ny, f);
    }
}


/**
Store a photon. Some acceptance tests are performed first
*/
bool
PhotonMapRadianceMethod::photonMapDoPhotonStore(
    const Camera *camera,
    SimpleRaytracingPathNode *node,
    ColorRgb power)
{
    if ( node->m_hit.getPatch() && node->m_hit.getPatch()->material ) {
        // Only add photons on surfaces with a certain reflection
        // coefficient

        const PhongBidirectionalScatteringDistributionFunction *bsdf;
        bsdf = node->m_hit.getPatch()->material->getBsdf();

        if ( !PhotonMap::zeroAlbedo(bsdf, &node->m_hit, BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT) ) {
            Photon photon(node->m_hit.getPoint(), power, node->m_inDirF);

            // Determine photon flags
            short flags = 0;

            if ( node->m_depth == 1 ) {
                // Direct light photon
                flags |= DIRECT_LIGHT_PHOTON;
            }

            if ( photonMapState.densityControl == PhotonMapDensityControlOption::NO_DENSITY_CONTROL ) {
                return photonMapConfig.currentMap->addPhoton(photon, node->m_hit.getNormal(), flags);
            } else {
                float reqDensity;
                if ( photonMapState.densityControl == PhotonMapDensityControlOption::CONSTANT_RD ) {
                    reqDensity = photonMapState.constantRD;
                } else {
                    reqDensity = photonMapConfig.currentImpMap->getRequiredDensity(
                            camera,
                            node->m_hit.getPoint(),
                            node->m_hit.getNormal());
                }

                return photonMapConfig.currentMap->DC_AddPhoton(photon, node->m_hit, reqDensity, flags);
            }
        }
    }
    return false;
}

/**
Handle one path : store at all end positions and for testing, connect to the eye
*/
void
PhotonMapRadianceMethod::photonMapHandlePath(
    Camera *camera,
    const VoxelGrid *sceneWorldVoxelGrid,
    PhotonMapConfig *config,
    const RadianceMethod *radianceMethod)
{
    bool lDone;
    BiPath *bp = &config->biPath;
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
        factor = static_cast<float>(currentNode->m_G / currentNode->m_pdfFromPrev);
        accPower.scale(factor);

        // Store photon, but not emitted light
        if ( config->currentMap == config->globalMap ) {
            // Store
            if ( bp->m_lightSize > 1 && photonMapDoPhotonStore(camera, currentNode, accPower) ) {
                // Screen next event estimation for testing
                bp->m_lightEndNode = currentNode;
                photonMapDoScreenNEE(camera, sceneWorldVoxelGrid, config, radianceMethod);
            }
        } else {
            // Caustic map...
            // Store
            if ( bp->m_lightSize > 2 && photonMapDoPhotonStore(camera, currentNode, accPower) ) {
                // Screen next event estimation for testing

                bp->m_lightEndNode = currentNode;
                photonMapDoScreenNEE(camera, sceneWorldVoxelGrid, config, radianceMethod);
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

void
PhotonMapRadianceMethod::photonMapTracePath(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    PhotonMapConfig *config,
    char bsdfFlags) {
    config->biPath.m_eyePath = config->eyeConfig.tracePath(camera, sceneVoxelGrid, sceneBackground, config->biPath.m_eyePath);

    // Use qmc for light sampling
    SimpleRaytracingPathNode *path = config->biPath.m_lightPath;

    // First node
    double x1 = drand48(); // nrs[0] * RECIP
    double x2 = drand48(); // nrs[1] * RECIP

    path = config->lightConfig.traceNode(camera, sceneVoxelGrid, sceneBackground, path, x1, x2, bsdfFlags);
    if ( path == nullptr ) {
        return;
    }

    config->biPath.m_lightPath = path;  // In case no nodes were present

    path->ensureNext();

    // Second node
    SimpleRaytracingPathNode *node = path->next();
    x1 = drand48(); // nrs[2] * RECIP
    x2 = drand48(); // nrs[3] * RECIP // 4D Niederreiter...

    if ( config->lightConfig.traceNode(camera, sceneVoxelGrid, sceneBackground, node, x1, x2, bsdfFlags) ) {
        // Successful trace
        node->ensureNext();
        config->lightConfig.tracePath(camera, sceneVoxelGrid, sceneBackground, node->next(), bsdfFlags);
    }
}

void
PhotonMapRadianceMethod::photonMapTracePaths(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    Background *sceneBackground,
    PhotonMapConfig *config,
    int numberOfPaths,
    char bsdfFlags,
    const RadianceMethod *radianceMethod)
{
    // Fill in config structures
    for ( int i = 0; i < numberOfPaths; i++ ) {
        photonMapTracePath(camera, sceneWorldVoxelGrid, sceneBackground, config, bsdfFlags);
        photonMapHandlePath(camera, sceneWorldVoxelGrid, config, radianceMethod);
    }
}

void
PhotonMapRadianceMethod::photonMapBRRealIteration(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    Background *sceneBackground,
    const RadianceMethod *radianceMethod)
{
    photonMapState.iterationNumber++;

    java::System::err.printf("PhotonMapRadianceMethod Iteration %li\n", static_cast<long>(photonMapState.iterationNumber));

    if ( (photonMapState.iterationNumber > 1) && (photonMapState.doGlobalMap || photonMapState.doCausticMap) ) {
        float scaleFactor = (static_cast<float>(photonMapState.iterationNumber) - 1.0f) / static_cast<float>(photonMapState.iterationNumber);
        photonMapConfig.screen->scaleRadiance(scaleFactor);
    }

    if ( photonMapState.densityControl == PhotonMapDensityControlOption::IMPORTANCE_RD
      && photonMapState.doImportanceMap ) {
        photonMapState.i_iteration_nr++;
        photonMapConfig.currentMap = photonMapConfig.importanceMap;
        photonMapState.totalIPaths = photonMapState.i_iteration_nr * photonMapState.iPathsPerIteration;
        photonMapConfig.currentMap->setTotalPaths(photonMapState.totalIPaths);
        photonMapConfig.importanceCMap->setTotalPaths(photonMapState.totalIPaths);

        PhotonMapImportance::tracePotentialPaths(
            camera,
            sceneWorldVoxelGrid,
            sceneBackground,
            static_cast<int>(photonMapState.iPathsPerIteration),
            photonMapState,
            photonMapConfig);

        java::System::err.printf("Total potential paths : %li, Total rays %li\n",
                photonMapState.totalIPaths,
                Statistics::instance().rayTracer.rayCount);
    }

    // Global map
    if ( photonMapState.doGlobalMap ) {
        photonMapState.gIterationNumber++;
        photonMapConfig.currentMap = photonMapConfig.globalMap;
        photonMapState.totalGPaths = photonMapState.gIterationNumber * photonMapState.gPathsPerIteration;
        photonMapConfig.currentMap->setTotalPaths(photonMapState.totalGPaths);

        // Set correct importance map: indirect importance
        photonMapConfig.currentImpMap = photonMapConfig.importanceMap;

        photonMapTracePaths(
                camera,
                sceneWorldVoxelGrid,
                sceneBackground,
                &photonMapConfig,
                static_cast<int>(photonMapState.gPathsPerIteration),
                BSDF_ALL_COMPONENTS,
                radianceMethod);

        java::System::err.printf("Global map: ");
        photonMapConfig.globalMap->printStats(&java::System::err);
    }

    // Caustic map
    if ( photonMapState.doCausticMap ) {
        photonMapState.cIterationNumber++;
        photonMapConfig.currentMap = photonMapConfig.causticMap;
        photonMapState.totalCPaths = photonMapState.cIterationNumber * photonMapState.cPathsPerIteration;
        photonMapConfig.currentMap->setTotalPaths(photonMapState.totalCPaths);

        // Set correct importance map: direct importance
        photonMapConfig.currentImpMap = photonMapConfig.importanceCMap;

        photonMapTracePaths(
            camera,
            sceneWorldVoxelGrid,
            sceneBackground,
            &photonMapConfig,
            static_cast<int>(photonMapState.cPathsPerIteration),
            BSDF_SPECULAR_COMPONENT);

        java::System::err.printf("Caustic map: ");
        photonMapConfig.causticMap->printStats(&java::System::err);
    }
}

/**
Performs one step of the radiance computations. The goal most often is
to fill in a RGB color for display of each patch and/or vertex. These
colors are used for hardware rendering if the default hardware rendering
method is not updated in this file
*/
bool
PhotonMapRadianceMethod::doStep(Scene *scene, RenderOptions */*renderOptions*/) {
    photonMapState.lastClock = java::System::nanoTime();

    photonMapBRRealIteration(scene->camera, scene->voxelGrid, scene->background, this);
    photonMapRadiosityUpdateCpuSecs();

    photonMapState.runStopNumber++;

    return false; // Done. Return false if you want the computations to continue
}

/**
Undoes the effect of mainInitApplication() and all side-effects of Step()
*/
void
PhotonMapRadianceMethod::terminate(java::ArrayList<Patch *> */*scenePatches*/) {
    if ( photonMapConfig.screen ) {
        delete photonMapConfig.screen;
        photonMapConfig.screen = nullptr;
    }

    photonMapConfig.lightConfig.releaseVars();
    photonMapConfig.eyeConfig.releaseVars();

    if ( photonMapConfig.globalMap ) {
        delete photonMapConfig.globalMap;
        photonMapConfig.globalMap = nullptr;
    }

    if ( photonMapConfig.importanceMap ) {
        delete photonMapConfig.importanceMap;
        photonMapConfig.importanceMap = nullptr;
    }

    if ( photonMapConfig.importanceCMap ) {
        delete photonMapConfig.importanceCMap;
        photonMapConfig.importanceCMap = nullptr;
    }

    if ( photonMapConfig.causticMap ) {
        delete photonMapConfig.causticMap;
        photonMapConfig.causticMap = nullptr;
    }

    if ( photonMapConfig.lightList ) {
        delete photonMapConfig.lightList;
        photonMapConfig.lightList = nullptr;
    }
}

/**
Returns the radiance emitted in the node related direction
*/
ColorRgb
PhotonMapRadianceMethod::getNodeGRadiance(SimpleRaytracingPathNode *node) const {
    ColorRgb col;

    photonMapConfig.globalMap->doBalancing(photonMapState.balanceKDTree);
    col = photonMapConfig.globalMap->reconstruct(&node->m_hit, node->m_inDirF,
                                                         node->m_useBsdf,
                                                         node->m_inBsdf, node->m_outBsdf);
    return col;
}

/**
Returns the radiance emitted in the node related direction
*/
ColorRgb
PhotonMapRadianceMethod::getNodeCRadiance(SimpleRaytracingPathNode *node) const {
    ColorRgb col;

    photonMapConfig.causticMap->doBalancing(photonMapState.balanceKDTree);

    col = photonMapConfig.causticMap->reconstruct(&node->m_hit, node->m_inDirF,
                                                          node->m_useBsdf,
                                                          node->m_inBsdf, node->m_outBsdf);
    return col;
}

ColorRgb
PhotonMapRadianceMethod::getRadiance(
    Camera *camera,
    Patch *patch,
    double u,
    double v,
    Vector3D dir,
    const RenderOptions */*renderOptions*/) const
{
    RayHit hit;
    Vector3D point;
    PhongBidirectionalScatteringDistributionFunction *bsdf = patch->material->getBsdf();
    ColorRgb radiance;
    float density;

    patch->pointBarycentricMapping(u, v, &point);
    hit.init(patch, &point, &patch->normal, patch->material);
    Vector3D normal = hit.getNormal();
    hit.shadingNormal(&normal);
    hit.setNormal(&normal);

    if ( PhotonMap::zeroAlbedo(bsdf, &hit, BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT) ) {
        radiance.clear();
        return radiance;
    }

    RadiosityReturnOption radiosityReturn = RadiosityReturnOption::GLOBAL_RADIANCE;

    if ( globalDoingLocalRayCasting ) {
        radiosityReturn = photonMapState.radianceReturn;
    }

    switch ( radiosityReturn ) {
        case RadiosityReturnOption::GLOBAL_DENSITY:
            radiance = photonMapConfig.globalMap->getDensityColor(hit);
            break;
        case RadiosityReturnOption::CAUSTIC_DENSITY:
            radiance = photonMapConfig.causticMap->getDensityColor(hit);
            break;
        case RadiosityReturnOption::IMPORTANCE_C_DENSITY:
            radiance = photonMapConfig.importanceCMap->getDensityColor(hit);
            break;
        case RadiosityReturnOption::IMPORTANCE_G_DENSITY:
            radiance = photonMapConfig.importanceMap->getDensityColor(hit);
            break;
        case RadiosityReturnOption::REC_C_DENSITY:
            {
                Vector3D nn = hit.getNormal();
                photonMapConfig.importanceCMap->doBalancing(photonMapState.balanceKDTree);
                density = photonMapConfig.importanceCMap->getRequiredDensity(
                        camera, hit.getPoint(), nn);
                hit.setNormal(&nn);
                radiance = PhotonMap::getFalseColor(density, photonMapState);
            }
            break;
        case RadiosityReturnOption::REC_G_DENSITY:
            photonMapConfig.importanceMap->doBalancing(photonMapState.balanceKDTree);
            density = photonMapConfig.importanceMap->getRequiredDensity(
                    camera, hit.getPoint(), hit.getNormal());
            radiance = PhotonMap::getFalseColor(density, photonMapState);
            break;
        case RadiosityReturnOption::GLOBAL_RADIANCE:
            radiance = photonMapConfig.globalMap->reconstruct(
                    &hit, dir, bsdf, nullptr, bsdf);
            break;
        case RadiosityReturnOption::CAUSTIC_RADIANCE:
            radiance = photonMapConfig.causticMap->reconstruct(
                    &hit, dir, bsdf, nullptr, bsdf);
            break;
        default:
            radiance.clear();
            Error::error("photonMapGetRadiance", "Unknown radiance return");
    }

    return radiance;
}

char *
PhotonMapRadianceMethod::getStats() const {
    static char stats[STRING_LENGTH];
    int statsOffset = 0;

    PhotonMapRadianceMethod::appendStatsText(stats, &statsOffset, "Photon map Statistics:\n\n");
    PhotonMapRadianceMethod::appendStatsText(stats, &statsOffset, "Ray count %li\n", Statistics::instance().rayTracer.rayCount);
    PhotonMapRadianceMethod::appendStatsText(stats, &statsOffset, "Time %g\n", photonMapState.cpuSecs);

    if ( photonMapConfig.globalMap ) {
        PhotonMapRadianceMethod::appendStatsText(stats, &statsOffset, "Global Map: ");
        if ( statsOffset < STRING_LENGTH - 1 ) {
            photonMapConfig.globalMap->getStats(&stats[statsOffset], STRING_LENGTH - statsOffset);
            while ( statsOffset < STRING_LENGTH - 1 && stats[statsOffset] != '\0' ) {
                statsOffset++;
            }
        }
        PhotonMapRadianceMethod::appendStatsText(stats, &statsOffset, "\n");
    }
    if ( photonMapConfig.causticMap ) {
        PhotonMapRadianceMethod::appendStatsText(stats, &statsOffset, "Caustic Map: ");
        if ( statsOffset < STRING_LENGTH - 1 ) {
            photonMapConfig.causticMap->getStats(&stats[statsOffset], STRING_LENGTH - statsOffset);
            while ( statsOffset < STRING_LENGTH - 1 && stats[statsOffset] != '\0' ) {
                statsOffset++;
            }
        }
        PhotonMapRadianceMethod::appendStatsText(stats, &statsOffset, "\n");
    }
    if ( photonMapConfig.importanceMap ) {
        PhotonMapRadianceMethod::appendStatsText(stats, &statsOffset, "Global Importance Map: ");
        if ( statsOffset < STRING_LENGTH - 1 ) {
            photonMapConfig.importanceMap->getStats(&stats[statsOffset], STRING_LENGTH - statsOffset);
            while ( statsOffset < STRING_LENGTH - 1 && stats[statsOffset] != '\0' ) {
                statsOffset++;
            }
        }
        PhotonMapRadianceMethod::appendStatsText(stats, &statsOffset, "\n");
    }
    if ( photonMapConfig.importanceCMap ) {
        PhotonMapRadianceMethod::appendStatsText(stats, &statsOffset, "Caustic Importance Map: ");
        if ( statsOffset < STRING_LENGTH - 1 ) {
            photonMapConfig.importanceCMap->getStats(&stats[statsOffset], STRING_LENGTH - statsOffset);
            while ( statsOffset < STRING_LENGTH - 1 && stats[statsOffset] != '\0' ) {
                statsOffset++;
            }
        }
        PhotonMapRadianceMethod::appendStatsText(stats, &statsOffset, "\n");
    }

    return stats;
}

#endif
