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
#include "render/render.h"
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

#define STRING_LENGTH 2000

GalerkinState GalerkinRadianceMethod::galerkinState;

static FILE *globalVrmlFileDescriptor;
static int globalNumberOfWrites;
static int globalVertexId;
static int globalTrue = true;
static int globalFalse = false;

static void
iterationMethodOption(void *value) {
    char *name = *(char **) value;

    if ( strncasecmp(name, "jacobi", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.iteration_method = JACOBI;
    } else if ( strncasecmp(name, "gaussseidel", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.iteration_method = GAUSS_SEIDEL;
    } else if ( strncasecmp(name, "southwell", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.iteration_method = SOUTH_WELL;
    } else {
        logError(nullptr, "Invalid iteration method '%s'", name);
    }
}

static void
hierarchicalOption(void *value) {
    int yesno = *(int *) value;
    GalerkinRadianceMethod::galerkinState.hierarchical = yesno;
}

static void
lazyOption(void *value) {
    int yesno = *(int *) value;
    GalerkinRadianceMethod::galerkinState.lazy_linking = yesno;
}

static void
clusteringOption(void *value) {
    int yesno = *(int *) value;
    GalerkinRadianceMethod::galerkinState.clustered = yesno;
}

static void
importanceOption(void *value) {
    int yesno = *(int *) value;
    GalerkinRadianceMethod::galerkinState.importance_driven = yesno;
}

static void
ambientOption(void *value) {
    int yesno = *(int *) value;
    GalerkinRadianceMethod::galerkinState.use_ambient_radiance = yesno;
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
    {"-gr-link-error-threshold", 6, Tfloat, &GalerkinRadianceMethod::galerkinState.relLinkErrorThreshold, DEFAULT_ACTION,
    "-gr-link-error-threshold <float>: Relative link error threshold"},
    {"-gr-min-elem-area",6, Tfloat, &GalerkinRadianceMethod::galerkinState.relMinElemArea,                DEFAULT_ACTION,
    "-gr-min-elem-area <float> \t: Relative element area threshold"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

/**
For counting how much CPU time was used for the computations
*/
static void
updateCpuSecs() {
    clock_t t = clock();
    GalerkinRadianceMethod::galerkinState.cpu_secs += (float) (t - GalerkinRadianceMethod::galerkinState.lastClock) / (float) CLOCKS_PER_SEC;
    GalerkinRadianceMethod::galerkinState.lastClock = t;
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
    GalerkinRadianceMethod::galerkinState.topCluster->traverseAllLeafElements(galerkinWriteVertexCoords);
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
        vertexRadiosity[0] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 0.0, &GalerkinRadianceMethod::galerkinState);
        vertexRadiosity[1] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 0.0, &GalerkinRadianceMethod::galerkinState);
        vertexRadiosity[2] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 1.0, &GalerkinRadianceMethod::galerkinState);
    } else {
        vertexRadiosity[0] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 0.0, &GalerkinRadianceMethod::galerkinState);
        vertexRadiosity[1] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 0.0, &GalerkinRadianceMethod::galerkinState);
        vertexRadiosity[2] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 1.0, 1.0, &GalerkinRadianceMethod::galerkinState);
        vertexRadiosity[3] = basisGalerkinRadianceAtPoint(galerkinElement, galerkinElement->radiance, 0.0, 1.0, &GalerkinRadianceMethod::galerkinState);
    }

    if ( GalerkinRadianceMethod::galerkinState.use_ambient_radiance ) {
        ColorRgb reflectivity = galerkinElement->patch->radianceData->Rd;
        ColorRgb ambient;

        ambient.scalarProduct(reflectivity, GalerkinRadianceMethod::galerkinState.ambient_radiance);
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
}

GalerkinRadianceMethod::~GalerkinRadianceMethod() {
    if ( galerkinState.topCluster != nullptr ) {
        delete galerkinState.topCluster;
        galerkinState.topCluster = nullptr;
    }
}

void
GalerkinRadianceMethod::patchInit(Patch *patch) {
    ColorRgb reflectivity = patch->radianceData->Rd;
    ColorRgb selfEmittanceRadiance = patch->radianceData->Ed;

    if ( galerkinState.use_constant_radiance ) {
        // See Neumann et-al, "The Constant Radiosity Step", Euro-graphics Rendering Workshop
        // '95, Dublin, Ireland, June 1995, p 336-344
        galerkinGetRadiance(patch).scalarProduct(reflectivity, galerkinState.constant_radiance);
        galerkinGetRadiance(patch).add(galerkinGetRadiance(patch), selfEmittanceRadiance);
        if ( galerkinState.iteration_method == SOUTH_WELL )
            patch->radianceData->unShotRadiance[0].subtract(galerkinGetRadiance(patch), galerkinState.constant_radiance);
    } else {
        galerkinSetRadiance(patch, selfEmittanceRadiance);
        if ( galerkinState.iteration_method == SOUTH_WELL )
            patch->radianceData->unShotRadiance[0] = galerkinGetRadiance(patch);
    }

    if ( galerkinState.importance_driven ) {
        switch ( galerkinState.iteration_method ) {
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

/**
Recomputes the color of a patch using ambient radiance term, ... if requested for
*/
void
GalerkinRadianceMethod::recomputePatchColor(Patch *patch) {
    ColorRgb reflectivity = patch->radianceData->Rd;
    ColorRgb radVis;

    // Compute the patches color based on its radiance + ambient radiance if desired
    if ( galerkinState.use_ambient_radiance ) {
        radVis.scalarProduct(reflectivity, galerkinState.ambient_radiance);
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
GalerkinRadianceMethod::parseOptions(int *argc, char **argv) {
    parseGeneralOptions(galerkinOptions, argc, argv);
}

void
GalerkinRadianceMethod::initialize(java::ArrayList<Patch *> *scenePatches) {
    galerkinState.iteration_nr = 0;
    galerkinState.cpu_secs = 0.0;

    basisGalerkinInitBasis();

    galerkinState.constant_radiance = GLOBAL_statistics.estimatedAverageRadiance;
    if ( galerkinState.use_constant_radiance ) {
        galerkinState.ambient_radiance.clear();
    } else {
        galerkinState.ambient_radiance = GLOBAL_statistics.estimatedAverageRadiance;
    }

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        patchInit(scenePatches->get(i));
    }

    galerkinState.topGeometry = GLOBAL_scene_clusteredWorldGeom;
    galerkinState.topCluster = galerkinCreateClusterHierarchy(galerkinState.topGeometry, &galerkinState);

    // Create a scratch software renderer for various operations on clusters
    scratchInit(&galerkinState);

    // Global variables used for form factor computation optimisation
    galerkinState.formFactorLastRcv = nullptr;
    galerkinState.formFactorLastSrc = nullptr;

    // Global variables for scratch rendering
    galerkinState.lastClusterId = -1;
    galerkinState.lastEye.set(HUGE, HUGE, HUGE);
}

int
GalerkinRadianceMethod::doStep(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    int done = false;

    if ( galerkinState.iteration_nr < 0 ) {
        logError("doGalerkinOneStep", "method not initialized");
        return true;    /* done, don't continue! */
    }

    galerkinState.iteration_nr++;
    galerkinState.lastClock = clock();

    // And now the real work
    switch ( galerkinState.iteration_method ) {
        case JACOBI:
        case GAUSS_SEIDEL:
            if ( galerkinState.clustered ) {
                done = doClusteredGatheringIteration(scenePatches, &galerkinState, this);
            } else {
                done = galerkinRadiosityDoGatheringIteration(scenePatches, &galerkinState, this);
            }
            break;
        case SOUTH_WELL:
            done = doShootingStep(scenePatches, &galerkinState, this);
            break;
        default:
            logFatal(2, "doGalerkinOneStep", "Invalid iteration method %d\n", galerkinState.iteration_method);
    }

    updateCpuSecs();

    return done;
}

void
GalerkinRadianceMethod::terminate(java::ArrayList<Patch *> *scenePatches) {
    scratchTerminate(&galerkinState);
    if ( galerkinState.topCluster != nullptr ) {
        galerkinDestroyClusterHierarchy(galerkinState.topCluster);
        galerkinState.topCluster = nullptr;
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

    rad = basisGalerkinRadianceAtPoint(leaf, leaf->radiance, u, v, &galerkinState);

    if ( galerkinState.use_ambient_radiance ) {
        // Add ambient radiance
        ColorRgb reflectivity = patch->radianceData->Rd;
        ColorRgb ambientRadiance;
        ambientRadiance.scalarProduct(reflectivity, galerkinState.ambient_radiance);
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
    snprintf(p, STRING_LENGTH, "Iteration: %d\n\n%n", galerkinState.iteration_nr, &n);
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
    snprintf(p, STRING_LENGTH, "CPU time: %g secs.\n%n", galerkinState.cpu_secs, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Minimum element area: %g m^2\n%n", GLOBAL_statistics.totalArea * (double) galerkinState.relMinElemArea, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Link error threshold: %g %s\n\n%n",
             (double) (galerkinState.errorNorm == RADIANCE_ERROR ?
                       M_PI * (galerkinState.relLinkErrorThreshold *
                               GLOBAL_statistics.maxSelfEmittedRadiance.luminance()) :
                       galerkinState.relLinkErrorThreshold *
                       GLOBAL_statistics.maxSelfEmittedPower.luminance()),
             (galerkinState.errorNorm == RADIANCE_ERROR ? "lux" : "lumen"),
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
