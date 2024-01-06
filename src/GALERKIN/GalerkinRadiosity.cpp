/* galerkin.h: Galerkin radiosity, with or without hierarchical refinement, with or
 *	       without clusters, with Jacobi, Gauss-Seidel or Southwell iterations,
 *	       potential-dirven or not ...
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

/* installs cubature rules for triangles and quadrilaterals of the specified degree */
void
SetCubatureRules(CUBARULE **trirule, CUBARULE **quadrule, CUBATURE_DEGREE degree) {
    switch ( degree ) {
        case DEGREE_1:
            *trirule = &CRT1;
            *quadrule = &CRQ1;
            break;
        case DEGREE_2:
            *trirule = &CRT2;
            *quadrule = &CRQ2;
            break;
        case DEGREE_3:
            *trirule = &CRT3;
            *quadrule = &CRQ3;
            break;
        case DEGREE_4:
            *trirule = &CRT4;
            *quadrule = &CRQ4;
            break;
        case DEGREE_5:
            *trirule = &CRT5;
            *quadrule = &CRQ5;
            break;
        case DEGREE_6:
            *trirule = &CRT7;
            *quadrule = &CRQ6;
            break;
        case DEGREE_7:
            *trirule = &CRT7;
            *quadrule = &CRQ7;
            break;
        case DEGREE_8:
            *trirule = &CRT8;
            *quadrule = &CRQ8;
            break;
        case DEGREE_9:
            *trirule = &CRT9;
            *quadrule = &CRQ9;
            break;
        case DEGREE_3_PROD:
            *trirule = &CRT5;
            *quadrule = &CRQ3PG;
            break;
        case DEGREE_5_PROD:
            *trirule = &CRT7;
            *quadrule = &CRQ5PG;
            break;
        case DEGREE_7_PROD:
            *trirule = &CRT9;
            *quadrule = &CRQ7PG;
            break;
        default:
            logFatal(2, "SetCubatureRules", "Invalid degree %d", degree);
    }
}

static void
GalerkinDefaults() {
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
    SetCubatureRules(&GLOBAL_galerkin_state.rcv3rule, &GLOBAL_galerkin_state.rcv4rule, GLOBAL_galerkin_state.rcv_degree);
    SetCubatureRules(&GLOBAL_galerkin_state.src3rule, &GLOBAL_galerkin_state.src4rule, GLOBAL_galerkin_state.src_degree);
    GLOBAL_galerkin_state.clusRule = &CRV1;
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
IterationMethodOption(void *value) {
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
HierarchicalOption(void *value) {
    int yesno = *(int *) value;
    GLOBAL_galerkin_state.hierarchical = yesno;
}

static void
LazyOption(void *value) {
    int yesno = *(int *) value;
    GLOBAL_galerkin_state.lazy_linking = yesno;
}

static void
ClusteringOption(void *value) {
    int yesno = *(int *) value;
    GLOBAL_galerkin_state.clustered = yesno;
}

static void
ImportanceOption(void *value) {
    int yesno = *(int *) value;
    GLOBAL_galerkin_state.importance_driven = yesno;
}

static void
AmbientOption(void *value) {
    int yesno = *(int *) value;
    GLOBAL_galerkin_state.use_ambient_radiance = yesno;
}

static CMDLINEOPTDESC galerkinOptions[] = {
        {"-gr-iteration-method",     6,  Tstring,  nullptr,            IterationMethodOption,
                "-gr-iteration-method <methodname>: Jacobi, GaussSeidel, Southwell"},
        {"-gr-hierarchical",         6,  TYPELESS, (void *) &t,  HierarchicalOption,
                "-gr-hierarchical    \t: do hierarchical refinement"},
        {"-gr-not-hierarchical",     10, TYPELESS, (void *) &f, HierarchicalOption,
                "-gr-not-hierarchical\t: don't do hierarchical refinement"},
        {"-gr-lazy-linking",         6,  TYPELESS, (void *) &t,  LazyOption,
                "-gr-lazy-linking    \t: do lazy linking"},
        {"-gr-no-lazy-linking",      10, TYPELESS, (void *) &f, LazyOption,
                "-gr-no-lazy-linking \t: don't do lazy linking"},
        {"-gr-clustering",           6,  TYPELESS, (void *) &t,  ClusteringOption,
                "-gr-clustering      \t: do clustering"},
        {"-gr-no-clustering",        10, TYPELESS, (void *) &f, ClusteringOption,
                "-gr-no-clustering   \t: don't do clustering"},
        {"-gr-importance",           6,  TYPELESS, (void *) &t,  ImportanceOption,
                "-gr-importance      \t: do view-potential driven computations"},
        {"-gr-no-importance",        10, TYPELESS, (void *) &f, ImportanceOption,
                "-gr-no-importance   \t: don't use view-potential"},
        {"-gr-ambient",              6,  TYPELESS, (void *) &t,  AmbientOption,
                "-gr-ambient         \t: do visualisation with ambient term"},
        {"-gr-no-ambient",           10, TYPELESS, (void *) &f, AmbientOption,
                "-gr-no-ambient      \t: do visualisation without ambient term"},
        {"-gr-link-error-threshold", 6,  Tfloat,   &GLOBAL_galerkin_state.rel_link_error_threshold, DEFAULT_ACTION,
                "-gr-link-error-threshold <float>: Relative link error threshold"},
        {"-gr-min-elem-area",        6,  Tfloat,   &GLOBAL_galerkin_state.rel_min_elem_area,        DEFAULT_ACTION,
                "-gr-min-elem-area <float> \t: Relative element area threshold"},
        {nullptr,                       0,  TYPELESS, nullptr,                          DEFAULT_ACTION,
                nullptr}
};

static void
ParseGalerkinOptions(int *argc, char **argv) {
    ParseOptions(galerkinOptions, argc, argv);
}

static void
PrintGalerkinOptions(FILE *fp) {
}

/* for counting how much CPU time was used for the computations */
static void
UpdateCpuSecs() {
    clock_t t;

    t = clock();
    GLOBAL_galerkin_state.cpu_secs += (float) (t - GLOBAL_galerkin_state.lastclock) / (float) CLOCKS_PER_SEC;
    GLOBAL_galerkin_state.lastclock = t;
}

/* radiance data for a PATCH is a surface element. */
static void *
CreatePatchData(PATCH *patch) {
    return patch->radiance_data = (void *) CreateToplevelElement(patch);
}

static void
PrintPatchData(FILE *out, PATCH *patch) {
    PrintElement(out, (ELEMENT *) patch->radiance_data);
}

static void
DestroyPatchData(PATCH *patch) {
    DestroyToplevelElement((ELEMENT *) patch->radiance_data);
    patch->radiance_data = (void *) nullptr;
}

void
PatchRecomputeColor(PATCH *patch) {
    COLOR rho = REFLECTIVITY(patch);
    COLOR rad_vis;

    /* compute the patches color based on its radiance + ambient radiance
     * if desired. */
    if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
        colorProduct(rho, GLOBAL_galerkin_state.ambient_radiance, rad_vis);
        colorAdd(rad_vis, RADIANCE(patch), rad_vis);
        RadianceToRGB(rad_vis, &patch->color);
    } else {
        RadianceToRGB(RADIANCE(patch), &patch->color);
    }
    PatchComputeVertexColors(patch);
}

static void
PatchInit(PATCH *patch) {
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
                POTENTIAL(patch).f = patch->direct_potential;
                break;
            case SOUTHWELL:
                POTENTIAL(patch).f = UNSHOT_POTENTIAL(patch).f = patch->direct_potential;
                break;
            default:
                logFatal(-1, "PatchInit", "Invalid iteration method");
        }
    }

    PatchRecomputeColor(patch);
}

void
InitGalerkin() {
    GLOBAL_galerkin_state.iteration_nr = 0;
    GLOBAL_galerkin_state.cpu_secs = 0.;

    InitBasis();

    GLOBAL_galerkin_state.constant_radiance = GLOBAL_statistics_estimatedAverageRadiance;
    if ( GLOBAL_galerkin_state.use_constant_radiance ) {
        colorClear(GLOBAL_galerkin_state.ambient_radiance);
    } else {
        GLOBAL_galerkin_state.ambient_radiance = GLOBAL_statistics_estimatedAverageRadiance;
    }

    PatchListIterate(GLOBAL_scene_patches, PatchInit);

    GLOBAL_galerkin_state.top_geom = GLOBAL_scene_clusteredWorldGeom;
    GLOBAL_galerkin_state.top_cluster = galerkinCreateClusterHierarchy(GLOBAL_galerkin_state.top_geom);

    /* create a scratch software renderer for various operations on clusters */
    ScratchInit();

    /* global variables used for formfactor computation optimisation */
    GLOBAL_galerkin_state.fflastrcv = GLOBAL_galerkin_state.fflastsrc = (ELEMENT *) nullptr;

    /* global variables for scratch rendering */
    GLOBAL_galerkin_state.lastclusid = -1;
    VECTORSET(GLOBAL_galerkin_state.lasteye, HUGE, HUGE, HUGE);
}

static int
DoGalerkinOneStep() {
    int done = false;

    if ( GLOBAL_galerkin_state.iteration_nr < 0 ) {
        logError("DoGalerkinOneStep", "method not initialized");
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
                done = DoClusteredGatheringIteration();
            } else {
                done = DoGatheringIteration();
            }
            break;
        case SOUTHWELL:
            done = DoShootingStep();
            break;
        default:
            logFatal(2, "DoGalerkinOneStep", "Invalid iteration method %d\n", GLOBAL_galerkin_state.iteration_method);
    }

    UpdateCpuSecs();

    return done;
}

static void
TerminateGalerkin() {
    ScratchTerminate();
    galerkinDestroyClusterHierarchy(GLOBAL_galerkin_state.top_cluster);
}

static COLOR
GetRadiance(PATCH *patch, double u, double v, Vector3D dir) {
    ELEMENT *leaf;
    COLOR rad;

    if ( patch->jacobian ) {
        BilinearToUniform(patch, &u, &v);
    }

    leaf = RegularLeafElementAtPoint(TOPLEVEL_ELEMENT(patch), &u, &v);

    rad = RadianceAtPoint(leaf, leaf->radiance, u, v);

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
GetGalerkinStats() {
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
RenderElementHierarchy(ELEMENT *elem) {
    if ( !elem->regular_subelements ) {
        RenderElement(elem);
    } else ITERATE_REGULAR_SUBELEMENTS(elem, RenderElementHierarchy);
}

static void
GalerkinRenderPatch(PATCH *patch) {
    RenderElementHierarchy(TOPLEVEL_ELEMENT(patch));
}

void
GalerkinRender() {
    if ( renderopts.frustum_culling ) {
        RenderWorldOctree(GalerkinRenderPatch);
    } else PatchListIterate(GLOBAL_scene_patches, GalerkinRenderPatch);
}

static void
GalerkinUpdateMaterial(MATERIAL *oldmaterial, MATERIAL *newmaterial) {
    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        ShootingUpdateMaterial(oldmaterial, newmaterial);
    } else if ( GLOBAL_galerkin_state.iteration_method == JACOBI ||
                GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ) {
        GatheringUpdateMaterial(oldmaterial, newmaterial);
    } else {
        fprintf(stderr, "GalerkinUpdateMaterial: not yet implemented.\n");
    }

    RenderScene();
}

/* *********************************************************** */
/* VRML output */
static FILE *vrmlfp;
static int nwrit, vid;

static void
WriteVertexCoord(Vector3D *p) {
    if ( nwrit > 0 ) {
        fprintf(vrmlfp, ", ");
    }
    nwrit++;
    if ( nwrit % 4 == 0 ) {
        fprintf(vrmlfp, "\n\t  ");
    }
    fprintf(vrmlfp, "%g %g %g", p->x, p->y, p->z);
    vid++;
}

static void
WriteVertexCoords(ELEMENT *elem) {
    Vector3D v[8];
    int i, nverts = ElementVertices(elem, v);
    for ( i = 0; i < nverts; i++ ) {
        WriteVertexCoord(&v[i]);
    }
}

static void
WriteCoords() {
    nwrit = vid = 0;
    fprintf(vrmlfp, "\tcoord Coordinate {\n\t  point [ ");
    ForAllLeafElements(GLOBAL_galerkin_state.top_cluster, WriteVertexCoords);
    fprintf(vrmlfp, " ] ");
    fprintf(vrmlfp, "\n\t}\n");
}

static void
WriteVertexColor(RGB *color) {
    /* not yet written */
    if ( nwrit > 0 ) {
        fprintf(vrmlfp, ", ");
    }
    nwrit++;
    if ( nwrit % 4 == 0 ) {
        fprintf(vrmlfp, "\n\t  ");
    }
    fprintf(vrmlfp, "%.3g %.3g %.3g", color->r, color->g, color->b);
    vid++;
}

static void
ElementWriteVertexColors(ELEMENT *element) {
    COLOR vertrad[4];
    int i;

    if ( element->pog.patch->nrvertices == 3 ) {
        vertrad[0] = RadianceAtPoint(element, element->radiance, 0., 0.);
        vertrad[1] = RadianceAtPoint(element, element->radiance, 1., 0.);
        vertrad[2] = RadianceAtPoint(element, element->radiance, 0., 1.);
    } else {
        vertrad[0] = RadianceAtPoint(element, element->radiance, 0., 0.);
        vertrad[1] = RadianceAtPoint(element, element->radiance, 1., 0.);
        vertrad[2] = RadianceAtPoint(element, element->radiance, 1., 1.);
        vertrad[3] = RadianceAtPoint(element, element->radiance, 0., 1.);
    }

    if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
        COLOR rho = REFLECTIVITY(element->pog.patch), ambient;

        colorProduct(rho, GLOBAL_galerkin_state.ambient_radiance, ambient);
        for ( i = 0; i < element->pog.patch->nrvertices; i++ ) {
            colorAdd(vertrad[i], ambient, vertrad[i]);
        }
    }

    for ( i = 0; i < element->pog.patch->nrvertices; i++ ) {
        RGB col;
        RadianceToRGB(vertrad[i], &col);
        WriteVertexColor(&col);
    }
}

static void
WriteVertexColors() {
    vid = nwrit = 0;
    fprintf(vrmlfp, "\tcolor Color {\n\t  color [ ");
    ForAllLeafElements(GLOBAL_galerkin_state.top_cluster, ElementWriteVertexColors);
    fprintf(vrmlfp, " ] ");
    fprintf(vrmlfp, "\n\t}\n");
}

static void
WriteColors() {
    if ( !renderopts.smooth_shading ) {
        logWarning(nullptr, "I assume you want a smooth shaded model ...");
    }
    fprintf(vrmlfp, "\tcolorPerVertex %s\n", "TRUE");
    WriteVertexColors();
}

static void
WriteCoordIndex(int index) {
    nwrit++;
    if ( nwrit % 20 == 0 ) {
        fprintf(vrmlfp, "\n\t  ");
    }
    fprintf(vrmlfp, "%d ", index);
}

static void
ElementWriteCoordIndices(ELEMENT *elem) {
    int i;
    for ( i = 0; i < elem->pog.patch->nrvertices; i++ ) {
        WriteCoordIndex(vid++);
    }
    WriteCoordIndex(-1);
}

static void
WriteCoordIndices() {
    vid = nwrit = 0;
    fprintf(vrmlfp, "\tcoordIndex [ ");
    ForAllLeafElements(GLOBAL_galerkin_state.top_cluster, ElementWriteCoordIndices);
    fprintf(vrmlfp, " ]\n");
}

static void
GalerkinWriteVRML(FILE *fp) {
    WriteVRMLHeader(fp);

    vrmlfp = fp;
    WriteCoords();
    WriteColors();
    WriteCoordIndices();

    WriteVRMLTrailer(fp);
}

/* **************************************************************** */
RADIANCEMETHOD GalerkinRadiosity = {
        "Galerkin", 3,
        "Galerkin Radiosity",
        "galerkinButton",
        GalerkinDefaults,
        ParseGalerkinOptions,
        PrintGalerkinOptions,
        InitGalerkin,
        DoGalerkinOneStep,
        TerminateGalerkin,
        GetRadiance,
        CreatePatchData,
        PrintPatchData,
        DestroyPatchData,
        CreateGalerkinControlPanel,
        (void (*)(void *)) nullptr,
        ShowGalerkinControlPanel,
        HideGalerkinControlPanel,
        GetGalerkinStats,
        GalerkinRender,
        (void (*)(void)) nullptr,
        GalerkinUpdateMaterial,
        GalerkinWriteVRML
};

void CreateGalerkinControlPanel(void *parent_widget) {}

void ShowGalerkinControlPanel() {}

void HideGalerkinControlPanel() {}
