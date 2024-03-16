/**
Galerkin radiosity, with or without hierarchical refinement, with or
without clusters, with Jacobi, Gauss-Seidel or Southwell iterations,
potential-driven or not.
*/

#include <cstring>
#include <ctime>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "skin/Vertex.h"
#include "render/render.h"
#include "scene/Camera.h"
#include "common/options.h"
#include "io/writevrml.h"
#include "render/opengl.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/scratch.h"

GalerkinState GLOBAL_galerkin_state;

GalerkinState::GalerkinState():
    iteration_nr(),
    hierarchical(),
    importance_driven(),
    clustered(),
    iteration_method(),
    lazy_linking(),
    exact_visibility(),
    multiResolutionVisibility(),
    use_constant_radiance(),
    use_ambient_radiance(),
    constant_radiance(),
    ambient_radiance(),
    shaftCullMode(),
    rcvDegree(),
    srcDegree(),
    rcv3rule(),
    rcv4rule(),
    src3rule(),
    src4rule(),
    clusterRule(),
    topCluster(),
    topGeometry(),
    errorNorm(),
    relMinElemArea(),
    relLinkErrorThreshold (),
    basisType(),
    clusteringStrategy(),
    formFactorLastRcv(),
    formFactorLastSrc(),
    scratch(),
    scratchFbSize(),
    lastClusterId(),
    lastEye(),
    lastClock(),
    cpu_secs()
{
    GLOBAL_galerkin_state.hierarchical = DEFAULT_GAL_HIERARCHICAL;
    GLOBAL_galerkin_state.importance_driven = DEFAULT_GAL_IMPORTANCE_DRIVEN;
    GLOBAL_galerkin_state.clustered = DEFAULT_GAL_CLUSTERED;
    GLOBAL_galerkin_state.iteration_method = DEFAULT_GAL_ITERATION_METHOD;
    GLOBAL_galerkin_state.lazy_linking = DEFAULT_GAL_LAZY_LINKING;
    GLOBAL_galerkin_state.use_constant_radiance = DEFAULT_GAL_CONSTANT_RADIANCE;
    GLOBAL_galerkin_state.use_ambient_radiance = DEFAULT_GAL_AMBIENT_RADIANCE;
    GLOBAL_galerkin_state.shaftCullMode = DEFAULT_GAL_SHAFT_CULL_MODE;
    GLOBAL_galerkin_state.rcvDegree = DEFAULT_GAL_RCV_CUBATURE_DEGREE;
    GLOBAL_galerkin_state.srcDegree = DEFAULT_GAL_SRC_CUBATURE_DEGREE;
    setCubatureRules(&GLOBAL_galerkin_state.rcv3rule, &GLOBAL_galerkin_state.rcv4rule, GLOBAL_galerkin_state.rcvDegree);
    setCubatureRules(&GLOBAL_galerkin_state.src3rule, &GLOBAL_galerkin_state.src4rule, GLOBAL_galerkin_state.srcDegree);
    GLOBAL_galerkin_state.clusterRule = &GLOBAL_crv1;
    GLOBAL_galerkin_state.relMinElemArea = DEFAULT_GAL_REL_MIN_ELEM_AREA;
    GLOBAL_galerkin_state.relLinkErrorThreshold = DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD;
    GLOBAL_galerkin_state.errorNorm = DEFAULT_GAL_ERROR_NORM;
    GLOBAL_galerkin_state.basisType = DEFAULT_GAL_BASIS_TYPE;
    GLOBAL_galerkin_state.exact_visibility = DEFAULT_GAL_EXACT_VISIBILITY;
    GLOBAL_galerkin_state.multiResolutionVisibility = DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY;
    GLOBAL_galerkin_state.clusteringStrategy = DEFAULT_GAL_CLUSTERING_STRATEGY;
    GLOBAL_galerkin_state.scratch = nullptr;
    GLOBAL_galerkin_state.scratchFbSize = DEFAULT_GAL_SCRATCH_FB_SIZE;

    GLOBAL_galerkin_state.iteration_nr = -1; // This means "not initialized"
}

GalerkinRadianceMethod::GalerkinRadianceMethod() {
    className = GALERKIN;
}

GalerkinRadianceMethod::~GalerkinRadianceMethod() {
}

static int globalTrue = true;
static int globalFalse = false;

#define STRING_LENGTH 2000

/**
Installs cubature rules for triangles and quadrilaterals of the specified degree
*/
void
setCubatureRules(CUBARULE **triRule, CUBARULE **quadRule, GalerkinCubatureDegree degree) {
    switch ( degree ) {
        case DEGREE_1:
            *triRule = &GLOBAL_crt1;
            *quadRule = &GLOBAL_crq1;
            break;
        case DEGREE_2:
            *triRule = &GLOBAL_crt2;
            *quadRule = &GLOBAL_crq2;
            break;
        case DEGREE_3:
            *triRule = &GLOBAL_crt3;
            *quadRule = &GLOBAL_crq3;
            break;
        case DEGREE_4:
            *triRule = &GLOBAL_crt4;
            *quadRule = &GLOBAL_crq4;
            break;
        case DEGREE_5:
            *triRule = &GLOBAL_crt5;
            *quadRule = &GLOBAL_crq5;
            break;
        case DEGREE_6:
            *triRule = &GLOBAL_crt7;
            *quadRule = &GLOBAL_crq6;
            break;
        case DEGREE_7:
            *triRule = &GLOBAL_crt7;
            *quadRule = &GLOBAL_crq7;
            break;
        case DEGREE_8:
            *triRule = &GLOBAL_crt8;
            *quadRule = &GLOBAL_crq8;
            break;
        case DEGREE_9:
            *triRule = &GLOBAL_crt9;
            *quadRule = &GLOBAL_crq9;
            break;
        case DEGREE_3_PROD:
            *triRule = &GLOBAL_crt5;
            *quadRule = &GLOBAL_crq3pg;
            break;
        case DEGREE_5_PROD:
            *triRule = &GLOBAL_crt7;
            *quadRule = &GLOBAL_crq5pg;
            break;
        case DEGREE_7_PROD:
            *triRule = &GLOBAL_crt9;
            *quadRule = &GLOBAL_crq7pg;
            break;
        default:
            logFatal(2, "setCubatureRules", "Invalid degree %d", degree);
    }
}

static void
iterationMethodOption(void *value) {
    char *name = *(char **) value;

    if ( strncasecmp(name, "jacobi", 2) == 0 ) {
        GLOBAL_galerkin_state.iteration_method = JACOBI;
    } else if ( strncasecmp(name, "gaussseidel", 2) == 0 ) {
        GLOBAL_galerkin_state.iteration_method = GAUSS_SEIDEL;
    } else if ( strncasecmp(name, "southwell", 2) == 0 ) {
        GLOBAL_galerkin_state.iteration_method = SOUTH_WELL;
    } else {
        logError(nullptr, "Invalid iteration method '%s'", name);
    }
}

static void
hierarchicalOption(void *value) {
    int yesno = *(int *) value;
    GLOBAL_galerkin_state.hierarchical = yesno;
}

static void
lazyOption(void *value) {
    int yesno = *(int *) value;
    GLOBAL_galerkin_state.lazy_linking = yesno;
}

static void
clusteringOption(void *value) {
    int yesno = *(int *) value;
    GLOBAL_galerkin_state.clustered = yesno;
}

static void
importanceOption(void *value) {
    int yesno = *(int *) value;
    GLOBAL_galerkin_state.importance_driven = yesno;
}

static void
ambientOption(void *value) {
    int yesno = *(int *) value;
    GLOBAL_galerkin_state.use_ambient_radiance = yesno;
}

static CommandLineOptionDescription galerkinOptions[] = {
    {"-gr-iteration-method",     6,  Tstring,  nullptr, iterationMethodOption,
    "-gr-iteration-method <methodname>: Jacobi, GaussSeidel, Southwell"},
    {"-gr-hierarchical",         6,  TYPELESS, (void *) &globalTrue, hierarchicalOption,
    "-gr-hierarchical    \t: do hierarchical refinement"},
    {"-gr-not-hierarchical",     10, TYPELESS, (void *) &globalFalse, hierarchicalOption,
    "-gr-not-hierarchical\t: don't do hierarchical refinement"},
    {"-gr-lazy-linking",         6,  TYPELESS, (void *) &globalTrue, lazyOption,
    "-gr-lazy-linking    \t: do lazy linking"},
    {"-gr-no-lazy-linking",      10, TYPELESS, (void *) &globalFalse, lazyOption,
    "-gr-no-lazy-linking \t: don't do lazy linking"},
    {"-gr-clustering",           6,  TYPELESS, (void *) &globalTrue, clusteringOption,
    "-gr-clustering      \t: do clustering"},
    {"-gr-no-clustering",        10, TYPELESS, (void *) &globalFalse, clusteringOption,
    "-gr-no-clustering   \t: don't do clustering"},
    {"-gr-importance",           6,  TYPELESS, (void *) &globalTrue, importanceOption,
    "-gr-importance      \t: do view-potential driven computations"},
    {"-gr-no-importance",        10, TYPELESS, (void *) &globalFalse, importanceOption,
    "-gr-no-importance   \t: don't use view-potential"},
    {"-gr-ambient",              6,  TYPELESS, (void *) &globalTrue, ambientOption,
    "-gr-ambient         \t: do visualisation with ambient term"},
    {"-gr-no-ambient",           10, TYPELESS, (void *) &globalFalse, ambientOption,
    "-gr-no-ambient      \t: do visualisation without ambient term"},
    {"-gr-link-error-threshold", 6,  Tfloat,   &GLOBAL_galerkin_state.relLinkErrorThreshold, DEFAULT_ACTION,
    "-gr-link-error-threshold <float>: Relative link error threshold"},
    {"-gr-min-elem-area",6, Tfloat, &GLOBAL_galerkin_state.relMinElemArea, DEFAULT_ACTION,
    "-gr-min-elem-area <float> \t: Relative element area threshold"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
GalerkinRadianceMethod::parseOptions(int *argc, char **argv) {
    parseGeneralOptions(galerkinOptions, argc, argv);
}

/**
For counting how much CPU time was used for the computations
*/
static void
updateCpuSecs() {
    clock_t t;

    t = clock();
    GLOBAL_galerkin_state.cpu_secs += (float) (t - GLOBAL_galerkin_state.lastClock) / (float) CLOCKS_PER_SEC;
    GLOBAL_galerkin_state.lastClock = t;
}

/**
Radiance data for a Patch is a surface element
*/
Element *
GalerkinRadianceMethod::createPatchData(Patch *patch) {
    return patch->radianceData = new GalerkinElement(patch);
}

void
GalerkinRadianceMethod::destroyPatchData(Patch *patch) {
    delete ((GalerkinElement *)patch->radianceData);
    patch->radianceData = nullptr;
}

/**
Recomputes the color of a patch using ambient radiance term, ... if requested for
*/
void
patchRecomputeColor(Patch *patch) {
    COLOR reflectivity = patch->radianceData->Rd;
    COLOR rad_vis;

    /* compute the patches color based on its radiance + ambient radiance
     * if desired. */
    if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
        colorProduct(reflectivity, GLOBAL_galerkin_state.ambient_radiance, rad_vis);
        colorAdd(rad_vis, RADIANCE(patch), rad_vis);
        radianceToRgb(rad_vis, &patch->color);
    } else {
        radianceToRgb(RADIANCE(patch), &patch->color);
    }
    patchComputeVertexColors(patch);
}

static void
patchInit(Patch *patch) {
    COLOR reflectivity = patch->radianceData->Rd;
    COLOR selfEmittanceRadiance = patch->radianceData->Ed;

    if ( GLOBAL_galerkin_state.use_constant_radiance ) {
        // See Neumann et-al, "The Constant Radiosity Step", Euro-graphics Rendering Workshop
        // '95, Dublin, Ireland, June 1995, p 336-344
        colorProduct(reflectivity, GLOBAL_galerkin_state.constant_radiance, RADIANCE(patch));
        colorAdd(RADIANCE(patch), selfEmittanceRadiance, RADIANCE(patch));
        if ( GLOBAL_galerkin_state.iteration_method == SOUTH_WELL )
            colorSubtract(RADIANCE(patch), GLOBAL_galerkin_state.constant_radiance,
                          UN_SHOT_RADIANCE(patch));
    } else {
        RADIANCE(patch) = selfEmittanceRadiance;
        if ( GLOBAL_galerkin_state.iteration_method == SOUTH_WELL )
            UN_SHOT_RADIANCE(patch) = RADIANCE(patch);
    }

    if ( GLOBAL_galerkin_state.importance_driven ) {
        switch ( GLOBAL_galerkin_state.iteration_method ) {
            case GAUSS_SEIDEL:
            case JACOBI:
                POTENTIAL(patch) = patch->directPotential;
                break;
            case SOUTH_WELL:
                POTENTIAL(patch) = UN_SHOT_POTENTIAL(patch) = patch->directPotential;
                break;
            default:
                logFatal(-1, "patchInit", "Invalid iteration method");
        }
    }

    patchRecomputeColor(patch);
}

void
GalerkinRadianceMethod::initialize(java::ArrayList<Patch *> *scenePatches) {
    GLOBAL_galerkin_state.iteration_nr = 0;
    GLOBAL_galerkin_state.cpu_secs = 0.0;

    basisGalerkinInitBasis();

    GLOBAL_galerkin_state.constant_radiance = GLOBAL_statistics.estimatedAverageRadiance;
    if ( GLOBAL_galerkin_state.use_constant_radiance ) {
        colorClear(GLOBAL_galerkin_state.ambient_radiance);
    } else {
        GLOBAL_galerkin_state.ambient_radiance = GLOBAL_statistics.estimatedAverageRadiance;
    }

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        patchInit(scenePatches->get(i));
    }

    GLOBAL_galerkin_state.topGeometry = GLOBAL_scene_clusteredWorldGeom;
    GLOBAL_galerkin_state.topCluster = galerkinCreateClusterHierarchy(GLOBAL_galerkin_state.topGeometry);

    // Create a scratch software renderer for various operations on clusters
    scratchInit();

    // Global variables used for form factor computation optimisation
    GLOBAL_galerkin_state.formFactorLastRcv = nullptr;
    GLOBAL_galerkin_state.formFactorLastSrc = nullptr;

    // Global variables for scratch rendering
    GLOBAL_galerkin_state.lastClusterId = -1;
    vectorSet(GLOBAL_galerkin_state.lastEye, HUGE, HUGE, HUGE);
}

int
GalerkinRadianceMethod::doStep(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    int done = false;

    if ( GLOBAL_galerkin_state.iteration_nr < 0 ) {
        logError("doGalerkinOneStep", "method not initialized");
        return true;    /* done, don't continue! */
    }

    GLOBAL_galerkin_state.iteration_nr++;
    GLOBAL_galerkin_state.lastClock = clock();

    // And now the real work
    switch ( GLOBAL_galerkin_state.iteration_method ) {
        case JACOBI:
        case GAUSS_SEIDEL:
            if ( GLOBAL_galerkin_state.clustered ) {
                done = doClusteredGatheringIteration(scenePatches);
            } else {
                done = randomWalkRadiosityDoGatheringIteration(scenePatches);
            }
            break;
        case SOUTH_WELL:
            done = doShootingStep(scenePatches);
            break;
        default:
            logFatal(2, "doGalerkinOneStep", "Invalid iteration method %d\n", GLOBAL_galerkin_state.iteration_method);
    }

    updateCpuSecs();

    return done;
}

void
GalerkinRadianceMethod::terminate(java::ArrayList<Patch *> *scenePatches) {
    scratchTerminate();
    if ( GLOBAL_galerkin_state.topCluster != nullptr ) {
        galerkinDestroyClusterHierarchy(GLOBAL_galerkin_state.topCluster);
        GLOBAL_galerkin_state.topCluster = nullptr;
    }
}

COLOR
GalerkinRadianceMethod::getRadiance(Patch *patch, double u, double v, Vector3D dir) {
    GalerkinElement *leaf;
    COLOR rad;

    if ( patch->jacobian ) {
        patch->biLinearToUniform(&u, &v);
    }

    GalerkinElement *topLevelElement = patchGalerkinElement(patch);
    leaf = topLevelElement->regularLeafAtPoint(&u, &v);

    rad = basisGalerkinRadianceAtPoint(leaf, leaf->radiance, u, v);

    if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
        // Add ambient radiance
        COLOR reflectivity = patch->radianceData->Rd;
        COLOR ambientRadiance;
        colorProduct(reflectivity, GLOBAL_galerkin_state.ambient_radiance, ambientRadiance);
        colorAdd(rad, ambientRadiance, rad);
    }

    return rad;
}

char *
GalerkinRadianceMethod::getStats() {
    static char stats[STRING_LENGTH];
    char *p;
    int n;

    p = stats;
    snprintf(p, STRING_LENGTH, "Galerkin Radiosity Statistics:\n\n%n", &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Iteration: %d\n\n%n", GLOBAL_galerkin_state.iteration_nr, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Nr. elements: %d\n%n", galerkinElementGetNumberOfElements(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "clusters: %d\n%n", galerkinElementGetNumberOfClusters(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "surface elements: %d\n\n%n", galerkinElementGetNumberOfSurfaceElements(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Nr. interactions: %d\n%n", getNumberOfInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "cluster to cluster: %d\n%n", getNumberOfClusterToClusterInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "cluster to surface: %d\n%n", getNumberOfClusterToSurfaceInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "surface to cluster: %d\n%n", getNumberOfSurfaceToClusterInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "surface to surface: %d\n\n%n", getNumberOfSurfaceToSurfaceInteractions(), &n);
    p += n;
    snprintf(p, STRING_LENGTH, "CPU time: %g secs.\n%n", GLOBAL_galerkin_state.cpu_secs, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Minimum element area: %g m^2\n%n", GLOBAL_statistics.totalArea * (double) GLOBAL_galerkin_state.relMinElemArea, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Link error threshold: %g %s\n\n%n",
            (double) (GLOBAL_galerkin_state.errorNorm == RADIANCE_ERROR ?
                      M_PI * (GLOBAL_galerkin_state.relLinkErrorThreshold *
                              colorLuminance(GLOBAL_statistics.maxSelfEmittedRadiance)) :
                      GLOBAL_galerkin_state.relLinkErrorThreshold *
                      colorLuminance(GLOBAL_statistics.maxSelfEmittedPower)),
            (GLOBAL_galerkin_state.errorNorm == RADIANCE_ERROR ? "lux" : "lumen"),
            &n);

    return stats;
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
    GalerkinElement *topLevelElement = patchGalerkinElement(patch);
    renderElementHierarchy(topLevelElement);
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

static FILE *globalVrmlFileDescriptor;
static int globalNumberOfWrites;
static int globalVertexId;

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
    GLOBAL_galerkin_state.topCluster->traverseAllLeafElements(galerkinWriteVertexCoords);
    fprintf(globalVrmlFileDescriptor, " ] ");
    fprintf(globalVrmlFileDescriptor, "\n\t}\n");
}

static void
galerkinWriteVertexColor(RGB *color) {
    /* not yet written */
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
    COLOR vertexRadiosity[4];
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

    if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
        COLOR reflectivity = galerkinElement->patch->radianceData->Rd;
        COLOR ambient;

        colorProduct(reflectivity, GLOBAL_galerkin_state.ambient_radiance, ambient);
        for ( i = 0; i < galerkinElement->patch->numberOfVertices; i++ ) {
            colorAdd(vertexRadiosity[i], ambient, vertexRadiosity[i]);
        }
    }

    for ( i = 0; i < galerkinElement->patch->numberOfVertices; i++ ) {
        RGB col{};
        radianceToRgb(vertexRadiosity[i], &col);
        galerkinWriteVertexColor(&col);
    }
}

static void
galerkinWriteVertexColorsTopCluster() {
    globalVertexId = globalNumberOfWrites = 0;
    fprintf(globalVrmlFileDescriptor, "\tcolor Color {\n\t  color [ ");
    GLOBAL_galerkin_state.topCluster->traverseAllLeafElements(galerkinWriteVertexColors);
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
    GLOBAL_galerkin_state.topCluster->traverseAllLeafElements(galerkinWriteCoordIndices);
    fprintf(globalVrmlFileDescriptor, " ]\n");
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

void
galerkinFreeMemory() {
    if ( GLOBAL_galerkin_state.scratch != nullptr ) {
        delete GLOBAL_galerkin_state.scratch;
        GLOBAL_galerkin_state.scratch = nullptr;
    }
}
