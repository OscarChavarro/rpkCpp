/**
Monte Carlo Radiosity: common code for stochastic relaxation and random walks
*/

#include <stdlib.h>

#include "java/lang/System.h"
#include "material/RendererConfiguration.h"
#include "raycasting/stochasticRaytracing/Mcrad.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.txx"
#include "common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "render/Potential.h"
#include "raycasting/stochasticRaytracing/Hierarchy.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

static int
mcradQSortFloatCompare(const void *a, const void *b) {
    const float left = *((const float *)(a));
    const float right = *((const float *)(b));
    if ( Numeric::floatCompare(left, right) ) {
        return 1;
    }
    if ( Numeric::floatCompare(right, left) ) {
        return -1;
    }
    return 0;
}

/**
Common routines for stochastic relaxation and random walks
*/
void
Mcrad::monteCarloRadiosityDefaults() {
    StochasticRelaxation::activeState().inited = false;
    StochasticRelaxation::activeState().rayUnitsPerIt = 10;
    StochasticRelaxation::activeState().bidirectionalTransfers = false;
    StochasticRelaxation::activeState().constantControlVariate = false;
    StochasticRelaxation::activeState().controlRadiance.clear();
    StochasticRelaxation::activeState().indirectOnly = false;
    StochasticRelaxation::activeState().sequence = NIEDERREITER;
    StochasticRelaxation::activeState().approximationOrderType = CONSTANT;
    StochasticRelaxation::activeState().importanceDriven = false;
    StochasticRelaxation::activeState().radianceDriven = true;
    StochasticRelaxation::activeState().importanceUpdated = false;
    StochasticRelaxation::activeState().importanceUpdatedFromScratch = false;
    StochasticRelaxation::activeState().continuousRandomWalk = false;
    StochasticRelaxation::activeState().randomWalkEstimatorType = RW_SHOOTING;
    StochasticRelaxation::activeState().randomWalkEstimatorKind = RW_COLLISION;
    StochasticRelaxation::activeState().randomWalkNumLast = 1;
    StochasticRelaxation::activeState().weightedSampling = false;
    StochasticRelaxation::activeState().discardIncremental = false;
    StochasticRelaxation::activeState().incrementalUsesImportance = false;
    StochasticRelaxation::activeState().naiveMerging = false;
    StochasticRelaxation::activeState().show = SHOW_TOTAL_RADIANCE;
    StochasticRelaxation::activeState().doNonDiffuseFirstShot = false;
    StochasticRelaxation::activeState().initialLightSourceSamples = 1000;

    Hierarchy::elementHierarchyDefaults();
    Basismcrad::monteCarloRadiosityInitBasis();
}

/**
For counting how much CPU time was used for the computations
*/
void
Mcrad::mntCarloRadUpdCpuSecs() {
    const long t = System::nanoTime();
    StochasticRelaxation::activeState().cpuSeconds += ((float)(
        ((double)(t - StochasticRelaxation::activeState().lastClock)) / 1000000000.0));
    StochasticRelaxation::activeState().lastClock = t;
}

Element *
Mcrad::mntCarloRadCreatePtchData(Patch *patch) {
    patch->radianceData = StochasticRadiosityElement::stchsRadElemCreateFromPtch(patch);
    return patch->radianceData;
}

void
Mcrad::mntCarloRadDestroyPtchData(Patch *patch) {
    if ( patch->radianceData ) {
        StochasticRadiosityElement::stchsRadElemDestroy(McradP::topLvlStochRadElem(patch));
    }
    patch->radianceData = NULL;
}

/**
Compute new color for the patch: fine if no hierarchical refinement is used, e.g.
in the current random walk radiosity implementation
*/
void
Mcrad::mntCarloRadPtchCompNewClr(Patch *patch) {
    patch->color = StochasticRadiosityElement::stochasticRadiosityElementColor(McradP::topLvlStochRadElem(patch));
    patch->computeVertexColors();
}

/**
Initializes the computations for the current scene (if any): initialisations
are delayed to just before the first iteration step, see ReInit() below
*/
void
Mcrad::monteCarloRadiosityInit() {
    StochasticRelaxation::activeState().inited = false;
}

/**
Initialises patch data
*/
 void
Mcrad::monteCarloRadiosityInitPatch(const Patch *patch) {
    ColorRgb Ed = McradP::topLvlStochRadElem(patch)->Ed;

    Coefficientsmcrad::reAllocCoefficients(McradP::topLvlStochRadElem(patch));
    Coefficientsmcrad::stchsRadClearCoeff(McradP::getTopLevelPatchRad(patch), McradP::getTopLevelPatchBasis(patch));
    Coefficientsmcrad::stchsRadClearCoeff(McradP::getTopLevelPatchUnShotRad(patch), McradP::getTopLevelPatchBasis(patch));
    Coefficientsmcrad::stchsRadClearCoeff(McradP::getTopLevelPatchReceivedRad(patch), McradP::getTopLevelPatchBasis(patch));

    McradP::getTopLevelPatchRad(patch)[0] = McradP::getTopLevelPatchUnShotRad(patch)[0] = McradP::topLvlStochRadElem(patch)->sourceRad = Ed;
    McradP::getTopLevelPatchReceivedRad(patch)[0].clear();

    McradP::topLvlStochRadElem(patch)->rayIndex = patch->id * 11;
    McradP::topLvlStochRadElem(patch)->quality = 0.0;
    McradP::topLvlStochRadElem(patch)->ng = 0;
    McradP::topLvlStochRadElem(patch)->importance = 0.0;
    McradP::topLvlStochRadElem(patch)->unShotImportance = 0.0;
    McradP::topLvlStochRadElem(patch)->receivedImportance = 0.0;
    McradP::topLvlStochRadElem(patch)->sourceImportance = 0.0;
}

/**
Routines below update/re-initialise importance after a viewing change
*/
 void
Mcrad::mntCarloRadPullImps(Element *element) {
    StochasticRadiosityElement *child = ((StochasticRadiosityElement *)(element));
    StochasticRadiosityElement *parent = ((StochasticRadiosityElement *)(child->parent));
    StochasticRadiosityElement::stchsRadElemPullImp(parent, child, &parent->importance, &child->importance);
    StochasticRadiosityElement::stchsRadElemPullImp(parent, child, &parent->sourceImportance, &child->sourceImportance);
    StochasticRadiosityElement::stchsRadElemPullImp(parent, child, &parent->unShotImportance, &child->unShotImportance);
}

 void
Mcrad::mntCarloRadAccumImps(const StochasticRadiosityElement *elem) {
    StochasticRelaxation::activeState().totalYmp += elem->area * elem->importance;
    StochasticRelaxation::activeState().sourceYmp += elem->area * elem->sourceImportance;
    StochasticRelaxation::activeState().unShotYmp += elem->area * Math::abs(elem->unShotImportance);
}

/**
Update importance in the element hierarchy starting with the top cluster
*/
 void
Mcrad::mntCarloRadUpdImp(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = ((StochasticRadiosityElement *)(element));

    if ( stochasticRadiosityElement == NULL ) {
        return;
    }

    if ( !stochasticRadiosityElement->traverseAllChildren(mntCarloRadUpdImp) ) {
        // Leaf element
        float delta_imp = ((float)(stochasticRadiosityElement->patch->isVisible() ? 1.0 : 0.0)) - stochasticRadiosityElement->sourceImportance;
        stochasticRadiosityElement->importance += delta_imp;
        stochasticRadiosityElement->sourceImportance += delta_imp;
        stochasticRadiosityElement->unShotImportance += delta_imp;
        mntCarloRadAccumImps(stochasticRadiosityElement);
    } else {
        // Not a leaf element: clear & pull importance
        stochasticRadiosityElement->importance = stochasticRadiosityElement->sourceImportance = stochasticRadiosityElement->unShotImportance = 0.0;
        stochasticRadiosityElement->traverseAllChildren(mntCarloRadPullImps);
    }
}

/**
Re-init importance in the element hierarchy starting with the top cluster
*/
 void
Mcrad::mntCarloRadReInitImp(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = ((StochasticRadiosityElement *)(element));

    if ( stochasticRadiosityElement == NULL ) {
        return;
    }

    if ( !stochasticRadiosityElement->traverseAllChildren(mntCarloRadReInitImp) ) {
        // Leaf element
        stochasticRadiosityElement->importance = (stochasticRadiosityElement->patch->isVisible()) ? 1.0 : 0.0;
        stochasticRadiosityElement->sourceImportance = stochasticRadiosityElement->importance;
        stochasticRadiosityElement->unShotImportance = stochasticRadiosityElement->importance;
        mntCarloRadAccumImps(stochasticRadiosityElement);
    } else {
        // Not a leaf element: clear & pull importance
        stochasticRadiosityElement->importance = stochasticRadiosityElement->sourceImportance = stochasticRadiosityElement->unShotImportance = 0.0;
        stochasticRadiosityElement->traverseAllChildren(mntCarloRadPullImps);
    }
}

void
Mcrad::mntCarloRadUpdViewImp(Scene *scene, const RenderOptions *renderOptions) {
    System::err.printf("Updating direct visibility ... \n");

    Potential::updateDirectVisibility(scene, renderOptions);

    StochasticRelaxation::activeState().sourceYmp = 0.0;
    StochasticRelaxation::activeState().unShotYmp = 0.0;
    StochasticRelaxation::activeState().totalYmp = 0.0;
    mntCarloRadUpdImp(ElementHierarchyState::activeState().topCluster);

    if ( StochasticRelaxation::activeState().unShotYmp < StochasticRelaxation::activeState().sourceYmp ) {
        System::err.printf("Importance will be recomputed incrementally.\n");
        StochasticRelaxation::activeState().importanceUpdatedFromScratch = false;
    } else {
        System::err.printf("Importance will be recomputed from scratch.\n");
        StochasticRelaxation::activeState().importanceUpdatedFromScratch = true;

        // Re-compute from scratch
        StochasticRelaxation::activeState().sourceYmp = 0.0;
        StochasticRelaxation::activeState().unShotYmp = 0.0;
        StochasticRelaxation::activeState().totalYmp = 0.0;
        mntCarloRadReInitImp(ElementHierarchyState::activeState().topCluster);
    }

    scene->camera->changed = false; // Indicate that direct importance has been computed for this view already
    StochasticRelaxation::activeState().importanceTracedRays = 0; // Start over
    StochasticRelaxation::activeState().importanceUpdated = true;
}

/**
Computes max_i (A_T/A_i): the ratio of the total area over the minimal patch
area in the scene, ignoring the 10% area occupied by the smallest patches
*/
 double
Mcrad::mntCarloRadDetAreaFrac(
    const ArrayList<Patch *> *scenePatches,
    const ArrayList<Geometry *> *sceneGeometries)
{
    float *areas;
    float cumulative;
    float areaFrac;
    int numberOfPatchIds = Patch::getNextId();
    int i;

    if ( sceneGeometries == NULL || sceneGeometries->size() == 0 ) {
        // An arbitrary positive number (in order to avoid divisions by zero
        return 100;
    }

    // Build a table of patch areas
    areas = new float[numberOfPatchIds];
    for ( i = 0; i < numberOfPatchIds; i++ ) {
        areas[i] = 0.0;
    }
    for ( i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        areas[patch->id] = patch->area;
    }

    // Sort the table to decreasing areas
    qsort(areas,
        numberOfPatchIds,
        sizeof(float),
        mcradQSortFloatCompare);

    // Find the patch such that 10% of the total surface area is filled by smaller patches
    for ( i = numberOfPatchIds - 1, cumulative = 0.0; i >= 0 && cumulative < Statistics::instance().radiance.totalArea * 0.1; i-- ) {
        cumulative += areas[i];
    }
    areaFrac = (i >= 0 && areas[i] > 0.0) ? Statistics::instance().radiance.totalArea / areas[i] : ((float)(Statistics::instance().reader.numberOfPatches));

    delete[] areas;

    return areaFrac;
}

/**
Determines elementary ray power for the initial incremental iterations
*/
 void
Mcrad::mntCarloRadDetInitNrRays(
    const ArrayList<Patch *> *scenePatches,
    const ArrayList<Geometry *> *sceneGeometries)
{
    double areaFrac = mntCarloRadDetAreaFrac(scenePatches, sceneGeometries);
    StochasticRelaxation::activeState().initialNumberOfRays = ((long)(((double)(StochasticRelaxation::activeState().rayUnitsPerIt)) * areaFrac));
}

/**
Really initialises: before the first iteration step
*/
void
Mcrad::monteCarloRadiosityReInit(Scene *scene, const RenderOptions *renderOptions) {
    if ( StochasticRelaxation::activeState().inited ) {
        return;
    }

    System::err.printf("Initialising Monte Carlo radiosity ...\n");

    Sample4d::setSequence4D(StochasticRelaxation::activeState().sequence);

    StochasticRelaxation::activeState().inited = true;
    StochasticRelaxation::activeState().cpuSeconds = 0.0;
    StochasticRelaxation::activeState().lastClock = System::nanoTime();
    StochasticRelaxation::activeState().currentIteration = 0;
    StochasticRelaxation::activeState().tracedRays = StochasticRelaxation::activeState().prevTracedRays = StochasticRelaxation::activeState().numberOfMisses = 0;
    StochasticRelaxation::activeState().importanceTracedRays = StochasticRelaxation::activeState().prevImportanceTracedRays = 0;
    StochasticRelaxation::activeState().setSource = StochasticRelaxation::activeState().indirectOnly;
    StochasticRelaxation::activeState().tracedPaths = 0;
    StochasticRelaxation::activeState().controlRadiance.clear();

    StochasticRelaxation::activeState().unShotFlux.clear();
    StochasticRelaxation::activeState().unShotYmp = 0.0;
    StochasticRelaxation::activeState().totalFlux.clear();
    StochasticRelaxation::activeState().totalYmp = 0.0;
    StochasticRelaxation::activeState().indrcImpWghtdUnShotFlux.clear();
    for ( int i = 0; scene->patchList != NULL && i < scene->patchList->size(); i++ ) {
        Patch *patch = scene->patchList->get(i);
        monteCarloRadiosityInitPatch(patch);
        StochasticRelaxation::activeState().unShotFlux.addScaled(
            StochasticRelaxation::activeState().unShotFlux,
            ((float)(M_PI)) * patch->area,
            McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().totalFlux.addScaled(
            StochasticRelaxation::activeState().totalFlux,
            ((float)(M_PI)) * patch->area,
            McradP::getTopLevelPatchRad(patch)[0]);
        StochasticRelaxation::activeState().indrcImpWghtdUnShotFlux.addScaled(
            StochasticRelaxation::activeState().indrcImpWghtdUnShotFlux,
            ((float)(M_PI)) * patch->area *
            (McradP::topLvlStochRadElem(patch)->importance - McradP::topLvlStochRadElem(patch)->sourceImportance),
            McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().unShotYmp += patch->area * Math::abs(McradP::topLvlStochRadElem(patch)->unShotImportance);
        StochasticRelaxation::activeState().totalYmp += patch->area * McradP::topLvlStochRadElem(patch)->importance;
        StochasticRelaxation::activeState().sourceYmp += patch->area * McradP::topLvlStochRadElem(patch)->sourceImportance;
        Mcrad::mntCarloRadPtchCompNewClr(patch);
    }

    mntCarloRadDetInitNrRays(scene->patchList, scene->geometryList);

    Hierarchy::elementHierarchyInit(scene->clusteredRootGeometry);

    if ( StochasticRelaxation::activeState().importanceDriven ) {
        Mcrad::mntCarloRadUpdViewImp(scene, renderOptions);
        StochasticRelaxation::activeState().importanceUpdatedFromScratch = true;
    }
}

void
Mcrad::monteCarloRadiosityPreStep(Scene *scene, const RenderOptions *renderOptions) {
    if ( !StochasticRelaxation::activeState().inited ) {
        Mcrad::monteCarloRadiosityReInit(scene, renderOptions);
    }
    if ( StochasticRelaxation::activeState().importanceDriven && scene->camera->changed ) {
        Mcrad::mntCarloRadUpdViewImp(scene, renderOptions);
    }

    StochasticRelaxation::activeState().lastClock = System::nanoTime();
    StochasticRelaxation::activeState().currentIteration++;
}

/**
Undoes the effect of mainInitApplication() and all side-effects of Step()
*/
void
Mcrad::monteCarloRadiosityTerminate(const ArrayList<Patch *> *scenePatches) {
    Hierarchy::elementHierarchyTerminate(scenePatches);
    StochasticRelaxation::activeState().inited = false;
}

 ColorRgb
Mcrad::mntCarloRadDffsReflAPnt(Patch *patch, double u, double v) {
    RayHit hit;
    Vector3D point;
    patch->uniformPoint(u, v, &point);
    hit.init(patch, &point, &patch->normal, patch->material);
    hit.setUv(u, v);
    unsigned int newFlags = hit.getFlags() | UV;
    hit.setFlags(newFlags);
    ColorRgb result;
    result.clear();
    if ( hit.getMaterial()->getBsdf() != NULL ) {
        result = hit.getMaterial()->getBsdf()->splitBsdfScatteredPower(&hit, BRDF_DIFFUSE_COMPONENT);
    }
    return result;
}

 ColorRgb
Mcrad::vertexReflectance(const Vertex *v) {
    int count = 0;
    ColorRgb rd;

    rd.clear();
    for ( int i = 0; v->radianceData != NULL && i < v->radianceData->size(); i++ ) {
        Element *genericElement = v->radianceData->get(i);
        if ( genericElement->className != ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        const StochasticRadiosityElement *element = ((StochasticRadiosityElement *)(genericElement));
        if ( !element->regularSubElements ) {
            rd.add(rd, element->Rd);
            count++;
        }
    }

    if ( count > 0 ) {
        rd.scaleInverse(((float)(count)), rd);
    }

    return rd;
}

 ColorRgb
Mcrad::mntCarloRadInterpReflAPnt(const StochasticRadiosityElement *leaf, double u, double v) {
    static const StochasticRadiosityElement *cachedLeaf = NULL;
    static ColorRgb vrd[4];
    static ColorRgb rd;

    if ( leaf != NULL ) {
        if ( leaf != cachedLeaf ) {
            for ( int i = 0; i < leaf->numberOfVertices; i++ ) {
                vrd[i] = vertexReflectance(leaf->vertices[i]);
            }
        }
        cachedLeaf = leaf;

        rd.clear();
        switch ( leaf->numberOfVertices ) {
            case 3:
                rd.interpolateBarycentric(vrd[0], vrd[1], vrd[2], ((float)(u)), ((float)(v)));
                break;
            case 4:
                rd.interpolateBiLinear(vrd[0], vrd[1], vrd[2], vrd[3], ((float)(u)), ((float)(v)));
                break;
            default:
                Logger::fatal(-1, "mntCarloRadInterpReflAPnt", "Invalid nr of vertices %d",
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
Mcrad::monteCarloRadiosityGetRadiance(Patch *patch, double u, double v, Vector3D /*dir*/, const RenderOptions *renderOptions) {
    ColorRgb TrueRdAtPoint = mntCarloRadDffsReflAPnt(patch, u, v);
    const StochasticRadiosityElement *leaf = StochasticRadiosityElement::stchsRadElemRegLeafElemAPnt(
        McradP::topLvlStochRadElem(patch), &u, &v);
    ColorRgb UsedRdAtPoint = renderOptions->smoothShading ? mntCarloRadInterpReflAPnt(leaf, u, v) : leaf->Rd;
    ColorRgb radianceAtPoint = StochasticRadiosityElement::stchsRadElemDispRadnAPnt(leaf, u, v, renderOptions);
    ColorRgb sourceRad;
    sourceRad.clear();

    // Subtract source radiance
    if ( StochasticRelaxation::activeState().show != SHOW_INDIRECT_RADIANCE ) {
        // sourceRad is self-emitted radiance when indirect-only is disabled.
        // Otherwise it represents direct illumination.
        if ( !StochasticRelaxation::activeState().doNonDiffuseFirstShot ) {
            sourceRad = leaf->sourceRad;
        }
        if ( StochasticRelaxation::activeState().indirectOnly || StochasticRelaxation::activeState().doNonDiffuseFirstShot ) {
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
Mcrad::mntCarloRadSclrRefl(const Patch *P) {
    return StochasticRadiosityElement::stchsRadElemSclrRefl(McradP::topLvlStochRadElem(P));
}

#endif
