/**
Monte Carlo Radiosity: common code for stochastic relaxation and random walks
*/

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/Statistics.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "skin/Patch.h"
#include "skin/Vertex.h"
#include "render/potential.h"
#include "scene/Camera.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/mcradP.h"

STATE GLOBAL_stochasticRaytracing_monteCarloRadiosityState;

/**
Common routines for stochastic relaxation and random walks
*/
void
monteCarloRadiosityDefaults() {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.rayUnitsPerIt = 10;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.bidirectionalTransfers = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sequence = NIEDERREITER;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType = StochasticRaytracingApproximation::CONSTANT;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven = true;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorType = RW_SHOOTING;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorKind = RW_COLLISION;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkNumLast = 1;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.discardIncremental = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.naiveMerging = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show = SHOW_TOTAL_RADIANCE;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot = false;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialLightSourceSamples = 1000;

    elementHierarchyDefaults();
    monteCarloRadiosityInitBasis();
}

/**
For counting how much CPU time was used for the computations
*/
void
monteCarloRadiosityUpdateCpuSecs() {
    clock_t t;

    t = clock();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds += (float) (t - GLOBAL_stochasticRaytracing_monteCarloRadiosityState.lastClock) / (float) CLOCKS_PER_SEC;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.lastClock = t;
}

Element *
monteCarloRadiosityCreatePatchData(Patch *patch) {
    patch->radianceData = stochasticRadiosityElementCreateFromPatch(patch);
    return patch->radianceData;
}

void
monteCarloRadiosityDestroyPatchData(Patch *patch) {
    if ( patch->radianceData ) {
        stochasticRadiosityElementDestroy(topLevelStochasticRadiosityElement(patch));
    }
    patch->radianceData = nullptr;
}

/**
Compute new color for the patch: fine if no hierarchical refinement is used, e.g.
in the current random walk radiosity implementation
*/
void
monteCarloRadiosityPatchComputeNewColor(Patch *patch) {
    patch->color = stochasticRadiosityElementColor(topLevelStochasticRadiosityElement(patch));
    patch->computeVertexColors();
}

/**
Initializes the computations for the current scene (if any): initialisations
are delayed to just before the first iteration step, see ReInit() below
*/
void
monteCarloRadiosityInit() {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited = false;
}

/**
Initialises patch data
*/
static void
monteCarloRadiosityInitPatch(const Patch *patch) {
    ColorRgb Ed = topLevelStochasticRadiosityElement(patch)->Ed;

    reAllocCoefficients(topLevelStochasticRadiosityElement(patch));
    stochasticRadiosityClearCoefficients(getTopLevelPatchRad(patch), getTopLevelPatchBasis(patch));
    stochasticRadiosityClearCoefficients(getTopLevelPatchUnShotRad(patch), getTopLevelPatchBasis(patch));
    stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(patch), getTopLevelPatchBasis(patch));

    getTopLevelPatchRad(patch)[0] = getTopLevelPatchUnShotRad(patch)[0] = topLevelStochasticRadiosityElement(patch)->sourceRad = Ed;
    getTopLevelPatchReceivedRad(patch)[0].clear();

    topLevelStochasticRadiosityElement(patch)->rayIndex = patch->id * 11;
    topLevelStochasticRadiosityElement(patch)->quality = 0.0;
    topLevelStochasticRadiosityElement(patch)->ng = 0;
    topLevelStochasticRadiosityElement(patch)->importance = 0.0;
    topLevelStochasticRadiosityElement(patch)->unShotImportance = 0.0;
    topLevelStochasticRadiosityElement(patch)->receivedImportance = 0.0;
    topLevelStochasticRadiosityElement(patch)->sourceImportance = 0.0;
}

/**
Routines below update/re-initialise importance after a viewing change
*/
static void
monteCarloRadiosityPullImportances(Element *element) {
    StochasticRadiosityElement *child = (StochasticRadiosityElement *)element;
    StochasticRadiosityElement *parent = (StochasticRadiosityElement *)child->parent;
    stochasticRadiosityElementPullImportance(parent, child, &parent->importance, &child->importance);
    stochasticRadiosityElementPullImportance(parent, child, &parent->sourceImportance, &child->sourceImportance);
    stochasticRadiosityElementPullImportance(parent, child, &parent->unShotImportance, &child->unShotImportance);
}

static void
monteCarloRadiosityAccumulateImportances(const StochasticRadiosityElement *elem) {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp += elem->area * elem->importance;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp += elem->area * elem->sourceImportance;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp += elem->area * java::Math::abs(elem->unShotImportance);
}

/**
Update importance in the element hierarchy starting with the top cluster
*/
static void
monteCarloRadiosityUpdateImportance(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }

    if ( !stochasticRadiosityElement->traverseAllChildren(monteCarloRadiosityUpdateImportance) ) {
        // Leaf element
        float delta_imp = (float)(stochasticRadiosityElement->patch->isVisible() ? 1.0 : 0.0) - stochasticRadiosityElement->sourceImportance;
        stochasticRadiosityElement->importance += delta_imp;
        stochasticRadiosityElement->sourceImportance += delta_imp;
        stochasticRadiosityElement->unShotImportance += delta_imp;
        monteCarloRadiosityAccumulateImportances(stochasticRadiosityElement);
    } else {
        // Not a leaf element: clear & pull importance
        stochasticRadiosityElement->importance = stochasticRadiosityElement->sourceImportance = stochasticRadiosityElement->unShotImportance = 0.0;
        stochasticRadiosityElement->traverseAllChildren(monteCarloRadiosityPullImportances);
    }
}

/**
Re-init importance in the element hierarchy starting with the top cluster
*/
static void
monteCarloRadiosityReInitImportance(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }

    if ( !stochasticRadiosityElement->traverseAllChildren(monteCarloRadiosityReInitImportance) ) {
        // Leaf element
        stochasticRadiosityElement->importance = (stochasticRadiosityElement->patch->isVisible()) ? 1.0 : 0.0;
        stochasticRadiosityElement->sourceImportance = stochasticRadiosityElement->importance;
        stochasticRadiosityElement->unShotImportance = stochasticRadiosityElement->importance;
        monteCarloRadiosityAccumulateImportances(stochasticRadiosityElement);
    } else {
        // Not a leaf element: clear & pull importance
        stochasticRadiosityElement->importance = stochasticRadiosityElement->sourceImportance = stochasticRadiosityElement->unShotImportance = 0.0;
        stochasticRadiosityElement->traverseAllChildren(monteCarloRadiosityPullImportances);
    }
}

void
monteCarloRadiosityUpdateViewImportance(Scene *scene, const RenderOptions *renderOptions) {
    fprintf(stderr, "Updating direct visibility ... \n");

    updateDirectVisibility(scene, renderOptions);

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.0;
    monteCarloRadiosityUpdateImportance(GLOBAL_stochasticRaytracing_hierarchy.topCluster);

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp < GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp ) {
        fprintf(stderr, "Importance will be recomputed incrementally.\n");
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch = false;
    } else {
        fprintf(stderr, "Importance will be recomputed from scratch.\n");
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch = true;

        // Re-compute from scratch
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp = 0.0;
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.0;
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.0;
        monteCarloRadiosityReInitImportance(GLOBAL_stochasticRaytracing_hierarchy.topCluster);
    }

    scene->camera->changed = false; // Indicate that direct importance has been computed for this view already
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays = 0; // Start over
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated = true;
}

/**
Computes max_i (A_T/A_i): the ratio of the total area over the minimal patch
area in the scene, ignoring the 10% area occupied by the smallest patches
*/
static double
monteCarloRadiosityDetermineAreaFraction(
    const java::ArrayList<Patch *> *scenePatches,
    const java::ArrayList<Geometry *> *sceneGeometries)
{
    float *areas;
    float cumulative;
    float areaFrac;
    int numberOfPatchIds = Patch::getNextId();
    int i;

    if ( sceneGeometries == nullptr || sceneGeometries->size() == 0 ) {
        // An arbitrary positive number (in order to avoid divisions by zero
        return 100;
    }

    // Build a table of patch areas
    areas = (float *)malloc(numberOfPatchIds * sizeof(float));
    for ( i = 0; i < numberOfPatchIds; i++ ) {
        areas[i] = 0.0;
    }
    for ( i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        areas[patch->id] = patch->area;
    }

    // Sort the table to decreasing areas
    qsort((void *) areas,
        numberOfPatchIds,
        sizeof(float),
        (QSORT_CALLBACK_TYPE) Numeric::floatCompare);

    // Find the patch such that 10% of the total surface area is filled by smaller patches
    for ( i = numberOfPatchIds - 1, cumulative = 0.0; i >= 0 && cumulative < GLOBAL_statistics.totalArea * 0.1; i-- ) {
        cumulative += areas[i];
    }
    areaFrac = (i >= 0 && areas[i] > 0.0) ? GLOBAL_statistics.totalArea / areas[i] : (float)GLOBAL_statistics.numberOfPatches;

    free(areas);

    return areaFrac;
}

/**
Determines elementary ray power for the initial incremental iterations
*/
static void
monteCarloRadiosityDetermineInitialNrRays(
    const java::ArrayList<Patch *> *scenePatches,
    const java::ArrayList<Geometry *> *sceneGeometries)
{
    double areaFrac = monteCarloRadiosityDetermineAreaFraction(scenePatches, sceneGeometries);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays = (long) ((double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.rayUnitsPerIt * areaFrac);
}

/**
Really initialises: before the first iteration step
*/
void
monteCarloRadiosityReInit(Scene *scene, const RenderOptions *renderOptions) {
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited ) {
        return;
    }

    fprintf(stderr, "Initialising Monte Carlo radiosity ...\n");

    setSequence4D(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sequence);

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited = true;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.lastClock = clock();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration = 0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses = 0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevImportanceTracedRays = 0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.setSource = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedPaths = 0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance.clear();

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.clear();
    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        Patch *patch = scene->patchList->get(i);
        monteCarloRadiosityInitPatch(patch);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux,
            (float)M_PI * patch->area,
            getTopLevelPatchUnShotRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux,
            (float)M_PI * patch->area,
            getTopLevelPatchRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux,
            (float)M_PI * patch->area *
            (topLevelStochasticRadiosityElement(patch)->importance - topLevelStochasticRadiosityElement(patch)->sourceImportance),
            getTopLevelPatchUnShotRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp += patch->area * java::Math::abs(topLevelStochasticRadiosityElement(patch)->unShotImportance);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp += patch->area * topLevelStochasticRadiosityElement(patch)->importance;
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp += patch->area * topLevelStochasticRadiosityElement(patch)->sourceImportance;
        monteCarloRadiosityPatchComputeNewColor(patch);
    }

    monteCarloRadiosityDetermineInitialNrRays(scene->patchList, scene->geometryList);

    elementHierarchyInit(scene->clusteredRootGeometry);

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        monteCarloRadiosityUpdateViewImportance(scene, renderOptions);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch = true;
    }
}

void
monteCarloRadiosityPreStep(Scene *scene, const RenderOptions *renderOptions) {
    if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited ) {
        monteCarloRadiosityReInit(scene, renderOptions);
    }
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven && scene->camera->changed ) {
        monteCarloRadiosityUpdateViewImportance(scene, renderOptions);
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.lastClock = clock();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration++;
}

/**
Undoes the effect of mainInitApplication() and all side-effects of Step()
*/
void
monteCarloRadiosityTerminate(const java::ArrayList<Patch *> *scenePatches) {
    elementHierarchyTerminate(scenePatches);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited = false;
}

static ColorRgb
monteCarloRadiosityDiffuseReflectanceAtPoint(Patch *patch, double u, double v) {
    RayHit hit;
    Vector3D point;
    patch->uniformPoint(u, v, &point);
    hit.init(patch, &point, &patch->normal, patch->material);
    hit.setUv(u, v);
    unsigned int newFlags = hit.getFlags() | RayHitFlag::UV;
    hit.setFlags(newFlags);
    ColorRgb result;
    result.clear();
    if ( hit.getMaterial()->getBsdf() != nullptr ) {
        result = hit.getMaterial()->getBsdf()->splitBsdfScatteredPower(&hit, BRDF_DIFFUSE_COMPONENT);
    }
    return result;
}

static ColorRgb
vertexReflectance(const Vertex *v) {
    int count = 0;
    ColorRgb rd;

    rd.clear();
    for ( int i = 0; v->radianceData != nullptr && i < v->radianceData->size(); i++ ) {
        Element *genericElement = v->radianceData->get(i);
        if ( genericElement->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        const StochasticRadiosityElement *element = (StochasticRadiosityElement *)genericElement;
        if ( !element->regularSubElements ) {
            rd.add(rd, element->Rd);
            count++;
        }
    }

    if ( count > 0 ) {
        rd.scaleInverse((float) count, rd);
    }

    return rd;
}

static ColorRgb
monteCarloRadiosityInterpolatedReflectanceAtPoint(const StochasticRadiosityElement *leaf, double u, double v) {
    static const StochasticRadiosityElement *cachedLeaf = nullptr;
    static ColorRgb vrd[4];
    static ColorRgb rd;

    if ( leaf != nullptr ) {
        if ( leaf != cachedLeaf ) {
            for ( int i = 0; i < leaf->numberOfVertices; i++ ) {
                vrd[i] = vertexReflectance(leaf->vertices[i]);
            }
        }
        cachedLeaf = leaf;

        rd.clear();
        switch ( leaf->numberOfVertices ) {
            case 3:
                rd.interpolateBarycentric(vrd[0], vrd[1], vrd[2], (float) u, (float) v);
                break;
            case 4:
                rd.interpolateBiLinear(vrd[0], vrd[1], vrd[2], vrd[3], (float) u, (float) v);
                break;
            default:
                logFatal(-1, "monteCarloRadiosityInterpolatedReflectanceAtPoint", "Invalid nr of vertices %d",
                         leaf->numberOfVertices);
        }
    }
    return rd;
}

/**
Returns the radiance emitted from the patch at the point with parameters
(u,v) into the direction 'dir'
*/
ColorRgb
monteCarloRadiosityGetRadiance(Patch *patch, double u, double v, Vector3D /*dir*/, const RenderOptions *renderOptions) {
    ColorRgb TrueRdAtPoint = monteCarloRadiosityDiffuseReflectanceAtPoint(patch, u, v);
    const StochasticRadiosityElement *leaf = stochasticRadiosityElementRegularLeafElementAtPoint(
        topLevelStochasticRadiosityElement(patch), &u, &v);
    ColorRgb UsedRdAtPoint = renderOptions->smoothShading ? monteCarloRadiosityInterpolatedReflectanceAtPoint(leaf, u, v) : leaf->Rd;
    ColorRgb radianceAtPoint = stochasticRadiosityElementDisplayRadianceAtPoint(leaf, u, v, renderOptions);
    ColorRgb sourceRad;
    sourceRad.clear();

    // Subtract source radiance
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.show != SHOW_INDIRECT_RADIANCE ) {
        // source_rad is self-emitted radiance if !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly. It is direct
        // illumination if GLOBAL_stochasticRaytracing_monteCarloRadiosityState.direct_only
        if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot ) {
            sourceRad = leaf->sourceRad;
        }
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly || GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot ) {
            // Subtract self-emitted radiance
            sourceRad.add(sourceRad, leaf->Ed);
        }
    }
    radianceAtPoint.subtract(radianceAtPoint, sourceRad);

    radianceAtPoint.scalarProduct(radianceAtPoint, TrueRdAtPoint);
    radianceAtPoint.divide(radianceAtPoint, UsedRdAtPoint);

    // Re-add source radiance
    radianceAtPoint.add(radianceAtPoint, sourceRad);

    return radianceAtPoint;
}

/**
Returns scalar reflectance, for importance propagation
*/
float
monteCarloRadiosityScalarReflectance(const Patch *P) {
    return stochasticRadiosityElementScalarReflectance(topLevelStochasticRadiosityElement(P));
}

#endif
