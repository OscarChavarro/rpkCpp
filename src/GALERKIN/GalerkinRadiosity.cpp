/**
Galerkin radiosity, with or without hierarchical refinement, with or
without clusters, with Jacobi, Gauss-Seidel or Southwell iterations,
potential-driven or not.
*/

#include <cstring>
#include <ctime>

#include "common/error.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "skin/Vertex.h"
#include "shared/render.h"
#include "shared/camera.h"
#include "shared/options.h"
#include "shared/writevrml.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "GALERKIN/GalerkinRadiosity.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/scratch.h"

GALERKIN_STATE GLOBAL_galerkin_state;

static int t = true;
static int f = false;

/**
Installs cubature rules for triangles and quadrilaterals of the specified degree
*/
void
setCubatureRules(CUBARULE **trirule, CUBARULE **quadrule, CUBATURE_DEGREE degree) {
    switch ( degree ) {
        case DEGREE_1:
            *trirule = &GLOBAL_crt1;
            *quadrule = &GLOBAL_crq1;
            break;
        case DEGREE_2:
            *trirule = &GLOBAL_crt2;
            *quadrule = &GLOBAL_crq2;
            break;
        case DEGREE_3:
            *trirule = &GLOBAL_crt3;
            *quadrule = &GLOBAL_crq3;
            break;
        case DEGREE_4:
            *trirule = &GLOBAL_crt4;
            *quadrule = &GLOBAL_crq4;
            break;
        case DEGREE_5:
            *trirule = &GLOBAL_crt5;
            *quadrule = &GLOBAL_crq5;
            break;
        case DEGREE_6:
            *trirule = &GLOBAL_crt7;
            *quadrule = &GLOBAL_crq6;
            break;
        case DEGREE_7:
            *trirule = &GLOBAL_crt7;
            *quadrule = &GLOBAL_crq7;
            break;
        case DEGREE_8:
            *trirule = &GLOBAL_crt8;
            *quadrule = &GLOBAL_crq8;
            break;
        case DEGREE_9:
            *trirule = &GLOBAL_crt9;
            *quadrule = &GLOBAL_crq9;
            break;
        case DEGREE_3_PROD:
            *trirule = &GLOBAL_crt5;
            *quadrule = &GLOBAL_crq3pg;
            break;
        case DEGREE_5_PROD:
            *trirule = &GLOBAL_crt7;
            *quadrule = &GLOBAL_crq5pg;
            break;
        case DEGREE_7_PROD:
            *trirule = &GLOBAL_crt9;
            *quadrule = &GLOBAL_crq7pg;
            break;
        default:
            logFatal(2, "setCubatureRules", "Invalid degree %d", degree);
    }
}

static void
galerkinDefaults() {
    GLOBAL_galerkin_state.hierarchical = DEFAULT_GAL_HIERARCHICAL;
    GLOBAL_galerkin_state.importance_driven = DEFAULT_GAL_IMPORTANCE_DRIVEN;
    GLOBAL_galerkin_state.clustered = DEFAULT_GAL_CLUSTERED;
    GLOBAL_galerkin_state.iteration_method = DEFAULT_GAL_ITERATION_METHOD;
    GLOBAL_galerkin_state.lazy_linking = DEFAULT_GAL_LAZY_LINKING;
    GLOBAL_galerkin_state.use_constant_radiance = DEFAULT_GAL_CONSTANT_RADIANCE;
    GLOBAL_galerkin_state.use_ambient_radiance = DEFAULT_GAL_AMBIENT_RADIANCE;
    GLOBAL_galerkin_state.shaftcullmode = DEFAULT_GAL_SHAFTCULLMODE;
    GLOBAL_galerkin_state.rcv_degree = DEFAULT_GAL_RCV_CUBATURE_DEGREE;
    GLOBAL_galerkin_state.src_degree = DEFAULT_GAL_SRC_CUBATURE_DEGREE;
    setCubatureRules(&GLOBAL_galerkin_state.rcv3rule, &GLOBAL_galerkin_state.rcv4rule, GLOBAL_galerkin_state.rcv_degree);
    setCubatureRules(&GLOBAL_galerkin_state.src3rule, &GLOBAL_galerkin_state.src4rule, GLOBAL_galerkin_state.src_degree);
    GLOBAL_galerkin_state.clusRule = &GLOBAL_crv1;
    GLOBAL_galerkin_state.rel_min_elem_area = DEFAULT_GAL_REL_MIN_ELEM_AREA;
    GLOBAL_galerkin_state.rel_link_error_threshold = DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD;
    GLOBAL_galerkin_state.error_norm = DEFAULT_GAL_ERROR_NORM;
    GLOBAL_galerkin_state.basis_type = DEFAULT_GAL_BASIS_TYPE;
    GLOBAL_galerkin_state.exact_visibility = DEFAULT_GAL_EXACT_VISIBILITY;
    GLOBAL_galerkin_state.multires_visibility = DEFAULT_GAL_MULTIRES_VISIBILITY;
    GLOBAL_galerkin_state.clustering_strategy = DEFAULT_GAL_CLUSTERING_STRATEGY;
    GLOBAL_galerkin_state.scratch = (SGL_CONTEXT *) nullptr;
    GLOBAL_galerkin_state.scratch_fb_size = DEFAULT_GAL_SCRATCH_FB_SIZE;

    GLOBAL_galerkin_state.iteration_nr = -1;    /* means "not initialized" */
}

static void
iterationMethodOption(void *value) {
    char *name = *(char **) value;

    if ( strncasecmp(name, "jacobi", 2) == 0 ) {
        GLOBAL_galerkin_state.iteration_method = JACOBI;
    } else if ( strncasecmp(name, "gaussseidel", 2) == 0 ) {
        GLOBAL_galerkin_state.iteration_method = GAUSS_SEIDEL;
    } else if ( strncasecmp(name, "southwell", 2) == 0 ) {
        GLOBAL_galerkin_state.iteration_method = SOUTHWELL;
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

static CMDLINEOPTDESC galerkinOptions[] = {
        {"-gr-iteration-method",     6,  Tstring,  nullptr, iterationMethodOption,
                "-gr-iteration-method <methodname>: Jacobi, GaussSeidel, Southwell"},
        {"-gr-hierarchical",         6,  TYPELESS, (void *) &t, hierarchicalOption,
                "-gr-hierarchical    \t: do hierarchical refinement"},
        {"-gr-not-hierarchical",     10, TYPELESS, (void *) &f, hierarchicalOption,
                "-gr-not-hierarchical\t: don't do hierarchical refinement"},
        {"-gr-lazy-linking",         6,  TYPELESS, (void *) &t, lazyOption,
                "-gr-lazy-linking    \t: do lazy linking"},
        {"-gr-no-lazy-linking",      10, TYPELESS, (void *) &f, lazyOption,
                "-gr-no-lazy-linking \t: don't do lazy linking"},
        {"-gr-clustering",           6,  TYPELESS, (void *) &t, clusteringOption,
                "-gr-clustering      \t: do clustering"},
        {"-gr-no-clustering",        10, TYPELESS, (void *) &f, clusteringOption,
                "-gr-no-clustering   \t: don't do clustering"},
        {"-gr-importance",           6,  TYPELESS, (void *) &t, importanceOption,
                "-gr-importance      \t: do view-potential driven computations"},
        {"-gr-no-importance",        10, TYPELESS, (void *) &f, importanceOption,
                "-gr-no-importance   \t: don't use view-potential"},
        {"-gr-ambient",              6,  TYPELESS, (void *) &t, ambientOption,
                "-gr-ambient         \t: do visualisation with ambient term"},
        {"-gr-no-ambient",           10, TYPELESS, (void *) &f, ambientOption,
                "-gr-no-ambient      \t: do visualisation without ambient term"},
        {"-gr-link-error-threshold", 6,  Tfloat,   &GLOBAL_galerkin_state.rel_link_error_threshold, DEFAULT_ACTION,
                "-gr-link-error-threshold <float>: Relative link error threshold"},
        {"-gr-min-elem-area",        6,  Tfloat,   &GLOBAL_galerkin_state.rel_min_elem_area,        DEFAULT_ACTION,
                "-gr-min-elem-area <float> \t: Relative element area threshold"},
        {nullptr,                       0,  TYPELESS, nullptr,                          DEFAULT_ACTION,
                nullptr}
};

static void
parseGalerkinOptions(int *argc, char **argv) {
    parseOptions(galerkinOptions, argc, argv);
}

static void
printGalerkinOptions(FILE *fp) {
}

/**
For counting how much CPU time was used for the computations
*/
static void
updateCpuSecs() {
    clock_t t;

    t = clock();
    GLOBAL_galerkin_state.cpu_secs += (float) (t - GLOBAL_galerkin_state.lastclock) / (float) CLOCKS_PER_SEC;
    GLOBAL_galerkin_state.lastclock = t;
}

/**
Radiance data for a Patch is a surface element
*/
static Element *
createPatchData(Patch *patch) {
    return patch->radiance_data = galerkinCreateToplevelElement(patch);
}

static void
printPatchData(FILE *out, Patch *patch) {
    galerkinPrintElement(out, (GalerkinElement *) patch->radiance_data);
}

static void
destroyPatchData(Patch *patch) {
    galerkinDestroyToplevelElement((GalerkinElement *) patch->radiance_data);
    patch->radiance_data = nullptr;
}

void
patchRecomputeColor(Patch *patch) {
    COLOR rho = REFLECTIVITY(patch);
    COLOR rad_vis;

    /* compute the patches color based on its radiance + ambient radiance
     * if desired. */
    if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
        colorProduct(rho, GLOBAL_galerkin_state.ambient_radiance, rad_vis);
        colorAdd(rad_vis, RADIANCE(patch), rad_vis);
        radianceToRgb(rad_vis, &patch->color);
    } else {
        radianceToRgb(RADIANCE(patch), &patch->color);
    }
    patchComputeVertexColors(patch);
}

static void
patchInit(Patch *patch) {
    COLOR rho = REFLECTIVITY(patch), Ed = SELFEMITTED_RADIANCE(patch);

    if ( GLOBAL_galerkin_state.use_constant_radiance ) {
        /* see Neumann et al, "The Constant Radiosity Step", Eurographics Rendering Workshop
         * '95, Dublin, Ireland, June 1995, p 336-344. */
        colorProduct(rho, GLOBAL_galerkin_state.constant_radiance, RADIANCE(patch));
        colorAdd(RADIANCE(patch), Ed, RADIANCE(patch));
        if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL )
            colorSubtract(RADIANCE(patch), GLOBAL_galerkin_state.constant_radiance,
                          UNSHOT_RADIANCE(patch));
    } else {
        RADIANCE(patch) = Ed;
        if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL )
            UNSHOT_RADIANCE(patch) = RADIANCE(patch);
    }

    if ( GLOBAL_galerkin_state.importance_driven ) {
        switch ( GLOBAL_galerkin_state.iteration_method ) {
            case GAUSS_SEIDEL:
            case JACOBI:
                POTENTIAL(patch).f = patch->directPotential;
                break;
            case SOUTHWELL:
                POTENTIAL(patch).f = UNSHOT_POTENTIAL(patch).f = patch->directPotential;
                break;
            default:
                logFatal(-1, "patchInit", "Invalid iteration method");
        }
    }

    patchRecomputeColor(patch);
}

void
initGalerkin() {
    GLOBAL_galerkin_state.iteration_nr = 0;
    GLOBAL_galerkin_state.cpu_secs = 0.;

    basisGalerkinInitBasis();

    GLOBAL_galerkin_state.constant_radiance = GLOBAL_statistics_estimatedAverageRadiance;
    if ( GLOBAL_galerkin_state.use_constant_radiance ) {
        colorClear(GLOBAL_galerkin_state.ambient_radiance);
    } else {
        GLOBAL_galerkin_state.ambient_radiance = GLOBAL_statistics_estimatedAverageRadiance;
    }

    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        patchInit(window->patch);
    }

    GLOBAL_galerkin_state.top_geom = GLOBAL_scene_clusteredWorldGeom;
    GLOBAL_galerkin_state.top_cluster = galerkinCreateClusterHierarchy(GLOBAL_galerkin_state.top_geom);

    /* create a scratch software renderer for various operations on clusters */
    ScratchInit();

    /* global variables used for formfactor computation optimisation */
    GLOBAL_galerkin_state.fflastrcv = GLOBAL_galerkin_state.fflastsrc = (GalerkinElement *) nullptr;

    /* global variables for scratch rendering */
    GLOBAL_galerkin_state.lastclusid = -1;
    VECTORSET(GLOBAL_galerkin_state.lasteye, HUGE, HUGE, HUGE);
}

static int
doGalerkinOneStep() {
    int done = false;

    if ( GLOBAL_galerkin_state.iteration_nr < 0 ) {
        logError("doGalerkinOneStep", "method not initialized");
        return true;    /* done, don't continue! */
    }

    GLOBAL_galerkin_state.wake_up = false;

    GLOBAL_galerkin_state.iteration_nr++;
    GLOBAL_galerkin_state.lastclock = clock();

    /* and now the real work */
    switch ( GLOBAL_galerkin_state.iteration_method ) {
        case JACOBI:
        case GAUSS_SEIDEL:
            if ( GLOBAL_galerkin_state.clustered ) {
                done = doClusteredGatheringIteration();
            } else {
                done = randomWalkRadiosityDoGatheringIteration();
            }
            break;
        case SOUTHWELL:
            done = doShootingStep();
            break;
        default:
            logFatal(2, "doGalerkinOneStep", "Invalid iteration method %d\n", GLOBAL_galerkin_state.iteration_method);
    }

    updateCpuSecs();

    return done;
}

static void
terminateGalerkin() {
    ScratchTerminate();
    galerkinDestroyClusterHierarchy(GLOBAL_galerkin_state.top_cluster);
}

static COLOR
getRadiance(Patch *patch, double u, double v, Vector3D dir) {
    GalerkinElement *leaf;
    COLOR rad;

    if ( patch->jacobian ) {
        bilinearToUniform(patch, &u, &v);
    }

    leaf = RegularLeafElementAtPoint(TOPLEVEL_ELEMENT(patch), &u, &v);

    rad = basisGalerkinRadianceAtPoint(leaf, leaf->radiance, u, v);

    if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
        /* add ambient radiance */
        COLOR rho = REFLECTIVITY(patch);
        COLOR ambirad;
        colorProduct(rho, GLOBAL_galerkin_state.ambient_radiance, ambirad);
        colorAdd(rad, ambirad, rad);
    }

    return rad;
}

static char *
getGalerkinStats() {
    static char stats[2000];
    char *p;
    int n;

    p = stats;
    sprintf(p, "Galerkin Radiosity Statistics:\n\n%n", &n);
    p += n;
    sprintf(p, "Iteration: %d\n\n%n", GLOBAL_galerkin_state.iteration_nr, &n);
    p += n;
    sprintf(p, "Nr. elements: %d\n%n", GetNumberOfElements(), &n);
    p += n;
    sprintf(p, "clusters: %d\n%n", GetNumberOfClusters(), &n);
    p += n;
    sprintf(p, "surface elements: %d\n\n%n", GetNumberOfSurfaceElements(), &n);
    p += n;
    sprintf(p, "Nr. interactions: %d\n%n", GetNumberOfInteractions(), &n);
    p += n;
    sprintf(p, "cluster to cluster: %d\n%n", GetNumberOfClusterToClusterInteractions(), &n);
    p += n;
    sprintf(p, "cluster to surface: %d\n%n", GetNumberOfClusterToSurfaceInteractions(), &n);
    p += n;
    sprintf(p, "surface to cluster: %d\n%n", GetNumberOfSurfaceToClusterInteractions(), &n);
    p += n;
    sprintf(p, "surface to surface: %d\n\n%n", GetNumberOfSurfaceToSurfaceInteractions(), &n);
    p += n;
    sprintf(p, "CPU time: %g secs.\n%n", GLOBAL_galerkin_state.cpu_secs, &n);
    p += n;
    sprintf(p, "Minimum element area: %g m^2\n%n", GLOBAL_statistics_totalArea * (double) GLOBAL_galerkin_state.rel_min_elem_area, &n);
    p += n;
    sprintf(p, "Link error threshold: %g %s\n\n%n",
            (double) (GLOBAL_galerkin_state.error_norm == RADIANCE_ERROR ?
                      M_PI * (GLOBAL_galerkin_state.rel_link_error_threshold *
                              colorLuminance(GLOBAL_statistics_maxSelfEmittedRadiance)) :
                      GLOBAL_galerkin_state.rel_link_error_threshold *
                              colorLuminance(GLOBAL_statistics_maxSelfEmittedPower)),
            (GLOBAL_galerkin_state.error_norm == RADIANCE_ERROR ? "lux" : "lumen"),
            &n);
    p += n;

    return stats;
}

static void
renderElementHierarchy(GalerkinElement *elem) {
    if ( !elem->regular_subelements ) {
        RenderElement(elem);
    } else ITERATE_REGULAR_SUBELEMENTS(elem, renderElementHierarchy);
}

static void
galerkinRenderPatch(Patch *patch) {
    renderElementHierarchy(TOPLEVEL_ELEMENT(patch));
}

void
galerkinRender() {
    if ( GLOBAL_render_renderOptions.frustum_culling ) {
        renderWorldOctree(galerkinRenderPatch);
    } else {
        for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
            galerkinRenderPatch(window->patch);
        }
    }
}

static void
galerkinUpdateMaterial(Material *oldMaterial, Material *newMaterial) {
    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        shootingUpdateMaterial(oldMaterial, newMaterial);
    } else if ( GLOBAL_galerkin_state.iteration_method == JACOBI ||
                GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ) {
        gatheringUpdateMaterial(oldMaterial, newMaterial);
    } else {
        fprintf(stderr, "galerkinUpdateMaterial: not yet implemented.\n");
    }

    renderScene();
}

/* *********************************************************** */
/* VRML output */
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
galerkinWriteVertexCoords(GalerkinElement *elem) {
    Vector3D v[8];
    int i, nverts = ElementVertices(elem, v);
    for ( i = 0; i < nverts; i++ ) {
        galerkinWriteVertexCoord(&v[i]);
    }
}

static void
galerkinWriteCoords() {
    globalNumberOfWrites = globalVertexId = 0;
    fprintf(globalVrmlFileDescriptor, "\tcoord Coordinate {\n\t  point [ ");
    ForAllLeafElements(GLOBAL_galerkin_state.top_cluster, galerkinWriteVertexCoords);
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
galerkinWriteVertexColors(GalerkinElement *element) {
    COLOR vertrad[4];
    int i;

    if ( element->patch->numberOfVertices == 3 ) {
        vertrad[0] = basisGalerkinRadianceAtPoint(element, element->radiance, 0., 0.);
        vertrad[1] = basisGalerkinRadianceAtPoint(element, element->radiance, 1., 0.);
        vertrad[2] = basisGalerkinRadianceAtPoint(element, element->radiance, 0., 1.);
    } else {
        vertrad[0] = basisGalerkinRadianceAtPoint(element, element->radiance, 0., 0.);
        vertrad[1] = basisGalerkinRadianceAtPoint(element, element->radiance, 1., 0.);
        vertrad[2] = basisGalerkinRadianceAtPoint(element, element->radiance, 1., 1.);
        vertrad[3] = basisGalerkinRadianceAtPoint(element, element->radiance, 0., 1.);
    }

    if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
        COLOR rho = REFLECTIVITY(element->patch), ambient;

        colorProduct(rho, GLOBAL_galerkin_state.ambient_radiance, ambient);
        for ( i = 0; i < element->patch->numberOfVertices; i++ ) {
            colorAdd(vertrad[i], ambient, vertrad[i]);
        }
    }

    for ( i = 0; i < element->patch->numberOfVertices; i++ ) {
        RGB col;
        radianceToRgb(vertrad[i], &col);
        galerkinWriteVertexColor(&col);
    }
}

static void
galerkinWriteVertexColors() {
    globalVertexId = globalNumberOfWrites = 0;
    fprintf(globalVrmlFileDescriptor, "\tcolor Color {\n\t  color [ ");
    ForAllLeafElements(GLOBAL_galerkin_state.top_cluster, galerkinWriteVertexColors);
    fprintf(globalVrmlFileDescriptor, " ] ");
    fprintf(globalVrmlFileDescriptor, "\n\t}\n");
}

static void
galerkinWriteColors() {
    if ( !GLOBAL_render_renderOptions.smooth_shading ) {
        logWarning(nullptr, "I assume you want a smooth shaded model ...");
    }
    fprintf(globalVrmlFileDescriptor, "\tcolorPerVertex %s\n", "TRUE");
    galerkinWriteVertexColors();
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
galerkinWriteCoordIndices(GalerkinElement *elem) {
    int i;
    for ( i = 0; i < elem->patch->numberOfVertices; i++ ) {
        galerkinWriteCoordIndex(globalVertexId++);
    }
    galerkinWriteCoordIndex(-1);
}

static void
galerkinWriteCoordIndices() {
    globalVertexId = globalNumberOfWrites = 0;
    fprintf(globalVrmlFileDescriptor, "\tcoordIndex [ ");
    ForAllLeafElements(GLOBAL_galerkin_state.top_cluster, galerkinWriteCoordIndices);
    fprintf(globalVrmlFileDescriptor, " ]\n");
}

static void
galerkinWriteVRML(FILE *fp) {
    WriteVRMLHeader(fp);

    globalVrmlFileDescriptor = fp;
    galerkinWriteCoords();
    galerkinWriteColors();
    galerkinWriteCoordIndices();

    WriteVRMLTrailer(fp);
}

/* **************************************************************** */
RADIANCEMETHOD GalerkinRadiosity = {
    "Galerkin",
    3,
    "Galerkin Radiosity",
    galerkinDefaults,
    parseGalerkinOptions,
    printGalerkinOptions,
    initGalerkin,
    doGalerkinOneStep,
    terminateGalerkin,
    getRadiance,
    createPatchData,
    printPatchData,
    destroyPatchData,
    getGalerkinStats,
    galerkinRender,
    (void (*)(void)) nullptr,
    galerkinUpdateMaterial,
    galerkinWriteVRML
};
