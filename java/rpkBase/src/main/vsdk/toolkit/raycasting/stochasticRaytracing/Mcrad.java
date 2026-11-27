/**
Monte Carlo radiosity
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.scene.FaceVisibilityTable;
import vsdk.toolkit.scene.FaceColorTable;
import java.util.function.BiConsumer;
import java.util.ArrayList;
import java.util.Arrays;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.render.Potential;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.ElementTypes;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;
import vsdk.toolkit.environment.geometry.elements.RpkVertex;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

/**
Common routines for stochastic relaxation and random walks
*/
public final class Mcrad {
    private Mcrad() {
    }

    public static void monteCarloRadiosityDefaults() {
        StochasticRelaxation.activeState().inited = 0;
        StochasticRelaxation.activeState().rayUnitsPerIt = 10;
        StochasticRelaxation.activeState().bidirectionalTransfers = 0;
        StochasticRelaxation.activeState().constantControlVariate = 0;
        StochasticRelaxation.activeState().controlRadiance = new ColorRgb(0.0, 0.0, 0.0);
        StochasticRelaxation.activeState().indirectOnly = 0;
        StochasticRelaxation.activeState().sequence = Sampler4DSequence.NIEDERREITER;
        StochasticRelaxation.activeState().approximationOrderType = StochasticRaytracingApproximation.CONSTANT;
        StochasticRelaxation.activeState().importanceDriven = 0;
        StochasticRelaxation.activeState().radianceDriven = 1;
        StochasticRelaxation.activeState().importanceUpdated = 0;
        StochasticRelaxation.activeState().importanceUpdatedFromScratch = 0;
        StochasticRelaxation.activeState().continuousRandomWalk = 0;
        StochasticRelaxation.activeState().randomWalkEstimatorType = RandomWalkEstimatorType.RW_SHOOTING;
        StochasticRelaxation.activeState().randomWalkEstimatorKind = RandomWalkEstimatorKind.RW_COLLISION;
        StochasticRelaxation.activeState().randomWalkNumLast = 1;
        StochasticRelaxation.activeState().weightedSampling = 0;
        StochasticRelaxation.activeState().discardIncremental = 0;
        StochasticRelaxation.activeState().incrementalUsesImportance = 0;
        StochasticRelaxation.activeState().naiveMerging = 0;
        StochasticRelaxation.activeState().show = WhatToShow.SHOW_TOTAL_RADIANCE;
        StochasticRelaxation.activeState().doNonDiffuseFirstShot = 0;
        StochasticRelaxation.activeState().initialLightSourceSamples = 1000;

        Hierarchy.elementHierarchyDefaults();
        Basismcrad.monteCarloRadiosityInitBasis();
    }

    /**
For counting how much CPU time was used for the computations
*/
    public static void monteCarloRadiosityUpdateCpuSecs() {
        final long t = System.nanoTime();
        StochasticRelaxation.activeState().cpuSeconds += (double)((double)(t - StochasticRelaxation.activeState().lastClock) / 1000000000.0);
        StochasticRelaxation.activeState().lastClock = t;
    }

    public static Element monteCarloRadiosityCreatePatchData(Patch patch) {
        return StochasticRadiosityElement.stochasticRadiosityElementCreateFromPatch(patch);
    }

    public static void monteCarloRadiosityDestroyPatchData(Patch patch) {
        // Elements are bound once the method has been initialized
        if ( ElementHierarchyState.activeState().getRadianceElements() != null ) {
            StochasticRadiosityElement element = McradP.topLevelStochasticRadiosityElement(patch);
            if ( element != null ) {
                StochasticRadiosityElement.stochasticRadiosityElementDestroy(element);
            }
        }
    }

    /**
Compute new color for the patch: fine if no hierarchical refinement is used, e.getG().
in the current random walk radiosity implementation
*/
    public static void monteCarloRadiosityPatchComputeNewColor(Patch patch, FaceColorTable faceColors) {
        faceColors.set(patch.getFaceIndex(), StochasticRadiosityElement.stochasticRadiosityElementColor(McradP.topLevelStochasticRadiosityElement(patch)));
        patch.computeVertexColors(faceColors);
    }

    /**
Initializes the computations for the current scene (if any): initialisations
are delayed to just before the first iteration step, see ReInit() below
*/
    public static void monteCarloRadiosityInit() {
        StochasticRelaxation.activeState().inited = 0;
    }

    /**
Initialises patch data
*/
    private static void monteCarloRadiosityInitPatch(Patch patch) {
        ColorRgb Ed = McradP.topLevelStochasticRadiosityElement(patch).Ed;

        Coefficientsmcrad.reAllocCoefficients(McradP.topLevelStochasticRadiosityElement(patch));
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchRad(patch), McradP.getTopLevelPatchBasis(patch));
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchUnShotRad(patch), McradP.getTopLevelPatchBasis(patch));
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchReceivedRad(patch), McradP.getTopLevelPatchBasis(patch));

        McradP.getTopLevelPatchRad(patch)[0] = new ColorRgb(Ed.getR(), Ed.getG(), Ed.getB());
        McradP.getTopLevelPatchUnShotRad(patch)[0] = new ColorRgb(Ed.getR(), Ed.getG(), Ed.getB());
        McradP.topLevelStochasticRadiosityElement(patch).sourceRad = new ColorRgb(Ed.getR(), Ed.getG(), Ed.getB());
        McradP.getTopLevelPatchReceivedRad(patch)[0] = new ColorRgb(0.0, 0.0, 0.0);

        McradP.topLevelStochasticRadiosityElement(patch).rayIndex = patch.getId() * 11L;
        McradP.topLevelStochasticRadiosityElement(patch).quality = 0.0;
        McradP.topLevelStochasticRadiosityElement(patch).ng = 0;
        McradP.topLevelStochasticRadiosityElement(patch).importance = 0.0;
        McradP.topLevelStochasticRadiosityElement(patch).unShotImportance = 0.0;
        McradP.topLevelStochasticRadiosityElement(patch).receivedImportance = 0.0;
        McradP.topLevelStochasticRadiosityElement(patch).sourceImportance = 0.0;
    }

    /**
Routines below update/re-initialise importance after a viewing change
*/
    private static void monteCarloRadiosityPullImportances(Element element) {
        StochasticRadiosityElement child = (StochasticRadiosityElement)element;
        StochasticRadiosityElement parent = (StochasticRadiosityElement)child.parent;
        double scale = child.area / parent.area;
        parent.importance += scale * child.importance;
        parent.sourceImportance += scale * child.sourceImportance;
        parent.unShotImportance += scale * child.unShotImportance;
    }

    private static void monteCarloRadiosityAccumulateImportances(StochasticRadiosityElement elem) {
        StochasticRelaxation.activeState().totalYmp += elem.area * elem.importance;
        StochasticRelaxation.activeState().sourceYmp += elem.area * elem.sourceImportance;
        StochasticRelaxation.activeState().unShotYmp += elem.area * Math.abs(elem.unShotImportance);
    }

    /**
Calls visit on every child of the element, in the order of
Element.traverseAllChildren, passing the face visibility along. Returns
false for a leaf element.
*/
    private static boolean monteCarloRadiosityVisitChildren(
        Element element,
        BiConsumer<Element, FaceVisibilityTable> visit,
        FaceVisibilityTable faceVisibility)
    {
        if ( element.isCluster() ) {
            for ( int i = 0; element.irregularSubElements != null && i < element.irregularSubElements.size(); i++ ) {
                visit.accept(element.irregularSubElements.get(i), faceVisibility);
            }
            return true;
        }
        if ( element.regularSubElements != null ) {
            for ( int i = 0; i < 4; i++ ) {
                if ( element.regularSubElements[i] != null ) {
                    visit.accept(element.regularSubElements[i], faceVisibility);
                }
            }
            return true;
        }
        return false;
    }

    /**
Update importance in the element hierarchy starting with the top cluster
*/
    private static void monteCarloRadiosityUpdateImportance(Element element, FaceVisibilityTable faceVisibility) {
        StochasticRadiosityElement stochasticRadiosityElement = (StochasticRadiosityElement)element;

        if ( stochasticRadiosityElement == null ) {
            return;
        }

        if ( !monteCarloRadiosityVisitChildren(stochasticRadiosityElement, Mcrad::monteCarloRadiosityUpdateImportance, faceVisibility) ) {
            // Leaf element
            double deltaImp = (double)(faceVisibility.isVisible(stochasticRadiosityElement.patch.getFaceIndex()) ? 1.0 : 0.0) - stochasticRadiosityElement.sourceImportance;
            stochasticRadiosityElement.importance += deltaImp;
            stochasticRadiosityElement.sourceImportance += deltaImp;
            stochasticRadiosityElement.unShotImportance += deltaImp;
            monteCarloRadiosityAccumulateImportances(stochasticRadiosityElement);
        } else {
            // Not a leaf element: clear & pull importance
            stochasticRadiosityElement.importance = stochasticRadiosityElement.sourceImportance = stochasticRadiosityElement.unShotImportance = 0.0;
            stochasticRadiosityElement.traverseAllChildren(Mcrad::monteCarloRadiosityPullImportances);
        }
    }

    /**
Re-init importance in the element hierarchy starting with the top cluster
*/
    private static void monteCarloRadiosityReInitImportance(Element element, FaceVisibilityTable faceVisibility) {
        StochasticRadiosityElement stochasticRadiosityElement = (StochasticRadiosityElement)element;

        if ( stochasticRadiosityElement == null ) {
            return;
        }

        if ( !monteCarloRadiosityVisitChildren(stochasticRadiosityElement, Mcrad::monteCarloRadiosityReInitImportance, faceVisibility) ) {
            // Leaf element
            stochasticRadiosityElement.importance = (double)(faceVisibility.isVisible(stochasticRadiosityElement.patch.getFaceIndex()) ? 1.0 : 0.0);
            stochasticRadiosityElement.sourceImportance = stochasticRadiosityElement.importance;
            stochasticRadiosityElement.unShotImportance = stochasticRadiosityElement.importance;
            monteCarloRadiosityAccumulateImportances(stochasticRadiosityElement);
        } else {
            // Not a leaf element: clear & pull importance
            stochasticRadiosityElement.importance = stochasticRadiosityElement.sourceImportance = stochasticRadiosityElement.unShotImportance = 0.0;
            stochasticRadiosityElement.traverseAllChildren(Mcrad::monteCarloRadiosityPullImportances);
        }
    }

    public static void monteCarloRadiosityUpdateViewImportance(Scene scene, RendererConfiguration renderOptions) {
        System.err.printf("Updating direct visibility ... \n");

        Potential.updateDirectVisibility(scene, renderOptions);

        StochasticRelaxation.activeState().sourceYmp = 0.0;
        StochasticRelaxation.activeState().unShotYmp = 0.0;
        StochasticRelaxation.activeState().totalYmp = 0.0;
        monteCarloRadiosityUpdateImportance(ElementHierarchyState.activeState().topCluster, scene.getFaceVisibility());

        if ( StochasticRelaxation.activeState().unShotYmp < StochasticRelaxation.activeState().sourceYmp ) {
            System.err.printf("Importance will be recomputed incrementally.\n");
            StochasticRelaxation.activeState().importanceUpdatedFromScratch = 0;
        } else {
            System.err.printf("Importance will be recomputed from scratch.\n");
            StochasticRelaxation.activeState().importanceUpdatedFromScratch = 1;

            // Re-compute from scratch
            StochasticRelaxation.activeState().sourceYmp = 0.0;
            StochasticRelaxation.activeState().unShotYmp = 0.0;
            StochasticRelaxation.activeState().totalYmp = 0.0;
            monteCarloRadiosityReInitImportance(ElementHierarchyState.activeState().topCluster, scene.getFaceVisibility());
        }

        scene.camera.changed = 0; // Indicate that direct importance has been computed for this view already
        StochasticRelaxation.activeState().importanceTracedRays = 0; // Start over
        StochasticRelaxation.activeState().importanceUpdated = 1;
    }

    /**
Computes max_i (A_T/A_i): the ratio of the total area over the minimal patch
area in the scene, ignoring the 10% area occupied by the smallest patches
*/
    private static double monteCarloRadiosityDetermineAreaFraction(
        ArrayList<Patch> scenePatches,
        ArrayList<RpkGeometry> sceneGeometries)
    {
        int numberOfPatchIds = Patch.getNextId();

        if ( sceneGeometries == null || sceneGeometries.size() == 0 ) {
            // An arbitrary positive number (in order to avoid divisions by zero
            return 100;
        }

        // Build a table of patch areas
        double[] areas = new double[numberOfPatchIds];
        Arrays.fill(areas, 0.0);
        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            areas[patch.getId()] = patch.getArea();
        }

        // Sort the table to decreasing areas
        Arrays.sort(areas);

        // Find the patch such that 10% of the total surface area is filled by smaller patches
        int i;
        double cumulative;
        for ( i = numberOfPatchIds - 1, cumulative = 0.0; i >= 0 && cumulative < Statistics.instance().radiance.totalArea * 0.1; i-- ) {
            cumulative += areas[i];
        }
        double areaFrac = (i >= 0 && areas[i] > 0.0)
            ? Statistics.instance().radiance.totalArea / areas[i]
            : (double)Statistics.instance().reader.numberOfPatches;

        return areaFrac;
    }

    /**
Determines elementary ray power for the initial incremental iterations
*/
    private static void monteCarloRadiosityDetermineInitialNrRays(
        ArrayList<Patch> scenePatches,
        ArrayList<RpkGeometry> sceneGeometries)
    {
        double areaFrac = monteCarloRadiosityDetermineAreaFraction(scenePatches, sceneGeometries);
        StochasticRelaxation.activeState().initialNumberOfRays = (long)((double)StochasticRelaxation.activeState().rayUnitsPerIt * areaFrac);
    }

    /**
Really initialises: before the first iteration step
*/
    public static void monteCarloRadiosityReInit(Scene scene, RendererConfiguration renderOptions) {
        if ( StochasticRelaxation.activeState().inited != 0 ) {
            return;
        }

        System.err.printf("Initialising Monte Carlo radiosity ...\n");

        Sample4d.setSequence4D(StochasticRelaxation.activeState().sequence);

        StochasticRelaxation.activeState().inited = 1;
        StochasticRelaxation.activeState().cpuSeconds = 0.0;
        StochasticRelaxation.activeState().lastClock = System.nanoTime();
        StochasticRelaxation.activeState().currentIteration = 0;
        StochasticRelaxation.activeState().tracedRays = 0;
        StochasticRelaxation.activeState().prevTracedRays = 0;
        StochasticRelaxation.activeState().numberOfMisses = 0;
        StochasticRelaxation.activeState().importanceTracedRays = 0;
        StochasticRelaxation.activeState().prevImportanceTracedRays = 0;
        StochasticRelaxation.activeState().setSource = StochasticRelaxation.activeState().indirectOnly;
        StochasticRelaxation.activeState().tracedPaths = 0;
        StochasticRelaxation.activeState().controlRadiance = new ColorRgb(0.0, 0.0, 0.0);

        StochasticRelaxation.activeState().unShotFlux = new ColorRgb(0.0, 0.0, 0.0);
        StochasticRelaxation.activeState().unShotYmp = 0.0;
        StochasticRelaxation.activeState().totalFlux = new ColorRgb(0.0, 0.0, 0.0);
        StochasticRelaxation.activeState().totalYmp = 0.0;
        StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux = new ColorRgb(0.0, 0.0, 0.0);
        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            Patch patch = scene.patchList.get(i);
            monteCarloRadiosityInitPatch(patch);
            StochasticRelaxation.activeState().unShotFlux = (StochasticRelaxation.activeState().unShotFlux).addScaled((double)Math.PI * patch.getArea(), McradP.getTopLevelPatchUnShotRad(patch)[0]);
            StochasticRelaxation.activeState().totalFlux = (StochasticRelaxation.activeState().totalFlux).addScaled((double)Math.PI * patch.getArea(), McradP.getTopLevelPatchRad(patch)[0]);
            StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux = (StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux).addScaled((double)Math.PI * patch.getArea() * (McradP.topLevelStochasticRadiosityElement(patch).importance - McradP.topLevelStochasticRadiosityElement(patch).sourceImportance), McradP.getTopLevelPatchUnShotRad(patch)[0]);
            StochasticRelaxation.activeState().unShotYmp += patch.getArea() * Math.abs(McradP.topLevelStochasticRadiosityElement(patch).unShotImportance);
            StochasticRelaxation.activeState().totalYmp += patch.getArea() * McradP.topLevelStochasticRadiosityElement(patch).importance;
            StochasticRelaxation.activeState().sourceYmp += patch.getArea() * McradP.topLevelStochasticRadiosityElement(patch).sourceImportance;
            Mcrad.monteCarloRadiosityPatchComputeNewColor(patch, scene.getFaceColors());
        }

        monteCarloRadiosityDetermineInitialNrRays(scene.patchList, scene.geometryList);

        Hierarchy.elementHierarchyInit(scene.clusteredRootGeometry);

        if ( StochasticRelaxation.activeState().importanceDriven != 0 ) {
            Mcrad.monteCarloRadiosityUpdateViewImportance(scene, renderOptions);
            StochasticRelaxation.activeState().importanceUpdatedFromScratch = 1;
        }
    }

    public static void monteCarloRadiosityPreStep(Scene scene, RendererConfiguration renderOptions) {
        if ( StochasticRelaxation.activeState().inited == 0 ) {
            Mcrad.monteCarloRadiosityReInit(scene, renderOptions);
        }
        if ( StochasticRelaxation.activeState().importanceDriven != 0 && scene.camera.changed != 0 ) {
            Mcrad.monteCarloRadiosityUpdateViewImportance(scene, renderOptions);
        }

        StochasticRelaxation.activeState().lastClock = System.nanoTime();
        StochasticRelaxation.activeState().currentIteration++;
    }

    /**
Undoes the effect of mainInitApplication() and all side-effects of Step()
*/
    public static void monteCarloRadiosityTerminate(ArrayList<Patch> scenePatches) {
        Hierarchy.elementHierarchyTerminate(scenePatches);
        StochasticRelaxation.activeState().inited = 0;
    }

    private static ColorRgb monteCarloRadiosityDiffuseReflectanceAtPoint(Patch patch, double u, double v) {
        RpkRayHit hit = new RpkRayHit();
        Vector3Dd point = patch.uniformPoint(u, v);
        hit.init(patch, point, patch.getNormal(), patch.getMaterial());
        hit.setUv(u, v);
        ColorRgb result = new ColorRgb();
        result = new ColorRgb(0.0, 0.0, 0.0);
        if ( hit.getMaterial().getBsdf() != null ) {
            result = new ColorRgb(hit.getMaterial().getBsdf().splitBsdfScatteredPower(hit.shadingContext(), BsdfComponent.BRDF_DIFFUSE_COMPONENT));
        }
        return result;
    }

    private static ColorRgb vertexReflectance(RpkVertex v) {
        int count = 0;
        ColorRgb rd = new ColorRgb();

        rd = new ColorRgb(0.0, 0.0, 0.0);
        for ( int i = 0; v.radianceData != null && i < v.radianceData.size(); i++ ) {
            Element genericElement = v.radianceData.get(i);
            if ( genericElement.className != ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY ) {
                continue;
            }
            StochasticRadiosityElement element = (StochasticRadiosityElement)genericElement;
            if ( element.regularSubElements == null ) {
                rd = rd.add(element.Rd);
                count++;
            }
        }

        if ( count > 0 ) {
            rd = rd.scaleInverse((double)count);
        }

        return rd;
    }

    private static StochasticRadiosityElement cachedLeaf = null;
    private static ColorRgb[] vrd = new ColorRgb[] {new ColorRgb(), new ColorRgb(), new ColorRgb(), new ColorRgb()};
    private static ColorRgb cachedRd = new ColorRgb();

    private static ColorRgb monteCarloRadiosityInterpolatedReflectanceAtPoint(StochasticRadiosityElement leaf, double u, double v) {
        if ( leaf != null ) {
            if ( leaf != cachedLeaf ) {
                for ( int i = 0; i < leaf.numberOfVertices; i++ ) {
                    vrd[i] = vertexReflectance(leaf.vertices[i]);
                }
            }
            cachedLeaf = leaf;

            cachedRd = new ColorRgb(0.0, 0.0, 0.0);
            switch ( leaf.numberOfVertices ) {
                case 3:
                    cachedRd = new ColorRgb(
                        vrd[0].getR() + (double)u * (vrd[1].getR() - vrd[0].getR()) + (double)v * (vrd[2].getR() - vrd[0].getR()),
                        vrd[0].getG() + (double)u * (vrd[1].getG() - vrd[0].getG()) + (double)v * (vrd[2].getG() - vrd[0].getG()),
                        vrd[0].getB() + (double)u * (vrd[1].getB() - vrd[0].getB()) + (double)v * (vrd[2].getB() - vrd[0].getB()));
                    break;
                case 4:
                    {
                        final double cc = (double)u * (double)v;
                        final double bb = (double)u - cc;
                        final double dd = (double)v - cc;
                        cachedRd = new ColorRgb(
                            vrd[0].getR() + bb * (vrd[1].getR() - vrd[0].getR()) + cc * (vrd[2].getR() - vrd[0].getR()) + dd * (vrd[3].getR() - vrd[0].getR()),
                            vrd[0].getG() + bb * (vrd[1].getG() - vrd[0].getG()) + cc * (vrd[2].getG() - vrd[0].getG()) + dd * (vrd[3].getG() - vrd[0].getG()),
                            vrd[0].getB() + bb * (vrd[1].getB() - vrd[0].getB()) + cc * (vrd[2].getB() - vrd[0].getB()) + dd * (vrd[3].getB() - vrd[0].getB()));
                    }
                    break;
                default:
                    Logger.reportMessage(null, Logger.FATAL_ERROR, "monteCarloRadiosityInterpolatedReflectanceAtPoint", String.format("Invalid nr of vertices %d", leaf.numberOfVertices));
            }
        }
        return cachedRd;
    }

    /**
Returns the radiance emitted from the patch at the point with parameters
(u,v) into the direction 'dir'
*/
    public static ColorRgb monteCarloRadiosityGetRadiance(Patch patch, double u, double v, Vector3Dd dir, RendererConfiguration renderOptions) {
        ColorRgb trueRdAtPoint = monteCarloRadiosityDiffuseReflectanceAtPoint(patch, u, v);
        double[] uu = new double[] {u};
        double[] vv = new double[] {v};
        StochasticRadiosityElement leaf = StochasticRadiosityElement.stochasticRadiosityElementRegularLeafElementAtPoint(
            McradP.topLevelStochasticRadiosityElement(patch), uu, vv);
        ColorRgb usedRdAtPoint = renderOptions.smoothShading
            ? monteCarloRadiosityInterpolatedReflectanceAtPoint(leaf, uu[0], vv[0])
            : leaf.Rd;
        ColorRgb radianceAtPoint = StochasticRadiosityElement.stochasticRadiosityElementDisplayRadianceAtPoint(leaf, uu[0], vv[0], renderOptions);
        ColorRgb sourceRad = new ColorRgb();
        sourceRad = new ColorRgb(0.0, 0.0, 0.0);

        // Subtract source radiance
        if ( StochasticRelaxation.activeState().show != WhatToShow.SHOW_INDIRECT_RADIANCE ) {
            // sourceRad is self-emitted radiance when indirect-only is disabled.
            // Otherwise it represents direct illumination.
            if ( StochasticRelaxation.activeState().doNonDiffuseFirstShot == 0 ) {
                sourceRad = leaf.sourceRad;
            }
            if ( StochasticRelaxation.activeState().indirectOnly != 0 || StochasticRelaxation.activeState().doNonDiffuseFirstShot != 0 ) {
                // Subtract self-emitted radiance
                sourceRad = sourceRad.add(leaf.Ed);
            }
        }
        radianceAtPoint = radianceAtPoint.subtract(sourceRad);

        radianceAtPoint = radianceAtPoint.multiply(trueRdAtPoint);
        radianceAtPoint = radianceAtPoint.divide(usedRdAtPoint);

        // Re-add source radiance
        radianceAtPoint = radianceAtPoint.add(sourceRad);

        return radianceAtPoint;
    }

    /**
Returns scalar reflectance, for importance propagation
*/
    public static double monteCarloRadiosityScalarReflectance(Patch P) {
        return StochasticRadiosityElement.stochasticRadiosityElementScalarReflectance(McradP.topLevelStochasticRadiosityElement(P));
    }
}
