/**
Monte Carlo radiosity
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.util.ArrayList;
import java.util.Arrays;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.skin.RayHitFlag;
import vsdk.toolkit.render.Potential;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.Element;
import vsdk.toolkit.skin.ElementTypes;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.RayHit;
import vsdk.toolkit.skin.Vertex;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

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
        StochasticRelaxation.activeState().controlRadiance.clear();
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
        StochasticRelaxation.activeState().cpuSeconds +=
            (float)((double)(t - StochasticRelaxation.activeState().lastClock) / 1000000000.0);
        StochasticRelaxation.activeState().lastClock = t;
    }

    public static Element monteCarloRadiosityCreatePatchData(Patch patch) {
        patch.radianceData = StochasticRadiosityElement.stochasticRadiosityElementCreateFromPatch(patch);
        return patch.radianceData;
    }

    public static void monteCarloRadiosityDestroyPatchData(Patch patch) {
        if ( patch.radianceData != null ) {
            StochasticRadiosityElement.stochasticRadiosityElementDestroy(McradP.topLevelStochasticRadiosityElement(patch));
        }
        patch.radianceData = null;
    }

    /**
Compute new color for the patch: fine if no hierarchical refinement is used, e.g.
in the current random walk radiosity implementation
*/
    public static void monteCarloRadiosityPatchComputeNewColor(Patch patch) {
        patch.color = StochasticRadiosityElement.stochasticRadiosityElementColor(McradP.topLevelStochasticRadiosityElement(patch));
        patch.computeVertexColors();
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

        McradP.getTopLevelPatchRad(patch)[0] = new ColorRgb(Ed.r, Ed.g, Ed.b);
        McradP.getTopLevelPatchUnShotRad(patch)[0] = new ColorRgb(Ed.r, Ed.g, Ed.b);
        McradP.topLevelStochasticRadiosityElement(patch).sourceRad = new ColorRgb(Ed.r, Ed.g, Ed.b);
        McradP.getTopLevelPatchReceivedRad(patch)[0].clear();

        McradP.topLevelStochasticRadiosityElement(patch).rayIndex = patch.id * 11L;
        McradP.topLevelStochasticRadiosityElement(patch).quality = 0.0f;
        McradP.topLevelStochasticRadiosityElement(patch).ng = 0;
        McradP.topLevelStochasticRadiosityElement(patch).importance = 0.0f;
        McradP.topLevelStochasticRadiosityElement(patch).unShotImportance = 0.0f;
        McradP.topLevelStochasticRadiosityElement(patch).receivedImportance = 0.0f;
        McradP.topLevelStochasticRadiosityElement(patch).sourceImportance = 0.0f;
    }

    /**
Routines below update/re-initialise importance after a viewing change
*/
    private static void monteCarloRadiosityPullImportances(Element element) {
        StochasticRadiosityElement child = (StochasticRadiosityElement)element;
        StochasticRadiosityElement parent = (StochasticRadiosityElement)child.parent;
        float scale = child.area / parent.area;
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
Update importance in the element hierarchy starting with the top cluster
*/
    private static void monteCarloRadiosityUpdateImportance(Element element) {
        StochasticRadiosityElement stochasticRadiosityElement = (StochasticRadiosityElement)element;

        if ( stochasticRadiosityElement == null ) {
            return;
        }

        if ( !stochasticRadiosityElement.traverseAllChildren(Mcrad::monteCarloRadiosityUpdateImportance) ) {
            // Leaf element
            float deltaImp = (float)(stochasticRadiosityElement.patch.isVisible() ? 1.0 : 0.0) - stochasticRadiosityElement.sourceImportance;
            stochasticRadiosityElement.importance += deltaImp;
            stochasticRadiosityElement.sourceImportance += deltaImp;
            stochasticRadiosityElement.unShotImportance += deltaImp;
            monteCarloRadiosityAccumulateImportances(stochasticRadiosityElement);
        } else {
            // Not a leaf element: clear & pull importance
            stochasticRadiosityElement.importance = stochasticRadiosityElement.sourceImportance = stochasticRadiosityElement.unShotImportance = 0.0f;
            stochasticRadiosityElement.traverseAllChildren(Mcrad::monteCarloRadiosityPullImportances);
        }
    }

    /**
Re-init importance in the element hierarchy starting with the top cluster
*/
    private static void monteCarloRadiosityReInitImportance(Element element) {
        StochasticRadiosityElement stochasticRadiosityElement = (StochasticRadiosityElement)element;

        if ( stochasticRadiosityElement == null ) {
            return;
        }

        if ( !stochasticRadiosityElement.traverseAllChildren(Mcrad::monteCarloRadiosityReInitImportance) ) {
            // Leaf element
            stochasticRadiosityElement.importance = (float)(stochasticRadiosityElement.patch.isVisible() ? 1.0 : 0.0);
            stochasticRadiosityElement.sourceImportance = stochasticRadiosityElement.importance;
            stochasticRadiosityElement.unShotImportance = stochasticRadiosityElement.importance;
            monteCarloRadiosityAccumulateImportances(stochasticRadiosityElement);
        } else {
            // Not a leaf element: clear & pull importance
            stochasticRadiosityElement.importance = stochasticRadiosityElement.sourceImportance = stochasticRadiosityElement.unShotImportance = 0.0f;
            stochasticRadiosityElement.traverseAllChildren(Mcrad::monteCarloRadiosityPullImportances);
        }
    }

    public static void monteCarloRadiosityUpdateViewImportance(Scene scene, RenderOptions renderOptions) {
        System.err.printf("Updating direct visibility ... \n");

        Potential.updateDirectVisibility(scene, renderOptions);

        StochasticRelaxation.activeState().sourceYmp = 0.0f;
        StochasticRelaxation.activeState().unShotYmp = 0.0f;
        StochasticRelaxation.activeState().totalYmp = 0.0f;
        monteCarloRadiosityUpdateImportance(ElementHierarchyState.activeState().topCluster);

        if ( StochasticRelaxation.activeState().unShotYmp < StochasticRelaxation.activeState().sourceYmp ) {
            System.err.printf("Importance will be recomputed incrementally.\n");
            StochasticRelaxation.activeState().importanceUpdatedFromScratch = 0;
        } else {
            System.err.printf("Importance will be recomputed from scratch.\n");
            StochasticRelaxation.activeState().importanceUpdatedFromScratch = 1;

            // Re-compute from scratch
            StochasticRelaxation.activeState().sourceYmp = 0.0f;
            StochasticRelaxation.activeState().unShotYmp = 0.0f;
            StochasticRelaxation.activeState().totalYmp = 0.0f;
            monteCarloRadiosityReInitImportance(ElementHierarchyState.activeState().topCluster);
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
        ArrayList<Geometry> sceneGeometries)
    {
        int numberOfPatchIds = Patch.getNextId();

        if ( sceneGeometries == null || sceneGeometries.size() == 0 ) {
            // An arbitrary positive number (in order to avoid divisions by zero
            return 100;
        }

        // Build a table of patch areas
        float[] areas = new float[numberOfPatchIds];
        Arrays.fill(areas, 0.0f);
        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            areas[patch.id] = patch.area;
        }

        // Sort the table to decreasing areas
        Arrays.sort(areas);

        // Find the patch such that 10% of the total surface area is filled by smaller patches
        int i;
        float cumulative;
        for ( i = numberOfPatchIds - 1, cumulative = 0.0f; i >= 0 && cumulative < Statistics.instance().radiance.totalArea * 0.1f; i-- ) {
            cumulative += areas[i];
        }
        float areaFrac = (i >= 0 && areas[i] > 0.0f)
            ? Statistics.instance().radiance.totalArea / areas[i]
            : (float)Statistics.instance().reader.numberOfPatches;

        return areaFrac;
    }

    /**
Determines elementary ray power for the initial incremental iterations
*/
    private static void monteCarloRadiosityDetermineInitialNrRays(
        ArrayList<Patch> scenePatches,
        ArrayList<Geometry> sceneGeometries)
    {
        double areaFrac = monteCarloRadiosityDetermineAreaFraction(scenePatches, sceneGeometries);
        StochasticRelaxation.activeState().initialNumberOfRays =
            (long)((double)StochasticRelaxation.activeState().rayUnitsPerIt * areaFrac);
    }

    /**
Really initialises: before the first iteration step
*/
    public static void monteCarloRadiosityReInit(Scene scene, RenderOptions renderOptions) {
        if ( StochasticRelaxation.activeState().inited != 0 ) {
            return;
        }

        System.err.printf("Initialising Monte Carlo radiosity ...\n");

        Sample4d.setSequence4D(StochasticRelaxation.activeState().sequence);

        StochasticRelaxation.activeState().inited = 1;
        StochasticRelaxation.activeState().cpuSeconds = 0.0f;
        StochasticRelaxation.activeState().lastClock = System.nanoTime();
        StochasticRelaxation.activeState().currentIteration = 0;
        StochasticRelaxation.activeState().tracedRays = 0;
        StochasticRelaxation.activeState().prevTracedRays = 0;
        StochasticRelaxation.activeState().numberOfMisses = 0;
        StochasticRelaxation.activeState().importanceTracedRays = 0;
        StochasticRelaxation.activeState().prevImportanceTracedRays = 0;
        StochasticRelaxation.activeState().setSource = StochasticRelaxation.activeState().indirectOnly;
        StochasticRelaxation.activeState().tracedPaths = 0;
        StochasticRelaxation.activeState().controlRadiance.clear();

        StochasticRelaxation.activeState().unShotFlux.clear();
        StochasticRelaxation.activeState().unShotYmp = 0.0f;
        StochasticRelaxation.activeState().totalFlux.clear();
        StochasticRelaxation.activeState().totalYmp = 0.0f;
        StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.clear();
        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            Patch patch = scene.patchList.get(i);
            monteCarloRadiosityInitPatch(patch);
            StochasticRelaxation.activeState().unShotFlux.addScaled(
                StochasticRelaxation.activeState().unShotFlux,
                (float)Math.PI * patch.area,
                McradP.getTopLevelPatchUnShotRad(patch)[0]);
            StochasticRelaxation.activeState().totalFlux.addScaled(
                StochasticRelaxation.activeState().totalFlux,
                (float)Math.PI * patch.area,
                McradP.getTopLevelPatchRad(patch)[0]);
            StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.addScaled(
                StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux,
                (float)Math.PI * patch.area *
                (McradP.topLevelStochasticRadiosityElement(patch).importance - McradP.topLevelStochasticRadiosityElement(patch).sourceImportance),
                McradP.getTopLevelPatchUnShotRad(patch)[0]);
            StochasticRelaxation.activeState().unShotYmp += patch.area * Math.abs(McradP.topLevelStochasticRadiosityElement(patch).unShotImportance);
            StochasticRelaxation.activeState().totalYmp += patch.area * McradP.topLevelStochasticRadiosityElement(patch).importance;
            StochasticRelaxation.activeState().sourceYmp += patch.area * McradP.topLevelStochasticRadiosityElement(patch).sourceImportance;
            Mcrad.monteCarloRadiosityPatchComputeNewColor(patch);
        }

        monteCarloRadiosityDetermineInitialNrRays(scene.patchList, scene.geometryList);

        Hierarchy.elementHierarchyInit(scene.clusteredRootGeometry);

        if ( StochasticRelaxation.activeState().importanceDriven != 0 ) {
            Mcrad.monteCarloRadiosityUpdateViewImportance(scene, renderOptions);
            StochasticRelaxation.activeState().importanceUpdatedFromScratch = 1;
        }
    }

    public static void monteCarloRadiosityPreStep(Scene scene, RenderOptions renderOptions) {
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
        RayHit hit = new RayHit();
        Vector3D point = new Vector3D();
        patch.uniformPoint(u, v, point);
        hit.init(patch, point, patch.normal, patch.material);
        hit.setUv(u, v);
        int newFlags = hit.getFlags() | RayHitFlag.UV;
        hit.setFlags(newFlags);
        ColorRgb result = new ColorRgb();
        result.clear();
        if ( hit.getMaterial().getBsdf() != null ) {
            result = hit.getMaterial().getBsdf().splitBsdfScatteredPower(hit, BsdfComponent.BRDF_DIFFUSE_COMPONENT);
        }
        return result;
    }

    private static ColorRgb vertexReflectance(Vertex v) {
        int count = 0;
        ColorRgb rd = new ColorRgb();

        rd.clear();
        for ( int i = 0; v.radianceData != null && i < v.radianceData.size(); i++ ) {
            Element genericElement = v.radianceData.get(i);
            if ( genericElement.className != ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY ) {
                continue;
            }
            StochasticRadiosityElement element = (StochasticRadiosityElement)genericElement;
            if ( element.regularSubElements == null ) {
                rd.add(rd, element.Rd);
                count++;
            }
        }

        if ( count > 0 ) {
            rd.scaleInverse((float)count, rd);
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

            cachedRd.clear();
            switch ( leaf.numberOfVertices ) {
                case 3:
                    cachedRd.interpolateBarycentric(vrd[0], vrd[1], vrd[2], (float)u, (float)v);
                    break;
                case 4:
                    cachedRd.interpolateBiLinear(vrd[0], vrd[1], vrd[2], vrd[3], (float)u, (float)v);
                    break;
                default:
                    Error.fatal(-1, "monteCarloRadiosityInterpolatedReflectanceAtPoint", "Invalid nr of vertices %d",
                        leaf.numberOfVertices);
            }
        }
        return cachedRd;
    }

    /**
Returns the radiance emitted from the patch at the point with parameters
(u,v) into the direction 'dir'
*/
    public static ColorRgb monteCarloRadiosityGetRadiance(Patch patch, double u, double v, Vector3D dir, RenderOptions renderOptions) {
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
        sourceRad.clear();

        // Subtract source radiance
        if ( StochasticRelaxation.activeState().show != WhatToShow.SHOW_INDIRECT_RADIANCE ) {
            // sourceRad is self-emitted radiance when indirect-only is disabled.
            // Otherwise it represents direct illumination.
            if ( StochasticRelaxation.activeState().doNonDiffuseFirstShot == 0 ) {
                sourceRad = leaf.sourceRad;
            }
            if ( StochasticRelaxation.activeState().indirectOnly != 0 || StochasticRelaxation.activeState().doNonDiffuseFirstShot != 0 ) {
                // Subtract self-emitted radiance
                sourceRad.add(sourceRad, leaf.Ed);
            }
        }
        radianceAtPoint.subtract(radianceAtPoint, sourceRad);

        radianceAtPoint.scalarProduct(radianceAtPoint, trueRdAtPoint);
        radianceAtPoint.divide(radianceAtPoint, usedRdAtPoint);

        // Re-add source radiance
        radianceAtPoint.add(radianceAtPoint, sourceRad);

        return radianceAtPoint;
    }

    /**
Returns scalar reflectance, for importance propagation
*/
    public static float monteCarloRadiosityScalarReflectance(Patch P) {
        return StochasticRadiosityElement.stochasticRadiosityElementScalarReflectance(McradP.topLevelStochasticRadiosityElement(P));
    }
}
