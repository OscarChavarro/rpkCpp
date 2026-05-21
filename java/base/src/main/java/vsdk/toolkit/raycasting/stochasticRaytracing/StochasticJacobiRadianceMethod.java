package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Locale;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.RadianceMethodAlgorithm;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;

/**
Stochastic Relaxation Radiosity (currently only stochastic Jacobi)
*/
public final class StochasticJacobiRadianceMethod extends RadianceMethod {
    private static final int STRING_LENGTH = 2000;
    private final StochasticRelaxation stochasticRelaxationState;
    private final ElementHierarchyState elementHierarchyState;
    private final StochasticRadiosityBasisState stochasticRadiosityBasisState;

    private static void appendStochasticStatsText(StringBuilder buffer, int[] offset, String format, Object... args) {
        if ( offset[0] >= STRING_LENGTH - 1 ) {
            return;
        }

        String text;
        try {
            text = String.format(Locale.US, format, args);
        } catch ( Exception e ) {
            text = format;
        }

        int available = STRING_LENGTH - offset[0];
        if ( available <= 0 ) {
            return;
        }
        if ( text.length() >= available ) {
            buffer.append(text, 0, available - 1);
            offset[0] = STRING_LENGTH - 1;
        } else {
            buffer.append(text);
            offset[0] += text.length();
        }
    }

    public StochasticJacobiRadianceMethod(
        StochasticRelaxation inStochasticRelaxationState,
        ElementHierarchyState inElementHierarchyState,
        StochasticRadiosityBasisState inStochasticRadiosityBasisState)
    {
        stochasticRelaxationState = inStochasticRelaxationState;
        elementHierarchyState = inElementHierarchyState;
        stochasticRadiosityBasisState = inStochasticRadiosityBasisState;

        StochasticRelaxation.setActiveState(stochasticRelaxationState);
        ElementHierarchyState.setActiveState(elementHierarchyState);
        StochasticRadiosityBasisState.setActiveState(stochasticRadiosityBasisState);
        Mcrad.monteCarloRadiosityDefaults();
        className = RadianceMethodAlgorithm.STOCHASTIC_JACOBI;
    }

    @Override
    public String getRadianceMethodName() {
        return "Stochastic Jacobi";
    }

    @Override
    public void parseOptions(int[] argc, String[] argv) {
    }

    @Override
    public void terminate(ArrayList<Patch> scenePatches) {
        StochasticRelaxation.setActiveState(stochasticRelaxationState);
        ElementHierarchyState.setActiveState(elementHierarchyState);
        StochasticRadiosityBasisState.setActiveState(stochasticRadiosityBasisState);
        Mcrad.monteCarloRadiosityTerminate(scenePatches);
    }

    @Override
    public ColorRgb getRadiance(
        Camera camera,
        Patch patch,
        double u,
        double v,
        Vector3D dir,
        RenderOptions renderOptions)
    {
        StochasticRelaxation.setActiveState(stochasticRelaxationState);
        ElementHierarchyState.setActiveState(elementHierarchyState);
        StochasticRadiosityBasisState.setActiveState(stochasticRadiosityBasisState);
        if ( camera == null ) {
            // camera is intentionally unused in C++ implementation.
        }
        return Mcrad.monteCarloRadiosityGetRadiance(patch, u, v, dir, renderOptions);
    }

    @Override
    public Element createPatchData(Patch patch) {
        return Mcrad.monteCarloRadiosityCreatePatchData(patch);
    }

    @Override
    public void destroyPatchData(Patch patch) {
        Mcrad.monteCarloRadiosityDestroyPatchData(patch);
    }

    @Override
    public void writeVRML(
        Camera camera,
        OutputStream outputStream,
        RenderOptions renderOptions)
    {
        if ( camera == null || outputStream == null || renderOptions == null ) {
            // Not implemented in C++ version either.
        }
    }

    @Override
    public void initialize(Scene scene, ToneMappingContext toneMapOptions) {
        StochasticRelaxation.setActiveState(stochasticRelaxationState);
        ElementHierarchyState.setActiveState(elementHierarchyState);
        StochasticRadiosityBasisState.setActiveState(stochasticRadiosityBasisState);
        if ( scene == null ) {
            // scene is intentionally unused in C++ implementation.
        }
        StochasticRelaxation.activeState().toneMapOptions = toneMapOptions;
        if ( StochasticRelaxation.activeState().toneMapOptions == null ) {
            Logger.fatal(-1, "StochasticJacobiRadianceMethod::initialize", "Tone mapping context not provided");
        }
        StochasticRelaxation.activeState().method = StochasticRaytracingMethod.STOCHASTIC_RELAXATION_RADIOSITY_METHOD;
        Mcrad.monteCarloRadiosityInit();
    }

    @Override
    public String getStats() {
        StochasticRelaxation.setActiveState(stochasticRelaxationState);
        ElementHierarchyState.setActiveState(elementHierarchyState);
        StochasticRadiosityBasisState.setActiveState(stochasticRadiosityBasisState);

        StringBuilder stats = new StringBuilder(STRING_LENGTH);
        int[] statsOffset = new int[] {0};

        appendStochasticStatsText(stats, statsOffset, "Stochastic Relaxation Radiosity\nStatistics\n\n");
        appendStochasticStatsText(stats, statsOffset, "Iteration nr: %d\n", StochasticRelaxation.activeState().currentIteration);
        appendStochasticStatsText(stats, statsOffset, "CPU time: %g secs\n", StochasticRelaxation.activeState().cpuSeconds);
        appendStochasticStatsText(
            stats, statsOffset, "%d elements (%d clusters, %d surfaces)\n",
            ElementHierarchyState.activeState().nr_elements,
            ElementHierarchyState.activeState().nr_clusters,
            ElementHierarchyState.activeState().nr_elements - ElementHierarchyState.activeState().nr_clusters);
        appendStochasticStatsText(stats, statsOffset, "Radiance rays: %d\n", StochasticRelaxation.activeState().tracedRays);
        appendStochasticStatsText(stats, statsOffset, "Importance rays: %d\n", StochasticRelaxation.activeState().importanceTracedRays);

        return stats.toString();
    }

    /**
Randomly returns floor(x) or ceil(x) so that the expected value is equal to x
*/
    private static long stochasticRelaxationRadiosityRandomRound(float x) {
        long l = (long)Math.floor(x);
        if ( Math.random() < (x - (float)l) ) {
            l++;
        }
        return l;
    }

    private static void stochasticRelaxationRadiosityRecomputeDisplayColors(ArrayList<Patch> scenePatches) {
        StochasticRadiosityElement topElement = ElementHierarchyState.activeState().topCluster;
        if ( topElement != null ) {
            topElement.traverseClusterLeafElements(StochasticRadiosityElement::stochasticRadiosityElementComputeNewVertexColors);
            topElement.traverseClusterLeafElements(StochasticRadiosityElement::stochasticRadiosityElementAdjustTVertexColors);
        } else {
            for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
                Mcrad.monteCarloRadiosityPatchComputeNewColor(scenePatches.get(i));
            }
        }
    }

    /**
Computes quality factor on given leaf element (see PhD Phillipe Bekaert p.152).
In the basic algorithms by Neumann et al. the quality factor would
correspond to the inverse of the elementary ray power. The quality factor
indicates the quality of the radiosity solution on a given leaf element.
The quality factor after different iterations is additive. It is used in order
to properly merge the result of new iterations with the result of previous
iterations properly taking into account the number of rays and importance
distribution
*/
    private static double stochasticRelaxationRadiosityQualityFactor(StochasticRadiosityElement elem, double w) {
        if ( StochasticRelaxation.activeState().importanceDriven != 0 ) {
            return w * elem.importance;
        }
        return w / StochasticRadiosityElement.stochasticRadiosityElementScalarReflectance(elem);
    }

    private static ColorRgb[] stochasticRelaxationRadiosityElementUnShotRadiance(StochasticRadiosityElement elem) {
        return elem.unShotRadiance;
    }

    private static void stochasticRelaxationRadiosityElementIncrementRadiance(StochasticRadiosityElement elem, double w) {
        // Each incremental iteration computes a different contribution to the
        // solution. The quality factor of the result remains constant
        if ( StochasticRelaxation.activeState().discardIncremental != 0 ) {
            elem.quality = 0.0f;
            if ( !repeatedDiscardIncrementalWarning ) {
                Logger.warning("stochasticRelaxationRadiosityElementIncrementRadiance",
                    "Solution of incremental Jacobi steps receives zero quality");
            }
            repeatedDiscardIncrementalWarning = true;
        } else {
            elem.quality = (float)stochasticRelaxationRadiosityQualityFactor(elem, w);
        }

        Coefficientsmcrad.stochasticRadiosityAddCoefficients(elem.radiance, elem.receivedRadiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityCopyCoefficients(elem.unShotRadiance, elem.receivedRadiance, elem.basis);
        if ( StochasticRelaxation.activeState().setSource != 0 ) {
            // Copy direct illumination and forget self emitted illumination
            elem.radiance[0].set(elem.receivedRadiance[0].getR(), elem.receivedRadiance[0].getG(), elem.receivedRadiance[0].getB());
            elem.sourceRad.set(elem.receivedRadiance[0].getR(), elem.receivedRadiance[0].getG(), elem.receivedRadiance[0].getB());
        }
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
    }

    private static void stochasticRelaxationRadiosityPrintIncrementalRadianceStats() {
        System.err.printf("%g secs., radiance rays = %d (%d not to background), un-shot flux = ",
            StochasticRelaxation.activeState().cpuSeconds,
            StochasticRelaxation.activeState().tracedRays,
            StochasticRelaxation.activeState().tracedRays - StochasticRelaxation.activeState().numberOfMisses);
        StochasticRelaxation.activeState().unShotFlux.print(System.err);
        System.err.printf(", total flux = ");
        StochasticRelaxation.activeState().totalFlux.print(System.err);
        System.err.printf(", indirect importance weighted un-shot flux = ");
        StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.print(System.err);
        System.err.printf("\n");
    }

    private static void stochasticRelaxationRadiosityDoIncrementalRadianceIterations(
        Scene scene,
        RadianceMethod radianceMethod,
        RenderOptions renderOptions)
    {
        double refUnShot;
        long stepNumber = 0;

        int weightedSampling = StochasticRelaxation.activeState().weightedSampling;
        int importanceDriven = StochasticRelaxation.activeState().importanceDriven;
        if ( StochasticRelaxation.activeState().incrementalUsesImportance == 0 ) {
            // Temporarily switch it off
            StochasticRelaxation.activeState().importanceDriven = 0;
        }
        StochasticRelaxation.activeState().weightedSampling = 0;

        stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
        refUnShot = StochasticRelaxation.activeState().unShotFlux.sumAbsComponents();
        if ( StochasticRelaxation.activeState().incrementalUsesImportance != 0 ) {
            refUnShot = StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.sumAbsComponents();
        }
        while ( true ) {
            // Choose nr of rays so that power carried by each ray remains equal, and
            // proportional to the number of basis functions in the rad. approx
            double unShotFraction;
            long numberOfRays;
            unShotFraction = StochasticRelaxation.activeState().unShotFlux.sumAbsComponents() / refUnShot;
            if ( StochasticRelaxation.activeState().incrementalUsesImportance != 0 ) {
                unShotFraction = StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.sumAbsComponents() / refUnShot;
            }
            if ( unShotFraction < 0.01 ) {
                // Only 1/100th of self-emitted power remains un-shot
                break;
            }
            numberOfRays = stochasticRelaxationRadiosityRandomRound(
                (float)(unShotFraction * (double)StochasticRelaxation.activeState().initialNumberOfRays *
                    StochasticRadiosityBasisState.activeState()
                        .approxDesc[StochasticRelaxation.activeState().approximationOrderType.ordinal()].basis_size));

            stepNumber++;
            System.err.printf("Incremental radiance propagation step %d: %.3f%% un-shot power left.\n",
                stepNumber, 100.0 * unShotFraction);

            StochasticJacobi.doStochasticJacobiIteration(
                scene.voxelGrid,
                numberOfRays,
                StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementUnShotRadiance,
                null,
                StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementIncrementRadiance,
                scene.patchList,
                renderOptions);

            StochasticRelaxation.activeState().setSource = 0; // Direct illumination is copied to SOURCE_FLUX(P) only the first time

            Mcrad.monteCarloRadiosityUpdateCpuSecs();
            stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
            if ( unShotFraction > 0.3 ) {
                stochasticRelaxationRadiosityRecomputeDisplayColors(scene.patchList);
            }
        }

        StochasticRelaxation.activeState().importanceDriven = importanceDriven; // Switch it back on if it was on
        StochasticRelaxation.activeState().weightedSampling = weightedSampling;

        if ( radianceMethod == null ) {
            // radianceMethod is intentionally unused in C++ implementation.
        }
    }

    private static float stochasticRelaxationRadiosityElementUnShotImportance(StochasticRadiosityElement elem) {
        return elem.unShotImportance;
    }

    private static void stochasticRelaxationRadiosityElementIncrementImportance(StochasticRadiosityElement elem, double w) {
        if ( w < -1 ) {
            // Keep C++ signature.
        }
        elem.importance += elem.receivedImportance;
        elem.unShotImportance = elem.receivedImportance;
        elem.receivedImportance = 0.0f;
    }

    private static void stochasticRelaxationRadiosityPrintIncrementalImportanceStats() {
        System.err.printf(
            "%g secs., importance rays = %d, un-shot importance = %g, total importance = %g, total area = %g\n",
            StochasticRelaxation.activeState().cpuSeconds,
            StochasticRelaxation.activeState().importanceTracedRays,
            StochasticRelaxation.activeState().unShotYmp,
            StochasticRelaxation.activeState().totalYmp,
            Statistics.instance().radiance.totalArea);
    }

    private static void stochasticRelaxationRadiosityDoIncrementalImportanceIterations(
        VoxelGrid sceneWorldVoxelGrid,
        ArrayList<Patch> scenePatches,
        RenderOptions renderOptions)
    {
        long stepNumber = 0;
        int radianceDriven = StochasticRelaxation.activeState().radianceDriven;
        int doHMeshing = ElementHierarchyState.activeState().do_h_meshing;
        HierarchyClusteringMode clustering = ElementHierarchyState.activeState().clustering;
        int weightedSampling = StochasticRelaxation.activeState().weightedSampling;

        if ( StochasticRelaxation.activeState().sourceYmp < Numeric.EPSILON ) {
            System.err.printf("No source importance!!\n");
            return;
        }

        StochasticRelaxation.activeState().radianceDriven = 0; // Temporary switch it off
        ElementHierarchyState.activeState().do_h_meshing = 0;
        ElementHierarchyState.activeState().clustering = HierarchyClusteringMode.NO_CLUSTERING;
        StochasticRelaxation.activeState().weightedSampling = 0;

        stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
        while ( true ) {
            // Choose nr of rays so that power carried by each ray is the same, and
            // proportional to the number of basis functions in the rad. approx. */
            double unShotFraction = StochasticRelaxation.activeState().unShotYmp / StochasticRelaxation.activeState().sourceYmp;
            long numberOfRays = stochasticRelaxationRadiosityRandomRound(
                (float)unShotFraction * (float)StochasticRelaxation.activeState().initialNumberOfRays);
            if ( unShotFraction < 0.01 ) {
                break;
            }

            stepNumber++;
            System.err.printf("Incremental importance propagation step %d: %.3f%% un-shot importance left.\n",
                stepNumber, 100.0 * unShotFraction);

            StochasticJacobi.doStochasticJacobiIteration(
                sceneWorldVoxelGrid,
                numberOfRays,
                null,
                StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementUnShotImportance,
                StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementIncrementImportance,
                scenePatches,
                renderOptions);

            Mcrad.monteCarloRadiosityUpdateCpuSecs();
            stochasticRelaxationRadiosityPrintIncrementalImportanceStats();
        }

        StochasticRelaxation.activeState().radianceDriven = radianceDriven; // Switch on again
        ElementHierarchyState.activeState().do_h_meshing = doHMeshing;
        ElementHierarchyState.activeState().clustering = clustering;
        StochasticRelaxation.activeState().weightedSampling = weightedSampling;
    }

    private static ColorRgb[] stochasticRelaxationRadiosityElementRadiance(StochasticRadiosityElement elem) {
        return elem.radiance;
    }

    private static void stochasticRelaxationRadiosityElementUpdateRadiance(StochasticRadiosityElement elem, double w) {
        double k = (double)StochasticRelaxation.activeState().prevTracedRays /
            (double)(StochasticRelaxation.activeState().tracedRays > 0 ? StochasticRelaxation.activeState().tracedRays : 1);

        if ( StochasticRelaxation.activeState().naiveMerging == 0 ) {
            double quality = stochasticRelaxationRadiosityQualityFactor(elem, w);
            if ( elem.quality < Numeric.EPSILON ) {
                // Solution of this iteration takes over
                k = 0.0;
            } else if ( quality < Numeric.EPSILON ) {
                // Keep result of previous iterations
                k = 1.0;
            } else if ( elem.quality + quality > Numeric.EPSILON ) {
                k = elem.quality / (elem.quality + quality);
            } else {
                // Quality of new solution is so high that it must take over
                k = 0.0;
            }
            elem.quality += (float)quality; // Add quality
        }

        // Subtract source radiosity
        elem.radiance[0].subtract(elem.radiance[0], elem.sourceRad);

        // Combine with previous results
        Coefficientsmcrad.stochasticRadiosityScaleCoefficients((float)k, elem.radiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityScaleCoefficients((1.0f - (float)k), elem.receivedRadiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityAddCoefficients(elem.radiance, elem.receivedRadiance, elem.basis);

        // Re-add source radiosity
        elem.radiance[0].add(elem.radiance[0], elem.sourceRad);

        // Clear un-shot and received radiance
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
    }

    private static void stochasticRelaxationRadiosityPrintRegularStats() {
        System.err.printf("%g secs., radiance rays = %d (%d not to background), un-shot flux = ",
            StochasticRelaxation.activeState().cpuSeconds,
            StochasticRelaxation.activeState().tracedRays,
            StochasticRelaxation.activeState().tracedRays - StochasticRelaxation.activeState().numberOfMisses);
        System.err.printf(", total flux = ");
        StochasticRelaxation.activeState().totalFlux.print(System.err);
        if ( StochasticRelaxation.activeState().importanceDriven != 0 ) {
            System.err.printf("\ntotal importance rays = %d, total importance = %g, total area = %g",
                StochasticRelaxation.activeState().importanceTracedRays,
                StochasticRelaxation.activeState().totalYmp,
                Statistics.instance().radiance.totalArea);
        }
        System.err.printf("\n");
    }

    private static void stochasticRelaxationRadiosityDoRegularRadianceIteration(
        VoxelGrid sceneWorldVoxelGrid,
        ArrayList<Patch> scenePatches,
        RenderOptions renderOptions)
    {
        System.err.printf("Regular radiance iteration %d:\n", StochasticRelaxation.activeState().currentIteration);
        StochasticJacobi.doStochasticJacobiIteration(
            sceneWorldVoxelGrid,
            StochasticRelaxation.activeState().raysPerIteration,
            StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementRadiance,
            null,
            StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementUpdateRadiance,
            scenePatches,
            renderOptions);

        Mcrad.monteCarloRadiosityUpdateCpuSecs();
        stochasticRelaxationRadiosityPrintRegularStats();
    }

    private static float stochasticRelaxationRadiosityElementImportance(StochasticRadiosityElement elem) {
        return elem.importance;
    }

    private static void stochasticRelaxationRadiosityElementUpdateImportance(StochasticRadiosityElement elem, double w) {
        if ( w < -1 ) {
            // Keep C++ signature.
        }
        double k = (double)StochasticRelaxation.activeState().prevImportanceTracedRays /
            (double)StochasticRelaxation.activeState().importanceTracedRays;

        elem.importance = (float)(
            k * (elem.importance - elem.sourceImportance) + (1.0 - k) * elem.receivedImportance + elem.sourceImportance);
        elem.unShotImportance = elem.receivedImportance = 0.0f;
    }

    private static void stochasticRelaxationRadiosityDoRegularImportanceIteration(
        VoxelGrid sceneWorldVoxelGrid,
        ArrayList<Patch> scenePatches,
        RenderOptions renderOptions)
    {
        long numberOfRays;
        int doHierarchicMeshing = ElementHierarchyState.activeState().do_h_meshing;
        HierarchyClusteringMode clustering = ElementHierarchyState.activeState().clustering;
        int weightedSampling = StochasticRelaxation.activeState().weightedSampling;
        ElementHierarchyState.activeState().do_h_meshing = 0;
        ElementHierarchyState.activeState().clustering = HierarchyClusteringMode.NO_CLUSTERING;
        StochasticRelaxation.activeState().weightedSampling = 0;

        numberOfRays = StochasticRelaxation.activeState().importanceRaysPerIteration;
        System.err.printf("Regular importance iteration %d:\n", StochasticRelaxation.activeState().currentIteration);

        StochasticJacobi.doStochasticJacobiIteration(
            sceneWorldVoxelGrid,
            numberOfRays,
            null,
            StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementImportance,
            StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementUpdateImportance,
            scenePatches,
            renderOptions);

        Mcrad.monteCarloRadiosityUpdateCpuSecs();
        stochasticRelaxationRadiosityPrintRegularStats();

        ElementHierarchyState.activeState().do_h_meshing = doHierarchicMeshing;
        ElementHierarchyState.activeState().clustering = clustering;
        StochasticRelaxation.activeState().weightedSampling = weightedSampling;
    }

    /**
Resets to zero all kind of things that should be reset to zero after a first
iteration of which the result only is to be used as the input of subsequent
iterations. Basically, everything that needs to be divided by the number of
rays except radiosity and importance needs to be reset to zero. This is
required for some of the experimental stuff to work
*/
    private static void stochasticRelaxationRadiosityElementDiscardIncremental(Element element) {
        StochasticRadiosityElement stochasticRadiosityElement = (StochasticRadiosityElement)element;

        if ( stochasticRadiosityElement == null ) {
            return;
        }

        stochasticRadiosityElement.quality = 0.0f;
        stochasticRadiosityElement.traverseAllChildren(StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementDiscardIncremental);
    }

    private static void stochasticRelaxationRadiosityDiscardIncremental() {
        StochasticRelaxation.activeState().tracedRays = 0;
        StochasticRelaxation.activeState().prevTracedRays = 0;

        stochasticRelaxationRadiosityElementDiscardIncremental(ElementHierarchyState.activeState().topCluster);
    }

    @Override
    public boolean doStep(Scene scene, RenderOptions renderOptions) {
        StochasticRelaxation.setActiveState(stochasticRelaxationState);
        ElementHierarchyState.setActiveState(elementHierarchyState);
        StochasticRadiosityBasisState.setActiveState(stochasticRadiosityBasisState);
        Mcrad.monteCarloRadiosityPreStep(scene, renderOptions);

        // Do some real work now
        if ( StochasticRelaxation.activeState().currentIteration == 1 ) {
            if ( StochasticRelaxation.activeState().doNonDiffuseFirstShot != 0 ) {
                Nondiff.doNonDiffuseFirstShot(scene, this, renderOptions);
            }
            long initialNrOfRays = StochasticRelaxation.activeState().tracedRays;

            if ( StochasticRelaxation.activeState().importanceDriven != 0 ) {
                if ( StochasticRelaxation.activeState().incrementalUsesImportance == 0 ) {
                    Logger.warning(null, "Importance is only used from the second iteration on ...");
                } else if ( StochasticRelaxation.activeState().importanceUpdated != 0 ) {
                    StochasticRelaxation.activeState().importanceUpdated = 0;

                    // Propagate importance changes
                    stochasticRelaxationRadiosityDoIncrementalImportanceIterations(
                        scene.voxelGrid, scene.patchList, renderOptions);
                    if ( StochasticRelaxation.activeState().importanceUpdatedFromScratch != 0 ) {
                        StochasticRelaxation.activeState().importanceRaysPerIteration =
                            StochasticRelaxation.activeState().importanceTracedRays;
                    }
                }
            }
            stochasticRelaxationRadiosityDoIncrementalRadianceIterations(scene, this, renderOptions);

            // Subsequent regular iterations will take as many rays as in the whole
            // sequence of incremental iteration steps
            StochasticRelaxation.activeState().raysPerIteration =
                StochasticRelaxation.activeState().tracedRays - initialNrOfRays;

            if ( StochasticRelaxation.activeState().discardIncremental != 0 ) {
                stochasticRelaxationRadiosityDiscardIncremental();
            }
        } else {
            if ( StochasticRelaxation.activeState().importanceDriven != 0 ) {
                if ( StochasticRelaxation.activeState().importanceUpdated != 0 ) {
                    StochasticRelaxation.activeState().importanceUpdated = 0;

                    // Propagate importance changes
                    stochasticRelaxationRadiosityDoIncrementalImportanceIterations(
                        scene.voxelGrid, scene.patchList, renderOptions);
                    if ( StochasticRelaxation.activeState().importanceUpdatedFromScratch != 0 ) {
                        StochasticRelaxation.activeState().importanceRaysPerIteration =
                            StochasticRelaxation.activeState().importanceTracedRays;
                    }
                } else {
                    stochasticRelaxationRadiosityDoRegularImportanceIteration(
                        scene.voxelGrid, scene.patchList, renderOptions);
                }
            }
            stochasticRelaxationRadiosityDoRegularRadianceIteration(scene.voxelGrid, scene.patchList, renderOptions);
        }

        stochasticRelaxationRadiosityRecomputeDisplayColors(scene.patchList);

        System.err.printf("%s\n", getStats());

        return false; // Always continue computing (never fully converged)
    }

    private static boolean repeatedDiscardIncrementalWarning = false;
}
