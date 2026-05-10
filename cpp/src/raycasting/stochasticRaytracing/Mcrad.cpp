#include "environment/geometry/elements/RayHitFlag.h"
/**
Monte Carlo Radiosity: common code for stochastic relaxation and random walks
*/

#include <cstdlib>

#include "java/lang/System.h"
#include "common/RenderOptions.h"
#include "raycasting/stochasticRaytracing/Mcrad.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.txx"
#include "common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "render/Potential.h"
#include "raycasting/stochasticRaytracing/Hierarchy.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

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
    StochasticRelaxation::activeState().sequence = Sampler4DSequence::NIEDERREITER;
    StochasticRelaxation::activeState().approximationOrderType = StochasticRaytracingApproximation::CONSTANT;
    StochasticRelaxation::activeState().importanceDriven = false;
    StochasticRelaxation::activeState().radianceDriven = true;
    StochasticRelaxation::activeState().importanceUpdated = false;
    StochasticRelaxation::activeState().importanceUpdatedFromScratch = false;
    StochasticRelaxation::activeState().continuousRandomWalk = false;
    StochasticRelaxation::activeState().randomWalkEstimatorType = RandomWalkEstimatorType::RW_SHOOTING;
    StochasticRelaxation::activeState().randomWalkEstimatorKind = RandomWalkEstimatorKind::RW_COLLISION;
    StochasticRelaxation::activeState().randomWalkNumLast = 1;
    StochasticRelaxation::activeState().weightedSampling = false;
    StochasticRelaxation::activeState().discardIncremental = false;
    StochasticRelaxation::activeState().incrementalUsesImportance = false;
    StochasticRelaxation::activeState().naiveMerging = false;
    StochasticRelaxation::activeState().show = WhatToShow::SHOW_TOTAL_RADIANCE;
    StochasticRelaxation::activeState().doNonDiffuseFirstShot = false;
    StochasticRelaxation::activeState().initialLightSourceSamples = 1000;

    Hierarchy::elementHierarchyDefaults();
    Basismcrad::monteCarloRadiosityInitBasis();
}

/**
For counting how much CPU time was used for the computations
*/
void
Mcrad::monteCarloRadiosityUpdateCpuSecs() {
    const long long t = java::System::nanoTime();
    StochasticRelaxation::activeState().cpuSeconds += static_cast<float>(
        static_cast<double>(t - StochasticRelaxation::activeState().lastClock) / 1000000000.0);
    StochasticRelaxation::activeState().lastClock = t;
}

Element *
Mcrad::monteCarloRadiosityCreatePatchData(Patch *patch) {
    patch->setRadianceData(StochasticRadiosityElement::stochasticRadiosityElementCreateFromPatch(patch));
    return patch->getRadianceData();
}

void
Mcrad::monteCarloRadiosityDestroyPatchData(Patch *patch) {
    if ( patch->getRadianceData() ) {
        StochasticRadiosityElement::stochasticRadiosityElementDestroy(McradP::topLevelStochasticRadiosityElement(patch));
    }
    patch->setRadianceData(nullptr);
}

/**
Compute new color for the patch: fine if no hierarchical refinement is used, e.g.
in the current random walk radiosity implementation
*/
void
Mcrad::monteCarloRadiosityPatchComputeNewColor(Patch *patch) {
    patch->setColor(StochasticRadiosityElement::stochasticRadiosityElementColor(McradP::topLevelStochasticRadiosityElement(patch)));
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
    ColorRgb Ed = McradP::topLevelStochasticRadiosityElement(patch)->Ed;

    Coefficientsmcrad::reAllocCoefficients(McradP::topLevelStochasticRadiosityElement(patch));
    Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchRad(patch), McradP::getTopLevelPatchBasis(patch));
    Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchUnShotRad(patch), McradP::getTopLevelPatchBasis(patch));
    Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchReceivedRad(patch), McradP::getTopLevelPatchBasis(patch));

    McradP::getTopLevelPatchRad(patch)[0] = McradP::getTopLevelPatchUnShotRad(patch)[0] = McradP::topLevelStochasticRadiosityElement(patch)->sourceRad = Ed;
    McradP::getTopLevelPatchReceivedRad(patch)[0].clear();

    McradP::topLevelStochasticRadiosityElement(patch)->rayIndex = patch->getId() * 11;
    McradP::topLevelStochasticRadiosityElement(patch)->quality = 0.0;
    McradP::topLevelStochasticRadiosityElement(patch)->ng = 0;
    McradP::topLevelStochasticRadiosityElement(patch)->importance = 0.0;
    McradP::topLevelStochasticRadiosityElement(patch)->unShotImportance = 0.0;
    McradP::topLevelStochasticRadiosityElement(patch)->receivedImportance = 0.0;
    McradP::topLevelStochasticRadiosityElement(patch)->sourceImportance = 0.0;
}

/**
Routines below update/re-initialise importance after a viewing change
*/
 void
Mcrad::monteCarloRadiosityPullImportances(Element *element) {
    StochasticRadiosityElement *child = static_cast<StochasticRadiosityElement *>(element);
    StochasticRadiosityElement *parent = static_cast<StochasticRadiosityElement *>(child->parent);
    StochasticRadiosityElement::stochasticRadiosityElementPullImportance(parent, child, &parent->importance, &child->importance);
    StochasticRadiosityElement::stochasticRadiosityElementPullImportance(parent, child, &parent->sourceImportance, &child->sourceImportance);
    StochasticRadiosityElement::stochasticRadiosityElementPullImportance(parent, child, &parent->unShotImportance, &child->unShotImportance);
}

 void
Mcrad::monteCarloRadiosityAccumulateImportances(const StochasticRadiosityElement *elem) {
    StochasticRelaxation::activeState().totalYmp += elem->area * elem->importance;
    StochasticRelaxation::activeState().sourceYmp += elem->area * elem->sourceImportance;
    StochasticRelaxation::activeState().unShotYmp += elem->area * java::Math::abs(elem->unShotImportance);
}

/**
Update importance in the element hierarchy starting with the top cluster
*/
 void
Mcrad::monteCarloRadiosityUpdateImportance(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = static_cast<StochasticRadiosityElement *>(element);

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }

    if ( !stochasticRadiosityElement->traverseAllChildren(monteCarloRadiosityUpdateImportance) ) {
        // Leaf element
        float delta_imp = static_cast<float>(stochasticRadiosityElement->patch->isVisible() ? 1.0 : 0.0) - stochasticRadiosityElement->sourceImportance;
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
 void
Mcrad::monteCarloRadiosityReInitImportance(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = static_cast<StochasticRadiosityElement *>(element);

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
Mcrad::monteCarloRadiosityUpdateViewImportance(Scene *scene, const RenderOptions *renderOptions) {
    java::System::err.printf("Updating direct visibility ... \n");

    Potential::updateDirectVisibility(scene, renderOptions);

    StochasticRelaxation::activeState().sourceYmp = 0.0;
    StochasticRelaxation::activeState().unShotYmp = 0.0;
    StochasticRelaxation::activeState().totalYmp = 0.0;
    monteCarloRadiosityUpdateImportance(ElementHierarchyState::activeState().topCluster);

    if ( StochasticRelaxation::activeState().unShotYmp < StochasticRelaxation::activeState().sourceYmp ) {
        java::System::err.printf("Importance will be recomputed incrementally.\n");
        StochasticRelaxation::activeState().importanceUpdatedFromScratch = false;
    } else {
        java::System::err.printf("Importance will be recomputed from scratch.\n");
        StochasticRelaxation::activeState().importanceUpdatedFromScratch = true;

        // Re-compute from scratch
        StochasticRelaxation::activeState().sourceYmp = 0.0;
        StochasticRelaxation::activeState().unShotYmp = 0.0;
        StochasticRelaxation::activeState().totalYmp = 0.0;
        monteCarloRadiosityReInitImportance(ElementHierarchyState::activeState().topCluster);
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
Mcrad::monteCarloRadiosityDetermineAreaFraction(
    const java::ArrayList<Patch *> *scenePatches,
    const java::ArrayList<Geometry *> *sceneGeometries)
{
    float *areas;
    float cumulative;
    float areaFrac;
    int numberOfPatchIds = Patch::getNextId();
    int i;

    auto qSortFloatCompare = [](const void *a, const void *b) -> int {
        const float left = *static_cast<const float *>(a);
        const float right = *static_cast<const float *>(b);
        if ( Numeric::floatCompare(left, right) ) {
            return 1;
        }
        if ( Numeric::floatCompare(right, left) ) {
            return -1;
        }
        return 0;
    };

    if ( sceneGeometries == nullptr || sceneGeometries->size() == 0 ) {
        // An arbitrary positive number (in order to avoid divisions by zero
        return 100;
    }

    // Build a table of patch areas
    areas = new float[numberOfPatchIds];
    for ( i = 0; i < numberOfPatchIds; i++ ) {
        areas[i] = 0.0;
    }
    for ( i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        areas[patch->getId()] = patch->getArea();
    }

    // Sort the table to decreasing areas
    qsort(areas,
        numberOfPatchIds,
        sizeof(float),
        qSortFloatCompare);

    // Find the patch such that 10% of the total surface area is filled by smaller patches
    for ( i = numberOfPatchIds - 1, cumulative = 0.0; i >= 0 && cumulative < Statistics::instance().radiance.totalArea * 0.1; i-- ) {
        cumulative += areas[i];
    }
    areaFrac = (i >= 0 && areas[i] > 0.0) ? Statistics::instance().radiance.totalArea / areas[i] : static_cast<float>(Statistics::instance().reader.numberOfPatches);

    delete[] areas;

    return areaFrac;
}

/**
Determines elementary ray power for the initial incremental iterations
*/
 void
Mcrad::monteCarloRadiosityDetermineInitialNrRays(
    const java::ArrayList<Patch *> *scenePatches,
    const java::ArrayList<Geometry *> *sceneGeometries)
{
    double areaFrac = monteCarloRadiosityDetermineAreaFraction(scenePatches, sceneGeometries);
    StochasticRelaxation::activeState().initialNumberOfRays = static_cast<long>(static_cast<double>(StochasticRelaxation::activeState().rayUnitsPerIt) * areaFrac);
}

/**
Really initialises: before the first iteration step
*/
void
Mcrad::monteCarloRadiosityReInit(Scene *scene, const RenderOptions *renderOptions) {
    if ( StochasticRelaxation::activeState().inited ) {
        return;
    }

    java::System::err.printf("Initialising Monte Carlo radiosity ...\n");

    Sample4d::setSequence4D(StochasticRelaxation::activeState().sequence);

    StochasticRelaxation::activeState().inited = true;
    StochasticRelaxation::activeState().cpuSeconds = 0.0;
    StochasticRelaxation::activeState().lastClock = java::System::nanoTime();
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
    StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux.clear();
    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        Patch *patch = scene->patchList->get(i);
        monteCarloRadiosityInitPatch(patch);
        StochasticRelaxation::activeState().unShotFlux.addScaled(
            StochasticRelaxation::activeState().unShotFlux,
            static_cast<float>(M_PI) * patch->getArea(),
            McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().totalFlux.addScaled(
            StochasticRelaxation::activeState().totalFlux,
            static_cast<float>(M_PI) * patch->getArea(),
            McradP::getTopLevelPatchRad(patch)[0]);
        StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux.addScaled(
            StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux,
            static_cast<float>(M_PI) * patch->getArea() *
            (McradP::topLevelStochasticRadiosityElement(patch)->importance - McradP::topLevelStochasticRadiosityElement(patch)->sourceImportance),
            McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().unShotYmp += patch->getArea() * java::Math::abs(McradP::topLevelStochasticRadiosityElement(patch)->unShotImportance);
        StochasticRelaxation::activeState().totalYmp += patch->getArea() * McradP::topLevelStochasticRadiosityElement(patch)->importance;
        StochasticRelaxation::activeState().sourceYmp += patch->getArea() * McradP::topLevelStochasticRadiosityElement(patch)->sourceImportance;
        Mcrad::monteCarloRadiosityPatchComputeNewColor(patch);
    }

    monteCarloRadiosityDetermineInitialNrRays(scene->patchList, scene->geometryList);

    Hierarchy::elementHierarchyInit(scene->clusteredRootGeometry);

    if ( StochasticRelaxation::activeState().importanceDriven ) {
        Mcrad::monteCarloRadiosityUpdateViewImportance(scene, renderOptions);
        StochasticRelaxation::activeState().importanceUpdatedFromScratch = true;
    }
}

void
Mcrad::monteCarloRadiosityPreStep(Scene *scene, const RenderOptions *renderOptions) {
    if ( !StochasticRelaxation::activeState().inited ) {
        Mcrad::monteCarloRadiosityReInit(scene, renderOptions);
    }
    if ( StochasticRelaxation::activeState().importanceDriven && scene->camera->changed ) {
        Mcrad::monteCarloRadiosityUpdateViewImportance(scene, renderOptions);
    }

    StochasticRelaxation::activeState().lastClock = java::System::nanoTime();
    StochasticRelaxation::activeState().currentIteration++;
}

/**
Undoes the effect of mainInitApplication() and all side-effects of Step()
*/
void
Mcrad::monteCarloRadiosityTerminate(const java::ArrayList<Patch *> *scenePatches) {
    Hierarchy::elementHierarchyTerminate(scenePatches);
    StochasticRelaxation::activeState().inited = false;
}

 ColorRgb
Mcrad::monteCarloRadiosityDiffuseReflectanceAtPoint(Patch *patch, double u, double v) {
    RayHit hit;
    Vector3D point;
    patch->uniformPoint(u, v, &point);
    hit.init(patch, &point, &patch->getNormal(), patch->getMaterial());
    hit.setUv(u, v);
    unsigned int newFlags = hit.getFlags() | RayHitFlag::UV;
    hit.setFlags(newFlags);
    ColorRgb result;
    result.clear();
    if ( hit.getMaterial()->getBsdf() != nullptr ) {
        bool shctxOk = false;
        ShadingContext shctx = hit.shadingContext(&shctxOk);
        if ( shctxOk ) {
            result = hit.getMaterial()->getBsdf()->splitBsdfScatteredPower(shctx, BRDF_DIFFUSE_COMPONENT);
        } else {
            result.clear();
        }
    }
    return result;
}

 ColorRgb
Mcrad::vertexReflectance(const Vertex *v) {
    int count = 0;
    ColorRgb rd;

    rd.clear();
    for ( int i = 0; v->radianceData != nullptr && i < v->radianceData->size(); i++ ) {
        Element *genericElement = v->radianceData->get(i);
        if ( genericElement->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        const StochasticRadiosityElement *element = static_cast<StochasticRadiosityElement *>(genericElement);
        if ( !element->regularSubElements ) {
            rd.add(rd, element->Rd);
            count++;
        }
    }

    if ( count > 0 ) {
        rd.scaleInverse(static_cast<float>(count), rd);
    }

    return rd;
}

 ColorRgb
Mcrad::monteCarloRadiosityInterpolatedReflectanceAtPoint(const StochasticRadiosityElement *leaf, double u, double v) {
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
                rd.interpolateBarycentric(vrd[0], vrd[1], vrd[2], static_cast<float>(u), static_cast<float>(v));
                break;
            case 4:
                rd.interpolateBiLinear(vrd[0], vrd[1], vrd[2], vrd[3], static_cast<float>(u), static_cast<float>(v));
                break;
            default:
                Logger::fatal(-1, "monteCarloRadiosityInterpolatedReflectanceAtPoint", "Invalid nr of vertices %d",
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
    ColorRgb TrueRdAtPoint = monteCarloRadiosityDiffuseReflectanceAtPoint(patch, u, v);
    const StochasticRadiosityElement *leaf = StochasticRadiosityElement::stochasticRadiosityElementRegularLeafElementAtPoint(
        McradP::topLevelStochasticRadiosityElement(patch), &u, &v);
    ColorRgb UsedRdAtPoint = renderOptions->smoothShading ? monteCarloRadiosityInterpolatedReflectanceAtPoint(leaf, u, v) : leaf->Rd;
    ColorRgb radianceAtPoint = StochasticRadiosityElement::stochasticRadiosityElementDisplayRadianceAtPoint(leaf, u, v, renderOptions);
    ColorRgb sourceRad;
    sourceRad.clear();

    // Subtract source radiance
    if ( StochasticRelaxation::activeState().show != WhatToShow::SHOW_INDIRECT_RADIANCE ) {
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
Mcrad::monteCarloRadiosityScalarReflectance(const Patch *P) {
    return StochasticRadiosityElement::stochasticRadiosityElementScalarReflectance(McradP::topLevelStochasticRadiosityElement(P));
}

#endif
