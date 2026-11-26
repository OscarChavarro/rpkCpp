/**
Monte Carlo Radiosity: common code for stochastic relaxation and random walks
*/

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include <cstdlib>

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/lang/System.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Mcrad.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/render/Potential.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Hierarchy.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/McradP.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"

/**
Common routines for stochastic relaxation and random walks
*/
void
Mcrad::monteCarloRadiosityDefaults() {
    StochasticRelaxation::activeState().inited = false;
    StochasticRelaxation::activeState().rayUnitsPerIt = 10;
    StochasticRelaxation::activeState().bidirectionalTransfers = false;
    StochasticRelaxation::activeState().constantControlVariate = false;
    StochasticRelaxation::activeState().controlRadiance = ColorRgb(0.0, 0.0, 0.0);
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
    StochasticRelaxation::activeState().cpuSeconds += static_cast<double>(
        static_cast<double>(t - StochasticRelaxation::activeState().lastClock) / 1000000000.0);
    StochasticRelaxation::activeState().lastClock = t;
}

Element *
Mcrad::monteCarloRadiosityCreatePatchData(Patch *patch) {
    return StochasticRadiosityElement::stochasticRadiosityElementCreateFromPatch(patch);
}

void
Mcrad::monteCarloRadiosityDestroyPatchData(Patch *patch) {
    // Elements are bound once the method has been initialized
    if ( ElementHierarchyState::activeState().getRadianceElements() != nullptr ) {
        StochasticRadiosityElement *element = McradP::topLevelStochasticRadiosityElement(patch);
        if ( element != nullptr ) {
            StochasticRadiosityElement::stochasticRadiosityElementDestroy(element);
        }
    }
}

/**
Compute new color for the patch: fine if no hierarchical refinement is used, e.g.
in the current random walk radiosity implementation
*/
void
Mcrad::monteCarloRadiosityPatchComputeNewColor(const Patch *patch, FaceColorTable *faceColors) {
    faceColors->set(patch->getFaceIndex(), StochasticRadiosityElement::stochasticRadiosityElementColor(McradP::topLevelStochasticRadiosityElement(patch)));
    patch->computeVertexColors(faceColors);
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
    const ColorRgb Ed = McradP::topLevelStochasticRadiosityElement(patch)->Ed;

    Coefficientsmcrad::reAllocCoefficients(McradP::topLevelStochasticRadiosityElement(patch));
    Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchRad(patch), McradP::getTopLevelPatchBasis(patch));
    Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchUnShotRad(patch), McradP::getTopLevelPatchBasis(patch));
    Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchReceivedRad(patch), McradP::getTopLevelPatchBasis(patch));

    McradP::topLevelStochasticRadiosityElement(patch)->sourceRad = Ed;
    McradP::getTopLevelPatchRad(patch)[0] = McradP::getTopLevelPatchUnShotRad(patch)[0] = Ed;
    McradP::getTopLevelPatchReceivedRad(patch)[0] = ColorRgb(0.0, 0.0, 0.0);

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
Calls visit on every child of the element, in the order of
Element::traverseAllChildren, passing the face visibility along. Returns
false for a leaf element.
*/
bool
Mcrad::monteCarloRadiosityVisitChildren(
    const Element *element,
    void (*visit)(Element *, const FaceVisibilityTable *),
    const FaceVisibilityTable *faceVisibility)
{
    if ( element->isCluster() ) {
        for ( int i = 0; element->irregularSubElements != nullptr && i < element->irregularSubElements->size(); i++ ) {
            visit(element->irregularSubElements->get(i), faceVisibility);
        }
        return true;
    }
    if ( element->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            if ( element->regularSubElements[i] != nullptr ) {
                visit(element->regularSubElements[i], faceVisibility);
            }
        }
        return true;
    }
    return false;
}

/**
Update importance in the element hierarchy starting with the top cluster
*/
 void
Mcrad::monteCarloRadiosityUpdateImportance(Element *element, const FaceVisibilityTable *faceVisibility) {
    StochasticRadiosityElement *stochasticRadiosityElement = static_cast<StochasticRadiosityElement *>(element);

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }

    if ( !monteCarloRadiosityVisitChildren(stochasticRadiosityElement, monteCarloRadiosityUpdateImportance, faceVisibility) ) {
        // Leaf element
        double delta_imp = static_cast<double>(faceVisibility->isVisible(stochasticRadiosityElement->patch->getFaceIndex()) ? 1.0 : 0.0) - stochasticRadiosityElement->sourceImportance;
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
Mcrad::monteCarloRadiosityReInitImportance(Element *element, const FaceVisibilityTable *faceVisibility) {
    StochasticRadiosityElement *stochasticRadiosityElement = static_cast<StochasticRadiosityElement *>(element);

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }

    if ( !monteCarloRadiosityVisitChildren(stochasticRadiosityElement, monteCarloRadiosityReInitImportance, faceVisibility) ) {
        // Leaf element
        stochasticRadiosityElement->importance = faceVisibility->isVisible(stochasticRadiosityElement->patch->getFaceIndex()) ? 1.0 : 0.0;
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
Mcrad::monteCarloRadiosityUpdateViewImportance(Scene *scene, const RendererConfiguration *renderOptions) {
    java::System::err.printf("Updating direct visibility ... \n");

    Potential::updateDirectVisibility(scene, renderOptions);

    StochasticRelaxation::activeState().sourceYmp = 0.0;
    StochasticRelaxation::activeState().unShotYmp = 0.0;
    StochasticRelaxation::activeState().totalYmp = 0.0;
    monteCarloRadiosityUpdateImportance(ElementHierarchyState::activeState().topCluster, scene->getFaceVisibility());

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
        monteCarloRadiosityReInitImportance(ElementHierarchyState::activeState().topCluster, scene->getFaceVisibility());
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
    const java::ArrayList<RpkGeometry *> *sceneGeometries)
{
    double *areas;
    double cumulative;
    double areaFrac;
    int numberOfPatchIds = Patch::getNextId();
    int i;

    auto qSortFloatCompare = [](const void *a, const void *b) -> int {
        const double left = *static_cast<const double *>(a);
        const double right = *static_cast<const double *>(b);
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
    areas = new double[numberOfPatchIds];
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
        sizeof(double),
        qSortFloatCompare);

    // Find the patch such that 10% of the total surface area is filled by smaller patches
    for ( i = numberOfPatchIds - 1, cumulative = 0.0; i >= 0 && cumulative < Statistics::instance().radiance.totalArea * 0.1; i-- ) {
        cumulative += areas[i];
    }
    areaFrac = (i >= 0 && areas[i] > 0.0) ? Statistics::instance().radiance.totalArea / areas[i] : static_cast<double>(Statistics::instance().reader.numberOfPatches);

    delete[] areas;

    return areaFrac;
}

/**
Determines elementary ray power for the initial incremental iterations
*/
 void
Mcrad::monteCarloRadiosityDetermineInitialNrRays(
    const java::ArrayList<Patch *> *scenePatches,
    const java::ArrayList<RpkGeometry *> *sceneGeometries)
{
    double areaFrac = monteCarloRadiosityDetermineAreaFraction(scenePatches, sceneGeometries);
    StochasticRelaxation::activeState().initialNumberOfRays = static_cast<long>(static_cast<double>(StochasticRelaxation::activeState().rayUnitsPerIt) * areaFrac);
}

/**
Really initialises: before the first iteration step
*/
void
Mcrad::monteCarloRadiosityReInit(Scene *scene, const RendererConfiguration *renderOptions) {
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
    StochasticRelaxation::activeState().controlRadiance = ColorRgb(0.0, 0.0, 0.0);

    StochasticRelaxation::activeState().unShotFlux = ColorRgb(0.0, 0.0, 0.0);
    StochasticRelaxation::activeState().unShotYmp = 0.0;
    StochasticRelaxation::activeState().totalFlux = ColorRgb(0.0, 0.0, 0.0);
    StochasticRelaxation::activeState().totalYmp = 0.0;
    StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux = ColorRgb(0.0, 0.0, 0.0);
    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        Patch *patch = scene->patchList->get(i);
        monteCarloRadiosityInitPatch(patch);
        StochasticRelaxation::activeState().unShotFlux = StochasticRelaxation::activeState().unShotFlux.addScaled(static_cast<double>(M_PI) * patch->getArea(), McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().totalFlux = StochasticRelaxation::activeState().totalFlux.addScaled(static_cast<double>(M_PI) * patch->getArea(), McradP::getTopLevelPatchRad(patch)[0]);
        StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux = StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux.addScaled(static_cast<double>(M_PI) * patch->getArea() *
            (McradP::topLevelStochasticRadiosityElement(patch)->importance - McradP::topLevelStochasticRadiosityElement(patch)->sourceImportance), McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().unShotYmp += patch->getArea() * java::Math::abs(McradP::topLevelStochasticRadiosityElement(patch)->unShotImportance);
        StochasticRelaxation::activeState().totalYmp += patch->getArea() * McradP::topLevelStochasticRadiosityElement(patch)->importance;
        StochasticRelaxation::activeState().sourceYmp += patch->getArea() * McradP::topLevelStochasticRadiosityElement(patch)->sourceImportance;
        Mcrad::monteCarloRadiosityPatchComputeNewColor(patch, scene->getFaceColors());
    }

    monteCarloRadiosityDetermineInitialNrRays(scene->patchList, scene->geometryList);

    Hierarchy::elementHierarchyInit(scene->clusteredRootGeometry);

    if ( StochasticRelaxation::activeState().importanceDriven ) {
        Mcrad::monteCarloRadiosityUpdateViewImportance(scene, renderOptions);
        StochasticRelaxation::activeState().importanceUpdatedFromScratch = true;
    }
}

void
Mcrad::monteCarloRadiosityPreStep(Scene *scene, const RendererConfiguration *renderOptions) {
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
Mcrad::monteCarloRadiosityDiffuseReflectanceAtPoint(const Patch *patch, double u, double v) {
    RpkRayHit hit;
    Vector3Dd point;
    patch->uniformPoint(u, v, &point);
    const Vector3Dd patchNormal = patch->getNormal();
    hit.init(patch, &point, &patchNormal, patch->getMaterial());
    hit.setUv(u, v);
    if ( hit.getMaterial()->getBsdf() != nullptr ) {
        bool shctxOk = false;
        ShadingContext shctx = hit.shadingContext(&shctxOk);
        if ( shctxOk ) {
            return hit.getMaterial()->getBsdf()->splitBsdfScatteredPower(shctx, BRDF_DIFFUSE_COMPONENT);
        }
    }
    return ColorRgb(0.0, 0.0, 0.0);
}

ColorRgb
Mcrad::vertexReflectance(const RpkVertex *v) {
    int count = 0;
    double rd[3] = {0.0, 0.0, 0.0};

    for ( int i = 0; v->radianceData != nullptr && i < v->radianceData->size(); i++ ) {
        Element *genericElement = v->radianceData->get(i);
        if ( genericElement->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        const StochasticRadiosityElement *element = static_cast<StochasticRadiosityElement *>(genericElement);
        if ( !element->regularSubElements ) {
            rd[0] += element->Rd.getR();
            rd[1] += element->Rd.getG();
            rd[2] += element->Rd.getB();
            count++;
        }
    }

    if ( count > 0 ) {
        const double scale = 1.0 / static_cast<double>(count);
        rd[0] = scale * rd[0];
        rd[1] = scale * rd[1];
        rd[2] = scale * rd[2];
    }

    return ColorRgb(rd[0], rd[1], rd[2]);
}

ColorRgb
Mcrad::monteCarloRadiosityInterpolatedReflectanceAtPoint(const StochasticRadiosityElement *leaf, double u, double v) {
    static const StochasticRadiosityElement *cachedLeaf = nullptr;
    static double vertexReflectance[4][3] = {};
    static double rd[3] = {};

    if ( leaf != nullptr ) {
        if ( leaf != cachedLeaf ) {
            for ( int i = 0; i < leaf->numberOfVertices; i++ ) {
                const ColorRgb reflectance = Mcrad::vertexReflectance(leaf->vertices[i]);
                vertexReflectance[i][0] = reflectance.getR();
                vertexReflectance[i][1] = reflectance.getG();
                vertexReflectance[i][2] = reflectance.getB();
            }
        }
        cachedLeaf = leaf;

        const double uu = static_cast<double>(u);
        const double vv = static_cast<double>(v);
        switch ( leaf->numberOfVertices ) {
            case 3: {
                for ( int channel = 0; channel < 3; channel++ ) {
                    rd[channel] = vertexReflectance[0][channel] +
                        uu * (vertexReflectance[1][channel] - vertexReflectance[0][channel]) +
                        vv * (vertexReflectance[2][channel] - vertexReflectance[0][channel]);
                }
                break;
            }
            case 4: {
                const double c = uu * vv;
                const double bb = uu - c;
                const double d = vv - c;
                for ( int channel = 0; channel < 3; channel++ ) {
                    rd[channel] = vertexReflectance[0][channel] +
                        bb * (vertexReflectance[1][channel] - vertexReflectance[0][channel]) +
                        c * (vertexReflectance[2][channel] - vertexReflectance[0][channel]) +
                        d * (vertexReflectance[3][channel] - vertexReflectance[0][channel]);
                }
                break;
            }
            default:
                Logger::reportMessage("Mcrad", Logger::FATAL_ERROR, "monteCarloRadiosityInterpolatedReflectanceAtPoint", java::String::format("Invalid nr of vertices %d", leaf->numberOfVertices));
        }
    }
    return ColorRgb(rd[0], rd[1], rd[2]);
}

/**
Returns the radiance emitted from the patch at the point with parameters
(u,v) into the direction 'dir'
*/
ColorRgb
Mcrad::monteCarloRadiosityGetRadiance(const Patch *patch, double u, double v, Vector3Dd /*dir*/, const RendererConfiguration *renderOptions) {
    const ColorRgb trueRd = monteCarloRadiosityDiffuseReflectanceAtPoint(patch, u, v);
    const StochasticRadiosityElement *leaf = StochasticRadiosityElement::stochasticRadiosityElementRegularLeafElementAtPoint(
        McradP::topLevelStochasticRadiosityElement(patch), &u, &v);
    const ColorRgb usedRd = renderOptions->isSmoothShading()
        ? monteCarloRadiosityInterpolatedReflectanceAtPoint(leaf, u, v)
        : leaf->Rd;
    const ColorRgb displayRadiance = StochasticRadiosityElement::stochasticRadiosityElementDisplayRadianceAtPoint(leaf, u, v, renderOptions);
    double radiance[3] = {displayRadiance.getR(), displayRadiance.getG(), displayRadiance.getB()};
    const double reflectivity[3] = {trueRd.getR(), trueRd.getG(), trueRd.getB()};
    const double usedReflectivity[3] = {usedRd.getR(), usedRd.getG(), usedRd.getB()};
    double source[3] = {0.0, 0.0, 0.0};

    // Subtract source radiance
    if ( StochasticRelaxation::activeState().show != WhatToShow::SHOW_INDIRECT_RADIANCE ) {
        // sourceRad is self-emitted radiance when indirect-only is disabled.
        // Otherwise it represents direct illumination.
        if ( !StochasticRelaxation::activeState().doNonDiffuseFirstShot ) {
            source[0] = leaf->sourceRad.getR();
            source[1] = leaf->sourceRad.getG();
            source[2] = leaf->sourceRad.getB();
        }
        if ( StochasticRelaxation::activeState().indirectOnly || StochasticRelaxation::activeState().doNonDiffuseFirstShot ) {
            // Subtract self-emitted radiance
            source[0] += leaf->Ed.getR();
            source[1] += leaf->Ed.getG();
            source[2] += leaf->Ed.getB();
        }
    }
    for ( int i = 0; i < 3; i++ ) {
        radiance[i] = radiance[i] - source[i];
        radiance[i] = radiance[i] * reflectivity[i];
        radiance[i] = usedReflectivity[i] != 0.0 ? radiance[i] / usedReflectivity[i] : radiance[i];
        radiance[i] = radiance[i] + source[i];
    }
    return ColorRgb(radiance[0], radiance[1], radiance[2]);
}

/**
Returns scalar reflectance, for importance propagation
*/
double
Mcrad::monteCarloRadiosityScalarReflectance(const Patch *P) {
    return StochasticRadiosityElement::stochasticRadiosityElementScalarReflectance(McradP::topLevelStochasticRadiosityElement(P));
}

#endif
