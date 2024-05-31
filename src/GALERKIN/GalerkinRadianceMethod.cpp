/**
Galerkin radiosity, with the following variants:
- With or without hierarchical refinement
- With or without clusters
- With Jacobi, Gauss-Seidel or South well iterations
- With potential-driven or not
*/

#include <ctime>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/Statistics.h"
#include "io/writevrml.h"
#include "render/opengl.h"
#include "render/glutDebugTools.h"
#include "tonemap/ToneMap.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/processing/ScratchVisibilityStrategy.h"
#include "GALERKIN/processing/ShootingStrategy.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "GALERKIN/processing/GatheringSimpleStrategy.h"
#include "GALERKIN/processing/GatheringClusteredStrategy.h"
#include "GALERKIN/processing/ClusterCreationStrategy.h"

#define STRING_LENGTH 2000

GalerkinState GalerkinRadianceMethod::galerkinState;

// Used for VRML export
static FILE *globalVrmlFileDescriptor;
static int globalNumberOfWrites;
static int globalVertexId;

static void
galerkinWriteVertexCoord(const Vector3D *p) {
    if ( globalNumberOfWrites > 0 ) {
        fprintf(globalVrmlFileDescriptor, ", ");
    }
    globalNumberOfWrites++;
    if ( globalNumberOfWrites % 4 == 0 ) {
        fprintf(globalVrmlFileDescriptor, "\n\t  ");
    }
    fprintf(globalVrmlFileDescriptor, "%g %g %g", p->x, p->y, p->z);
    globalVertexId++;
}

static void
galerkinWriteVertexCoords(Element *element) {
    const GalerkinElement *galerkinElement = (GalerkinElement *)element;
    Vector3D v[8];
    int numberOfVertices = galerkinElement->vertices(v, 8);
    for ( int i = 0; i < numberOfVertices; i++ ) {
        galerkinWriteVertexCoord(&v[i]);
    }
}

static void
galerkinWriteCoords() {
    globalNumberOfWrites = globalVertexId = 0;
    fprintf(globalVrmlFileDescriptor, "\tcoord Coordinate {\n\t  point [ ");
    GalerkinRadianceMethod::galerkinState.topCluster->traverseAllLeafElements(galerkinWriteVertexCoords);
    fprintf(globalVrmlFileDescriptor, " ] ");
    fprintf(globalVrmlFileDescriptor, "\n\t}\n");
}

static void
galerkinWriteVertexColor(const ColorRgb *color) {
    if ( globalNumberOfWrites > 0 ) {
        fprintf(globalVrmlFileDescriptor, ", ");
    }
    globalNumberOfWrites++;
    if ( globalNumberOfWrites % 4 == 0 ) {
        fprintf(globalVrmlFileDescriptor, "\n\t  ");
    }
    fprintf(globalVrmlFileDescriptor, "%.3g %.3g %.3g", color->r, color->g, color->b);
    globalVertexId++;
}

static void
galerkinWriteVertexColors(Element *element) {
    const GalerkinElement *galerkinElement = (GalerkinElement *)element;
    ColorRgb vertexRadiosity[4];
    int i;

    if ( galerkinElement->patch->numberOfVertices == 3 ) {
        vertexRadiosity[0] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 0.0);
        vertexRadiosity[1] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 0.0);
        vertexRadiosity[2] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 1.0);
    } else {
        vertexRadiosity[0] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 0.0);
        vertexRadiosity[1] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 0.0);
        vertexRadiosity[2] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 1.0);
        vertexRadiosity[3] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 1.0);
    }

    if ( GalerkinRadianceMethod::galerkinState.useAmbientRadiance ) {
        ColorRgb reflectivity = galerkinElement->patch->radianceData->Rd;
        ColorRgb ambient;

        ambient.scalarProduct(reflectivity, GalerkinRadianceMethod::galerkinState.ambientRadiance);
        for ( i = 0; i < galerkinElement->patch->numberOfVertices; i++ ) {
            vertexRadiosity[i].add(vertexRadiosity[i], ambient);
        }
    }

    for ( i = 0; i < galerkinElement->patch->numberOfVertices; i++ ) {
        ColorRgb col{};
        radianceToRgb(vertexRadiosity[i], &col);
        galerkinWriteVertexColor(&col);
    }
}

static void
galerkinWriteVertexColorsTopCluster() {
    globalVertexId = globalNumberOfWrites = 0;
    fprintf(globalVrmlFileDescriptor, "\tcolor Color {\n\t  color [ ");
    GalerkinRadianceMethod::galerkinState.topCluster->traverseAllLeafElements(galerkinWriteVertexColors);
    fprintf(globalVrmlFileDescriptor, " ] ");
    fprintf(globalVrmlFileDescriptor, "\n\t}\n");
}

static void
galerkinWriteColors(const RenderOptions *renderOptions) {
    if ( !renderOptions->smoothShading ) {
        logWarning(nullptr, "I assume you want a smooth shaded model ...");
    }
    fprintf(globalVrmlFileDescriptor, "\tcolorPerVertex %s\n", "TRUE");
    galerkinWriteVertexColorsTopCluster();
}

static void
galerkinWriteCoordIndex(int index) {
    globalNumberOfWrites++;
    if ( globalNumberOfWrites % 20 == 0 ) {
        fprintf(globalVrmlFileDescriptor, "\n\t  ");
    }
    fprintf(globalVrmlFileDescriptor, "%d ", index);
}

static void
galerkinWriteCoordIndices(Element *element) {
    const GalerkinElement *galerkinElement = (GalerkinElement *)element;
    for ( int i = 0; i < galerkinElement->patch->numberOfVertices; i++ ) {
        galerkinWriteCoordIndex(globalVertexId);
        globalVertexId++;
    }
    galerkinWriteCoordIndex(-1);
}

static void
galerkinWriteCoordIndicesTopCluster() {
    globalVertexId = globalNumberOfWrites = 0;
    fprintf(globalVrmlFileDescriptor, "\tcoordIndex [ ");
    GalerkinRadianceMethod::galerkinState.topCluster->traverseAllLeafElements(galerkinWriteCoordIndices);
    fprintf(globalVrmlFileDescriptor, " ]\n");
}

void
galerkinFreeMemory() {
    if ( GalerkinRadianceMethod::galerkinState.scratch != nullptr ) {
        delete GalerkinRadianceMethod::galerkinState.scratch;
        GalerkinRadianceMethod::galerkinState.scratch = nullptr;
    }
}

GalerkinRadianceMethod::GalerkinRadianceMethod() {
    className = GALERKIN;
    gatheringStrategy = nullptr;
}

GalerkinRadianceMethod::~GalerkinRadianceMethod() {
    if ( galerkinState.topCluster != nullptr ) {
        galerkinState.topCluster = nullptr;
    }
    if ( gatheringStrategy != nullptr ) {
        delete gatheringStrategy;
        gatheringStrategy = nullptr;
    }
}

void
GalerkinRadianceMethod::renderElementHierarchy(const GalerkinElement *element, const RenderOptions *renderOptions) {
    if ( element->regularSubElements == nullptr ) {
        element->render(renderOptions);
    } else {
        for ( int i = 0; i < 4; i++ ) {
            renderElementHierarchy((GalerkinElement *)element->regularSubElements[i], renderOptions);
        }
    }
}

void
GalerkinRadianceMethod::galerkinRenderPatch(const Patch *patch, const Camera * /*camera*/, const RenderOptions *renderOptions) {
    renderElementHierarchy(galerkinGetElement(patch), renderOptions);
}

/**
For counting how much CPU time was used for the computations
*/
void
GalerkinRadianceMethod::updateCpuSecs() {
    clock_t t = clock();
    galerkinState.cpuSeconds += (float) (t - galerkinState.lastClock) / (float) CLOCKS_PER_SEC;
    galerkinState.lastClock = t;
}

void
GalerkinRadianceMethod::patchInit(Patch *patch) {
    ColorRgb reflectivity = patch->radianceData->Rd;
    ColorRgb selfEmittanceRadiance = patch->radianceData->Ed;

    if ( galerkinState.useConstantRadiance ) {
        // See Neumann et-al, "The Constant Radiosity Step", Euro-graphics Rendering Workshop
        // '95, Dublin, Ireland, June 1995, p 336-344
        galerkinGetRadiance(patch).scalarProduct(reflectivity, galerkinState.constantRadiance);
        galerkinGetRadiance(patch).add(galerkinGetRadiance(patch), selfEmittanceRadiance);
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
            patch->radianceData->unShotRadiance[0].subtract(galerkinGetRadiance(patch), galerkinState.constantRadiance);
        }
    } else {
        galerkinSetRadiance(patch, selfEmittanceRadiance);
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
            patch->radianceData->unShotRadiance[0] = galerkinGetRadiance(patch);
        }
    }

    if ( galerkinState.importanceDriven ) {
        switch ( galerkinState.galerkinIterationMethod ) {
            case GalerkinIterationMethod::GAUSS_SEIDEL:
            case GalerkinIterationMethod::JACOBI:
                galerkinSetPotential(patch, patch->directPotential);
                break;
            case GalerkinIterationMethod::SOUTH_WELL:
                galerkinSetPotential(patch, patch->directPotential);
                galerkinSetUnShotPotential(patch, patch->directPotential);
                break;
            default:
                logFatal(-1, "patchInit", "Invalid iteration method");
        }
    }

    recomputePatchColor(patch);
}

/**
Recomputes the color of a patch using ambient radiance term, ... if requested for
*/
void
GalerkinRadianceMethod::recomputePatchColor(Patch *patch) {
    ColorRgb reflectivity = patch->radianceData->Rd;
    ColorRgb radVis;

    // Compute the patches color based on its radiance + ambient radiance if desired
    if ( galerkinState.useAmbientRadiance ) {
        radVis.scalarProduct(reflectivity, galerkinState.ambientRadiance);
        radVis.add(radVis, galerkinGetRadiance(patch));
        radianceToRgb(radVis, &patch->color);
    } else {
        radianceToRgb(galerkinGetRadiance(patch), &patch->color);
    }
    patch->computeVertexColors();
}

const char *
GalerkinRadianceMethod::getRadianceMethodName() const  {
    return "Galerkin";
}

void
GalerkinRadianceMethod::setStrategy() {
    if ( galerkinState.clustered ) {
        gatheringStrategy = new GatheringClusteredStrategy();
    } else {
        gatheringStrategy = new GatheringSimpleStrategy();
    }
}

void
GalerkinRadianceMethod::parseOptions(int *argc, char **argv) {
}

void
GalerkinRadianceMethod::initialize(Scene *scene) {
    galerkinState.iterationNumber = 0;
    galerkinState.cpuSeconds = 0.0;

    basisGalerkinInitBasis();

    galerkinState.constantRadiance = GLOBAL_statistics.estimatedAverageRadiance;
    if ( galerkinState.useConstantRadiance ) {
        galerkinState.ambientRadiance.clear();
    } else {
        galerkinState.ambientRadiance = GLOBAL_statistics.estimatedAverageRadiance;
    }

    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        patchInit(scene->patchList->get(i));
    }

    galerkinState.topCluster = ClusterCreationStrategy::createClusterHierarchy(scene->clusteredRootGeometry,
                                                                               &galerkinState);

    // Create a scratch software renderer for various operations on clusters
    if ( galerkinState.clusteringStrategy == GalerkinClusteringStrategy::Z_VISIBILITY ) {
        ScratchVisibilityStrategy::scratchInit(&galerkinState);
    }

    // Global variables for scratch rendering
    galerkinState.lastClusterId = -1;
    galerkinState.lastEye.set(Numeric::HUGE_FLOAT_VALUE, Numeric::HUGE_FLOAT_VALUE, Numeric::HUGE_FLOAT_VALUE);
}

bool
GalerkinRadianceMethod::doStep(Scene *scene, RenderOptions *renderOptions) {
    if ( galerkinState.iterationNumber < 0 ) {
        logError("doGalerkinOneStep", "method not initialized");
        return true; // Done, don't continue!
    }

    galerkinState.iterationNumber++;
    galerkinState.lastClock = clock();

    // And now the real work
    int done;

    switch ( galerkinState.galerkinIterationMethod ) {
        case GalerkinIterationMethod::JACOBI:
        case GalerkinIterationMethod::GAUSS_SEIDEL:
            done = gatheringStrategy->doGatheringIteration(scene, &galerkinState, renderOptions);
            break;
        case GalerkinIterationMethod::SOUTH_WELL:
            done = ShootingStrategy::doShootingStep(scene, &galerkinState, renderOptions);
            break;
        default:
            logFatal(2, "doGalerkinOneStep", "Invalid iteration method %d\n", galerkinState.galerkinIterationMethod);
    }

    updateCpuSecs();

    return done;
}

/**
Disposes of the cluster hierarchy
*/
void
GalerkinRadianceMethod::galerkinDestroyClusterHierarchy(GalerkinElement *clusterElement) {
    if ( !clusterElement || !clusterElement->isCluster() ) {
        return;
    }

    for ( int i = 0; clusterElement->irregularSubElements != nullptr && i < clusterElement->irregularSubElements->size(); i++ ) {
        GalerkinRadianceMethod::galerkinDestroyClusterHierarchy((GalerkinElement *)clusterElement->irregularSubElements->get(i));
    }
    delete clusterElement;
}

void
GalerkinRadianceMethod::terminate(java::ArrayList<Patch *> *scenePatches) {
    if ( galerkinState.clusteringStrategy == GalerkinClusteringStrategy::Z_VISIBILITY ) {
        ScratchVisibilityStrategy::scratchTerminate(&galerkinState);
    }
    if ( galerkinState.topCluster != nullptr ) {
        GalerkinRadianceMethod::galerkinDestroyClusterHierarchy(galerkinState.topCluster);
        delete galerkinState.topCluster;
        galerkinState.topCluster = nullptr;
    }
}

ColorRgb
GalerkinRadianceMethod::getRadiance(
    Camera *camera,
    Patch *patch,
    double u,
    double v,
    Vector3D dir,
    const RenderOptions *renderOptions) const
{
    const GalerkinElement *leaf;
    ColorRgb rad;

    if ( patch->jacobian ) {
        patch->biLinearToUniform(&u, &v);
    }

    GalerkinElement *topLevelElement = galerkinGetElement(patch);
    leaf = topLevelElement->regularLeafAtPoint(&u, &v);

    rad = basisGalerkinRadianceAtPoint(leaf, leaf->radiance, u, v);

    if ( galerkinState.useAmbientRadiance ) {
        // Add ambient radiance
        ColorRgb reflectivity = patch->radianceData->Rd;
        ColorRgb ambientRadiance;
        ambientRadiance.scalarProduct(reflectivity, galerkinState.ambientRadiance);
        rad.add(rad, ambientRadiance);
    }

    return rad;
}

/**
Radiance data for a Patch is a surface element
*/
Element *
GalerkinRadianceMethod::createPatchData(Patch *patch) {
    return patch->radianceData = new GalerkinElement(patch, &galerkinState);
}

void
GalerkinRadianceMethod::destroyPatchData(Patch *patch) {
    if ( patch == nullptr ) {
        return;
    }
    if ( patch->radianceData != nullptr ) {
        delete ((GalerkinElement *)patch->radianceData);
        patch->radianceData = nullptr;
    }
}

char *
GalerkinRadianceMethod::getStats() {
    static char stats[STRING_LENGTH]{};

    for ( int i = 0 ; i < STRING_LENGTH; i++ ) {
        stats[i] = '\0';
    }

    int n;

    char *p = stats;
    snprintf(p, STRING_LENGTH, "Galerkin Radiosity Statistics:\n\n%n", &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Iteration: %d\n\n%n", galerkinState.iterationNumber, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Nr. elements: %d\n%n", GalerkinElement::getNumberOfElements(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "clusters: %d\n%n", GalerkinElement::getNumberOfClusters(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "surface elements: %d\n\n%n", GalerkinElement::getNumberOfSurfaceElements(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Nr. interactions: %d\n%n", Interaction::getNumberOfInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "cluster to cluster: %d\n%n", Interaction::getNumberOfClusterToClusterInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "cluster to surface: %d\n%n", Interaction::getNumberOfClusterToSurfaceInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "surface to cluster: %d\n%n", Interaction::getNumberOfSurfaceToClusterInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "surface to surface: %d\n%n", Interaction::getNumberOfSurfaceToSurfaceInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "shadow hits: %d\n%n", GLOBAL_statistics.numberOfShadowRays, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "shadow hits cached: %d\n%n", GLOBAL_statistics.numberOfShadowCacheHits, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "CPU time: %g secs.\n%n", galerkinState.cpuSeconds, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Minimum element area: %g m^2\n%n", GLOBAL_statistics.totalArea * (double) galerkinState.relMinElemArea, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Link error threshold: %g %s\n\n%n",
         (galerkinState.errorNorm == RADIANCE_ERROR ?
                   M_PI * (galerkinState.relLinkErrorThreshold *
                           GLOBAL_statistics.maxSelfEmittedRadiance.luminance()) :
                   galerkinState.relLinkErrorThreshold *
                   GLOBAL_statistics.maxSelfEmittedPower.luminance()),
         (galerkinState.errorNorm == RADIANCE_ERROR ? "lux" : "lumen"),
         &n);

    return stats;
}

void
GalerkinRadianceMethod::renderScene(const Scene *scene, const RenderOptions *renderOptions) const {
    if ( renderOptions->frustumCulling ) {
        openGlRenderWorldOctree(scene, galerkinRenderPatch, renderOptions);
    } else {
        RenderOptions modifiedRenderOptions = *renderOptions;
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            if ( GLOBAL_render_glutDebugState.showSelectedPathOnly ) {
                if ( i == GLOBAL_render_glutDebugState.selectedPatch ) {
                    modifiedRenderOptions.drawOutlines = true;
                    modifiedRenderOptions.outlineColor = ColorRgb(1.0f, 0.0f, 0.0f);
                } else {
                    modifiedRenderOptions.drawOutlines = false;
                }
                galerkinRenderPatch(scene->patchList->get(i), scene->camera, &modifiedRenderOptions);
            } else {
                modifiedRenderOptions.outlineColor = ColorRgb(0.4f, 0.1f, 0.1f);
                if ( i == GLOBAL_render_glutDebugState.selectedPatch ) {
                    modifiedRenderOptions.outlineColor = ColorRgb(0.0f, 0.0f, 1.0f);
                }
                galerkinRenderPatch(scene->patchList->get(i), scene->camera, &modifiedRenderOptions);
            }
        }
    }
}

void
GalerkinRadianceMethod::writeVRML(const Camera *camera, FILE *fp, const RenderOptions *renderOptions) const {
    writeVrmlHeader(camera, fp, renderOptions);

    globalVrmlFileDescriptor = fp;
    galerkinWriteCoords();
    galerkinWriteColors(renderOptions);
    galerkinWriteCoordIndicesTopCluster();

    writeVRMLTrailer(fp);
}
