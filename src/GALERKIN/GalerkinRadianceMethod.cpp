/**
Galerkin radiosity, with the following variants:
- With or without hierarchical refinement
- With or without clusters
- With Jacobi, Gauss-Seidel or South well iterations
- With potential-driven or not
*/

#include <cstring>
#include <ctime>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/mymath.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "skin/Vertex.h"
#include "render/render.h"
#include "scene/Camera.h"
#include "common/options.h"
#include "io/writevrml.h"
#include "render/opengl.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/scratch.h"
#include "GALERKIN/gathering.h"
#include "GALERKIN/shooting.h"
#include "GALERKIN/GalerkinRadianceMethod.h"

static GalerkinState globalGalerkinState;
static FILE *globalVrmlFileDescriptor;
static int globalNumberOfWrites;
static int globalVertexId;
static int globalTrue = true;
static int globalFalse = false;

#define STRING_LENGTH 2000

static inline ColorRgb
galerkinGetRadiance(Patch *patch) {
    return ((GalerkinElement *)(patch->radianceData))->radiance[0];
}

static inline void
galerkinSetRadiance(Patch *patch, ColorRgb value) {
    ((GalerkinElement *)(patch->radianceData))->radiance[0] = value;
}

static inline void
galerkinSetPotential(Patch *patch, float value) {
    ((GalerkinElement *)((patch)->radianceData))->potential = value;
}

static inline void
galerkinSetUnShotPotential(Patch *patch, float value) {
    ((GalerkinElement *)((patch)->radianceData))->unShotPotential = value;
}

static void
iterationMethodOption(void *value) {
    char *name = *(char **) value;

    if ( strncasecmp(name, "jacobi", 2) == 0 ) {
        globalGalerkinState.iteration_method = JACOBI;
    } else if ( strncasecmp(name, "gaussseidel", 2) == 0 ) {
        globalGalerkinState.iteration_method = GAUSS_SEIDEL;
    } else if ( strncasecmp(name, "southwell", 2) == 0 ) {
        globalGalerkinState.iteration_method = SOUTH_WELL;
    } else {
        logError(nullptr, "Invalid iteration method '%s'", name);
    }
}

static void
hierarchicalOption(void *value) {
    int yesno = *(int *) value;
    globalGalerkinState.hierarchical = yesno;
}

static void
lazyOption(void *value) {
    int yesno = *(int *) value;
    globalGalerkinState.lazy_linking = yesno;
}

static void
clusteringOption(void *value) {
    int yesno = *(int *) value;
    globalGalerkinState.clustered = yesno;
}

static void
importanceOption(void *value) {
    int yesno = *(int *) value;
    globalGalerkinState.importance_driven = yesno;
}

static void
ambientOption(void *value) {
    int yesno = *(int *) value;
    globalGalerkinState.use_ambient_radiance = yesno;
}

static CommandLineOptionDescription galerkinOptions[] = {
    {"-gr-iteration-method", 6, Tstring, nullptr, iterationMethodOption,
    "-gr-iteration-method <methodname>: Jacobi, GaussSeidel, Southwell"},
    {"-gr-hierarchical", 6, TYPELESS, (void *) &globalTrue, hierarchicalOption,
    "-gr-hierarchical    \t: do hierarchical refinement"},
    {"-gr-not-hierarchical", 10, TYPELESS, (void *) &globalFalse, hierarchicalOption,
    "-gr-not-hierarchical\t: don't do hierarchical refinement"},
    {"-gr-lazy-linking", 6, TYPELESS, (void *) &globalTrue, lazyOption,
    "-gr-lazy-linking    \t: do lazy linking"},
    {"-gr-no-lazy-linking", 10, TYPELESS, (void *) &globalFalse, lazyOption,
    "-gr-no-lazy-linking \t: don't do lazy linking"},
    {"-gr-clustering", 6, TYPELESS, (void *) &globalTrue, clusteringOption,
    "-gr-clustering      \t: do clustering"},
    {"-gr-no-clustering", 10, TYPELESS, (void *) &globalFalse, clusteringOption,
    "-gr-no-clustering   \t: don't do clustering"},
    {"-gr-importance", 6, TYPELESS, (void *) &globalTrue, importanceOption,
    "-gr-importance      \t: do view-potential driven computations"},
    {"-gr-no-importance", 10, TYPELESS, (void *) &globalFalse, importanceOption,
    "-gr-no-importance   \t: don't use view-potential"},
    {"-gr-ambient", 6, TYPELESS, (void *) &globalTrue, ambientOption,
    "-gr-ambient         \t: do visualisation with ambient term"},
    {"-gr-no-ambient", 10, TYPELESS, (void *) &globalFalse, ambientOption,
    "-gr-no-ambient      \t: do visualisation without ambient term"},
    {"-gr-link-error-threshold", 6, Tfloat, &globalGalerkinState.relLinkErrorThreshold, DEFAULT_ACTION,
    "-gr-link-error-threshold <float>: Relative link error threshold"},
    {"-gr-min-elem-area",6, Tfloat, &globalGalerkinState.relMinElemArea, DEFAULT_ACTION,
    "-gr-min-elem-area <float> \t: Relative element area threshold"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

/**
For counting how much CPU time was used for the computations
*/
static void
updateCpuSecs() {
    clock_t t;

    t = clock();
    globalGalerkinState.cpu_secs += (float) (t - globalGalerkinState.lastClock) / (float) CLOCKS_PER_SEC;
    globalGalerkinState.lastClock = t;
}

void
GalerkinRadianceMethod::patchInit(Patch *patch) {
    ColorRgb reflectivity = patch->radianceData->Rd;
    ColorRgb selfEmittanceRadiance = patch->radianceData->Ed;

    if ( globalGalerkinState.use_constant_radiance ) {
        // See Neumann et-al, "The Constant Radiosity Step", Euro-graphics Rendering Workshop
        // '95, Dublin, Ireland, June 1995, p 336-344
        galerkinGetRadiance(patch).scalarProduct(reflectivity, globalGalerkinState.constant_radiance);
        galerkinGetRadiance(patch).add(galerkinGetRadiance(patch), selfEmittanceRadiance);
        if ( globalGalerkinState.iteration_method == SOUTH_WELL )
            patch->radianceData->unShotRadiance[0].subtract(galerkinGetRadiance(patch), globalGalerkinState.constant_radiance);
    } else {
        galerkinSetRadiance(patch, selfEmittanceRadiance);
        if ( globalGalerkinState.iteration_method == SOUTH_WELL )
            patch->radianceData->unShotRadiance[0] = galerkinGetRadiance(patch);
    }

    if ( globalGalerkinState.importance_driven ) {
        switch ( globalGalerkinState.iteration_method ) {
            case GAUSS_SEIDEL:
            case JACOBI:
                galerkinSetPotential(patch, patch->directPotential);
                break;
            case SOUTH_WELL:
                galerkinSetPotential(patch, patch->directPotential);
                galerkinSetUnShotPotential(patch, patch->directPotential);
                break;
            default:
                logFatal(-1, "patchInit", "Invalid iteration method");
        }
    }

    recomputePatchColor(patch);
}

static void
renderElementHierarchy(GalerkinElement *element) {
    if ( !element->regularSubElements ) {
        element->render();
    } else if ( element->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            renderElementHierarchy((GalerkinElement *)element->regularSubElements[i]);
        }
    }
}

static void
galerkinRenderPatch(Patch *patch) {
    GalerkinElement *topLevelElement = galerkinGetElement(patch);
    renderElementHierarchy(topLevelElement);
}

static void
galerkinWriteVertexCoord(Vector3D *p) {
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
    GalerkinElement *galerkinElement = (GalerkinElement *)element;
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
    globalGalerkinState.topCluster->traverseAllLeafElements(galerkinWriteVertexCoords);
    fprintf(globalVrmlFileDescriptor, " ] ");
    fprintf(globalVrmlFileDescriptor, "\n\t}\n");
}

static void
galerkinWriteVertexColor(ColorRgb *color) {
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
    GalerkinElement *galerkinElement = (GalerkinElement *)element;
    ColorRgb vertexRadiosity[4];
    int i;

    if ( galerkinElement->patch->numberOfVertices == 3 ) {
        vertexRadiosity[0] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 0.0, &globalGalerkinState);
        vertexRadiosity[1] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 0.0, &globalGalerkinState);
        vertexRadiosity[2] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 1.0, &globalGalerkinState);
    } else {
        vertexRadiosity[0] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 0.0, &globalGalerkinState);
        vertexRadiosity[1] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 0.0, &globalGalerkinState);
        vertexRadiosity[2] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 1.0, &globalGalerkinState);
        vertexRadiosity[3] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 1.0, &globalGalerkinState);
    }

    if ( globalGalerkinState.use_ambient_radiance ) {
        ColorRgb reflectivity = galerkinElement->patch->radianceData->Rd;
        ColorRgb ambient;

        ambient.scalarProduct(reflectivity, globalGalerkinState.ambient_radiance);
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
    globalGalerkinState.topCluster->traverseAllLeafElements(galerkinWriteVertexColors);
    fprintf(globalVrmlFileDescriptor, " ] ");
    fprintf(globalVrmlFileDescriptor, "\n\t}\n");
}

static void
galerkinWriteColors() {
    if ( !GLOBAL_render_renderOptions.smoothShading ) {
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
    GalerkinElement *galerkinElement = (GalerkinElement *)element;
    for ( int i = 0; i < galerkinElement->patch->numberOfVertices; i++ ) {
        galerkinWriteCoordIndex(globalVertexId++);
    }
    galerkinWriteCoordIndex(-1);
}

static void
galerkinWriteCoordIndicesTopCluster() {
    globalVertexId = globalNumberOfWrites = 0;
    fprintf(globalVrmlFileDescriptor, "\tcoordIndex [ ");
    globalGalerkinState.topCluster->traverseAllLeafElements(galerkinWriteCoordIndices);
    fprintf(globalVrmlFileDescriptor, " ]\n");
}

/**
Recomputes the color of a patch using ambient radiance term, ... if requested for
*/
void
GalerkinRadianceMethod::recomputePatchColor(Patch *patch) {
    ColorRgb reflectivity = patch->radianceData->Rd;
    ColorRgb radVis;

    // Compute the patches color based on its radiance + ambient radiance if desired
    if ( globalGalerkinState.use_ambient_radiance ) {
        radVis.scalarProduct(reflectivity, globalGalerkinState.ambient_radiance);
        radVis.add(radVis, galerkinGetRadiance(patch));
        radianceToRgb(radVis, &patch->color);
    } else {
        radianceToRgb(galerkinGetRadiance(patch), &patch->color);
    }
    patch->computeVertexColors();
}

void
galerkinFreeMemory() {
    if ( globalGalerkinState.scratch != nullptr ) {
        delete globalGalerkinState.scratch;
        globalGalerkinState.scratch = nullptr;
    }
}

GalerkinRadianceMethod::GalerkinRadianceMethod() {
    className = GALERKIN;
}

GalerkinRadianceMethod::~GalerkinRadianceMethod() {
    if ( globalGalerkinState.topCluster != nullptr ) {
        delete globalGalerkinState.topCluster;
        globalGalerkinState.topCluster = nullptr;
    }
}

const char *
GalerkinRadianceMethod::getRadianceMethodName() const  {
    return "Galerkin";
}

void
GalerkinRadianceMethod::parseOptions(int *argc, char **argv) {
    parseGeneralOptions(galerkinOptions, argc, argv);
}

void
GalerkinRadianceMethod::initialize(java::ArrayList<Patch *> *scenePatches) {
    globalGalerkinState.iteration_nr = 0;
    globalGalerkinState.cpu_secs = 0.0;

    basisGalerkinInitBasis();

    globalGalerkinState.constant_radiance = GLOBAL_statistics.estimatedAverageRadiance;
    if ( globalGalerkinState.use_constant_radiance ) {
        globalGalerkinState.ambient_radiance.clear();
    } else {
        globalGalerkinState.ambient_radiance = GLOBAL_statistics.estimatedAverageRadiance;
    }

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        patchInit(scenePatches->get(i));
    }

    globalGalerkinState.topGeometry = GLOBAL_scene_clusteredWorldGeom;
    globalGalerkinState.topCluster = galerkinCreateClusterHierarchy(globalGalerkinState.topGeometry, &globalGalerkinState);

    // Create a scratch software renderer for various operations on clusters
    scratchInit(&globalGalerkinState);

    // Global variables used for form factor computation optimisation
    globalGalerkinState.formFactorLastRcv = nullptr;
    globalGalerkinState.formFactorLastSrc = nullptr;

    // Global variables for scratch rendering
    globalGalerkinState.lastClusterId = -1;
    globalGalerkinState.lastEye.set(HUGE, HUGE, HUGE);
}

int
GalerkinRadianceMethod::doStep(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    int done = false;

    if ( globalGalerkinState.iteration_nr < 0 ) {
        logError("doGalerkinOneStep", "method not initialized");
        return true;    /* done, don't continue! */
    }

    globalGalerkinState.iteration_nr++;
    globalGalerkinState.lastClock = clock();

    // And now the real work
    switch ( globalGalerkinState.iteration_method ) {
        case JACOBI:
        case GAUSS_SEIDEL:
            if ( globalGalerkinState.clustered ) {
                done = doClusteredGatheringIteration(scenePatches, &globalGalerkinState, this);
            } else {
                done = galerkinRadiosityDoGatheringIteration(scenePatches, &globalGalerkinState, this);
            }
            break;
        case SOUTH_WELL:
            done = doShootingStep(scenePatches, &globalGalerkinState, this);
            break;
        default:
            logFatal(2, "doGalerkinOneStep", "Invalid iteration method %d\n", globalGalerkinState.iteration_method);
    }

    updateCpuSecs();

    return done;
}

void
GalerkinRadianceMethod::terminate(java::ArrayList<Patch *> *scenePatches) {
    scratchTerminate(&globalGalerkinState);
    if ( globalGalerkinState.topCluster != nullptr ) {
        galerkinDestroyClusterHierarchy(globalGalerkinState.topCluster);
        globalGalerkinState.topCluster = nullptr;
    }
}

ColorRgb
GalerkinRadianceMethod::getRadiance(Patch *patch, double u, double v, Vector3D dir) {
    GalerkinElement *leaf;
    ColorRgb rad;

    if ( patch->jacobian ) {
        patch->biLinearToUniform(&u, &v);
    }

    GalerkinElement *topLevelElement = galerkinGetElement(patch);
    leaf = topLevelElement->regularLeafAtPoint(&u, &v);

    rad = basisGalerkinRadianceAtPoint(leaf, leaf->radiance, u, v, &globalGalerkinState);

    if ( globalGalerkinState.use_ambient_radiance ) {
        // Add ambient radiance
        ColorRgb reflectivity = patch->radianceData->Rd;
        ColorRgb ambientRadiance;
        ambientRadiance.scalarProduct(reflectivity, globalGalerkinState.ambient_radiance);
        rad.add(rad, ambientRadiance);
    }

    return rad;
}

/**
Radiance data for a Patch is a surface element
*/
Element *
GalerkinRadianceMethod::createPatchData(Patch *patch) {
    return patch->radianceData = new GalerkinElement(patch, &globalGalerkinState);
}

void
GalerkinRadianceMethod::destroyPatchData(Patch *patch) {
    delete ((GalerkinElement *)patch->radianceData);
    patch->radianceData = nullptr;
}

char *
GalerkinRadianceMethod::getStats() {
    static char stats[STRING_LENGTH];
    char *p;
    int n;

    p = stats;
    snprintf(p, STRING_LENGTH, "Galerkin Radiosity Statistics:\n\n%n", &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Iteration: %d\n\n%n", globalGalerkinState.iteration_nr, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Nr. elements: %d\n%n", galerkinElementGetNumberOfElements(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "clusters: %d\n%n", galerkinElementGetNumberOfClusters(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "surface elements: %d\n\n%n", galerkinElementGetNumberOfSurfaceElements(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Nr. interactions: %d\n%n", Interaction::getNumberOfInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "cluster to cluster: %d\n%n", Interaction::getNumberOfClusterToClusterInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "cluster to surface: %d\n%n", Interaction::getNumberOfClusterToSurfaceInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "surface to cluster: %d\n%n", Interaction::getNumberOfSurfaceToClusterInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "surface to surface: %d\n\n%n", Interaction::getNumberOfSurfaceToSurfaceInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "CPU time: %g secs.\n%n", globalGalerkinState.cpu_secs, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Minimum element area: %g m^2\n%n", GLOBAL_statistics.totalArea * (double) globalGalerkinState.relMinElemArea, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Link error threshold: %g %s\n\n%n",
             (double) (globalGalerkinState.errorNorm == RADIANCE_ERROR ?
                       M_PI * (globalGalerkinState.relLinkErrorThreshold *
                               GLOBAL_statistics.maxSelfEmittedRadiance.luminance()) :
                       globalGalerkinState.relLinkErrorThreshold *
                       GLOBAL_statistics.maxSelfEmittedPower.luminance()),
             (globalGalerkinState.errorNorm == RADIANCE_ERROR ? "lux" : "lumen"),
             &n);

    return stats;
}

void
GalerkinRadianceMethod::renderScene(java::ArrayList<Patch *> *scenePatches) {
    if ( GLOBAL_render_renderOptions.frustumCulling ) {
        openGlRenderWorldOctree(galerkinRenderPatch);
    } else {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            galerkinRenderPatch(scenePatches->get(i));
        }
    }
}

void
GalerkinRadianceMethod::writeVRML(FILE *fp) {
    writeVrmlHeader(fp);

    globalVrmlFileDescriptor = fp;
    galerkinWriteCoords();
    galerkinWriteColors();
    galerkinWriteCoordIndicesTopCluster();

    writeVRMLTrailer(fp);
}
