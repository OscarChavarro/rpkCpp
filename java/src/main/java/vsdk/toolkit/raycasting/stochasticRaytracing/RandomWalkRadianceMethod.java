package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Locale;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.RadianceMethodAlgorithm;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.Element;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class RandomWalkRadianceMethod extends RadianceMethod {
    private static final int STRING_LENGTH = 2000;
    private final StochasticRelaxation stochasticRelaxationState;
    private final ElementHierarchyState elementHierarchyState;
    private final StochasticRadiosityBasisState stochasticRadiosityBasisState;
    private static final ColorRgb[] selfEmittedRadiance = new ColorRgb[GalerkinBasis.MAX_BASIS_SIZE];

    static {
        for ( int i = 0; i < selfEmittedRadiance.length; i++ ) {
            selfEmittedRadiance[i] = new ColorRgb();
        }
    }

    private static void appendRandomWalkStatsText(StringBuilder buffer, int[] offset, String format, Object... args) {
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

    public RandomWalkRadianceMethod(
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
        className = RadianceMethodAlgorithm.RANDOM_WALK;
    }

    @Override
    public String getRadianceMethodName() {
        return "Random walk";
    }

    @Override
    public void parseOptions(int[] argc, String[] argv) {
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
            Error.fatal(-1, "RandomWalkRadianceMethod::initialize", "Tone mapping context not provided");
        }
        StochasticRelaxation.activeState().method = StochasticRaytracingMethod.RANDOM_WALK_RADIOSITY_METHOD;
        Mcrad.monteCarloRadiosityInit();
    }

    private static void randomWalkRadiosityPrintStats() {
        System.err.printf("%g secs., total radiance rays = %d",
            StochasticRelaxation.activeState().cpuSeconds, StochasticRelaxation.activeState().tracedRays);
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

    /**
Used as un-normalised stochasticJacobiProbability for mimicking global lines
*/
    private static double randomWalkRadiosityPatchArea(Patch patch) {
        return patch.area;
    }

    /**
stochasticJacobiProbability proportional to power to be propagated
*/
    private static double randomWalkRadiosityScalarSourcePower(Patch patch) {
        ColorRgb radiance = McradP.topLevelStochasticRadiosityElement(patch).sourceRad;
        return patch.area * radiance.sumAbsComponents();
    }

    /**
Returns a double instead of a float in order to make it useful as
a survival stochasticJacobiProbability function
*/
    private static double randomWalkRadiosityScalarReflectance(Patch patch) {
        return Mcrad.monteCarloRadiosityScalarReflectance(patch);
    }

    private static ColorRgb[] randomWalkRadiosityGetSelfEmittedRadiance(StochasticRadiosityElement elem) {
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(selfEmittedRadiance, elem.basis);
        ColorRgb ed = McradP.topLevelStochasticRadiosityElement(elem.patch).Ed; // Emittance
        selfEmittedRadiance[0].set(ed.r, ed.g, ed.b);
        return selfEmittedRadiance;
    }

    /**
Subtracts (1 - rho) * control radiosity from the source radiosity of each patch
*/
    private static void randomWalkRadiosityReduceSource(ArrayList<Patch> scenePatches) {
        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            ColorRgb newSourceRadiance = new ColorRgb();
            ColorRgb rho = McradP.topLevelStochasticRadiosityElement(patch).Rd;

            newSourceRadiance.setMonochrome(1.0f);
            newSourceRadiance.subtract(newSourceRadiance, rho); // 1 - rho
            newSourceRadiance.selfScalarProduct(StochasticRelaxation.activeState().controlRadiance); // (1-rho) * beta
            newSourceRadiance.subtract(McradP.topLevelStochasticRadiosityElement(patch).sourceRad, newSourceRadiance); // E - (1-rho) * beta
            McradP.topLevelStochasticRadiosityElement(patch).sourceRad.set(
                newSourceRadiance.r, newSourceRadiance.g, newSourceRadiance.b);
        }
    }

    private static double randomWalkRadiosityScoreWeight(Path path, int nodeIndex) {
        double w = 0.0;
        int t = path.numberOfNodes - ((StochasticRelaxation.activeState().randomWalkNumLast > 0)
            ? StochasticRelaxation.activeState().randomWalkNumLast : 1);

        switch ( StochasticRelaxation.activeState().randomWalkEstimatorKind ) {
            case RW_COLLISION:
                w = 1.0;
                break;
            case RW_ABSORPTION:
                if ( nodeIndex == path.numberOfNodes - 1 ) {
                    // Last node
                    w = 1.0 / (1.0 - path.nodes[nodeIndex].probability);
                }
                break;
            case RW_SURVIVAL:
                if ( nodeIndex < path.numberOfNodes - 1 ) {
                    // Not last node
                    w = 1.0 / path.nodes[nodeIndex].probability;
                }
                break;
            case RW_LAST_BUT_NTH:
                if ( nodeIndex == t - 1 ) {
                    int lastNodeIndex = path.numberOfNodes - 1;
                    w = 1.0 / (1.0 - path.nodes[lastNodeIndex].probability);
                    // Absorption prob of the last node
                    for ( int n = lastNodeIndex - 1; n >= nodeIndex; n-- ) {
                        // Survival prob of n...numberOfNodes-2th node
                        w /= path.nodes[n].probability;
                    }
                }
                break;
            case RW_N_LAST:
                if ( nodeIndex == t ) {
                    // 1 / absorption probability of the last path node
                    w = 1.0 / (1.0 - path.nodes[path.numberOfNodes - 1].probability);
                } else if ( nodeIndex > t ) {
                    w = 1.0;
                }
                break;
            default:
                Error.fatal(
                    -1, "randomWalkRadiosityScoreWeight", "Unknown random walk estimator kind %d",
                    StochasticRelaxation.activeState().randomWalkEstimatorKind.ordinal());
                break;
        }
        return w;
    }

    private static void randomWalkRadiosityShootingScore(
        Path path,
        long numberOfPaths,
        Tracepath.PatchProbabilityCallback birthProbability)
    {
        if ( birthProbability == null ) {
            // Keep C++ signature.
        }
        ColorRgb accumPow = new ColorRgb();
        StochasticRaytracingPathNode firstNode = path.nodes[0];

        // path->nodes[0].probability is birth probability of the path
        accumPow.scaledCopy(
            (float)(firstNode.patch.area / firstNode.probability),
            McradP.topLevelStochasticRadiosityElement(firstNode.patch).sourceRad);
        for ( int n = 1; n < path.numberOfNodes; n++ ) {
            StochasticRaytracingPathNode node = path.nodes[n];
            double[] uin = new double[] {0.0};
            double[] vin = new double[] {0.0};
            double[] uOut = new double[] {0.0};
            double[] vOut = new double[] {0.0};
            double r = 1.0;
            Patch patch = node.patch;
            ColorRgb Rd = McradP.topLevelStochasticRadiosityElement(patch).Rd;
            accumPow.scalarProduct(accumPow, Rd);

            patch.uniformUv(node.inPoint, uin, vin);
            if ( StochasticRelaxation.activeState().continuousRandomWalk == 0 ) {
                r = 0.0;
                if ( n < path.numberOfNodes - 1 ) {
                    // Not continuous random walk and not node of absorption
                    patch.uniformUv(node.outpoint, uOut, vOut);
                }
            }

            double w = randomWalkRadiosityScoreWeight(path, n);
            GalerkinBasis basis = McradP.getTopLevelPatchBasis(patch);
            for ( int i = 0; i < basis.size; i++ ) {
                double dual = basis.dualFunction[i].eval(uin[0], vin[0]) / patch.area;
                McradP.getTopLevelPatchReceivedRad(patch)[i].addScaled(
                    McradP.getTopLevelPatchReceivedRad(patch)[i],
                    (float)(w * dual / (double)numberOfPaths),
                    accumPow);

                if ( StochasticRelaxation.activeState().continuousRandomWalk == 0 ) {
                    double basf = basis.function[i].eval(uOut[0], vOut[0]);
                    r += dual * patch.area * basf;
                }
            }

            accumPow.scale((float)(r / node.probability));
        }
    }

    private static void randomWalkRadiosityShootingUpdate(Patch patch, double w) {
        double oldQuality = McradP.topLevelStochasticRadiosityElement(patch).quality;
        McradP.topLevelStochasticRadiosityElement(patch).quality += (float)w;
        if ( McradP.topLevelStochasticRadiosityElement(patch).quality < Numeric.EPSILON ) {
            return;
        }
        double k = oldQuality / McradP.topLevelStochasticRadiosityElement(patch).quality;

        // Subtract self-emitted rad
        McradP.getTopLevelPatchRad(patch)[0].subtract(
            McradP.getTopLevelPatchRad(patch)[0], McradP.topLevelStochasticRadiosityElement(patch).sourceRad);

        // Weight with previous results
        Coefficientsmcrad.stochasticRadiosityScaleCoefficients((float)k, McradP.getTopLevelPatchRad(patch), McradP.getTopLevelPatchBasis(patch));
        Coefficientsmcrad.stochasticRadiosityScaleCoefficients(
            (1.0f - (float)k), McradP.getTopLevelPatchReceivedRad(patch), McradP.getTopLevelPatchBasis(patch));
        Coefficientsmcrad.stochasticRadiosityAddCoefficients(
            McradP.getTopLevelPatchRad(patch), McradP.getTopLevelPatchReceivedRad(patch), McradP.getTopLevelPatchBasis(patch));

        // Re-add self-emitted rad
        McradP.getTopLevelPatchRad(patch)[0].add(
            McradP.getTopLevelPatchRad(patch)[0], McradP.topLevelStochasticRadiosityElement(patch).sourceRad);

        // Clear un-shot and received radiance
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchUnShotRad(patch), McradP.getTopLevelPatchBasis(patch));
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchReceivedRad(patch), McradP.getTopLevelPatchBasis(patch));
    }

    private static void randomWalkRadiosityDoShootingIteration(
        VoxelGrid sceneWorldVoxelGrid,
        ArrayList<Patch> scenePatches)
    {
        long numberOfWalks = StochasticRelaxation.activeState().initialNumberOfRays;
        if ( StochasticRelaxation.activeState().continuousRandomWalk != 0 ) {
            numberOfWalks *= StochasticRadiosityBasisState.activeState()
                .approxDesc[StochasticRelaxation.activeState().approximationOrderType.ordinal()].basis_size;
        } else {
            numberOfWalks *= (long)Math.pow(
                StochasticRadiosityBasisState.activeState()
                    .approxDesc[StochasticRelaxation.activeState().approximationOrderType.ordinal()].basis_size,
                1.0 / (1.0 - Statistics.instance().radiance.averageReflectivity.maximumComponent()));
        }

        System.err.printf(
            "Shooting iteration %d (%d paths, approximately %d rays)\n",
            StochasticRelaxation.activeState().currentIteration,
            numberOfWalks,
            (long)Math.floor((double)numberOfWalks / (1.0 - Statistics.instance().radiance.averageReflectivity.maximumComponent())));

        Tracepath.tracePaths(
            sceneWorldVoxelGrid,
            numberOfWalks,
            RandomWalkRadianceMethod::randomWalkRadiosityScalarSourcePower,
            RandomWalkRadianceMethod::randomWalkRadiosityScalarReflectance,
            RandomWalkRadianceMethod::randomWalkRadiosityShootingScore,
            RandomWalkRadianceMethod::randomWalkRadiosityShootingUpdate,
            scenePatches);
    }

    /**
Determines control radiosity value for collision gathering estimator
*/
    private static ColorRgb randomWalkRadiosityDetermineGatheringControlRadiosity(ArrayList<Patch> scenePatches) {
        ColorRgb c1 = new ColorRgb();
        ColorRgb c2 = new ColorRgb();
        ColorRgb cr = new ColorRgb();

        c1.clear();
        c2.clear();

        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            ColorRgb absorb = new ColorRgb();
            ColorRgb rho = McradP.topLevelStochasticRadiosityElement(patch).Rd;
            ColorRgb Ed = McradP.topLevelStochasticRadiosityElement(patch).sourceRad;
            ColorRgb num = new ColorRgb();
            ColorRgb denominator = new ColorRgb();

            absorb.setMonochrome(1.0f);
            absorb.subtract(absorb, rho); // 1-rho

            num.scalarProduct(absorb, Ed);
            c1.addScaled(c1, patch.area, num); // A_P (1-rho_P) E_P

            denominator.scalarProduct(absorb, absorb);
            c2.addScaled(c2, patch.area, denominator); // A_P (1-rho_P)^2
        }

        cr.divide(c1, c2);
        System.err.printf("Control radiosity value = ");
        cr.print(System.err);
        System.err.printf(", luminosity = %g\n", cr.luminance());

        return cr;
    }

    private static void randomWalkRadiosityCollisionGatheringScore(
        Path path,
        long numberOfPaths,
        Tracepath.PatchProbabilityCallback birthProbability)
    {
        if ( numberOfPaths < 0 || birthProbability == null ) {
            // Keep C++ signature.
        }
        ColorRgb accumRad;
        int lastNodeIndex = path.numberOfNodes - 1;
        accumRad = new ColorRgb(
            McradP.topLevelStochasticRadiosityElement(path.nodes[lastNodeIndex].patch).sourceRad.r,
            McradP.topLevelStochasticRadiosityElement(path.nodes[lastNodeIndex].patch).sourceRad.g,
            McradP.topLevelStochasticRadiosityElement(path.nodes[lastNodeIndex].patch).sourceRad.b);
        for ( int n = lastNodeIndex - 1; n >= 0; n-- ) {
            StochasticRaytracingPathNode node = path.nodes[n];
            double[] uin = new double[] {0.0};
            double[] vin = new double[] {0.0};
            double[] uOut = new double[] {0.0};
            double[] vOut = new double[] {0.0};
            double r = 1.0;
            Patch patch = node.patch;
            ColorRgb Rd = McradP.topLevelStochasticRadiosityElement(patch).Rd;
            accumRad.selfScalarProduct(Rd);

            patch.uniformUv(node.outpoint, uOut, vOut);
            if ( StochasticRelaxation.activeState().continuousRandomWalk == 0 ) {
                r = 0.0;
                if ( n > 0 ) {
                    // Not continuous random walk and not birth node
                    patch.uniformUv(node.inPoint, uin, vin);
                }
            }

            GalerkinBasis basis = McradP.getTopLevelPatchBasis(patch);
            for ( int i = 0; i < basis.size; i++ ) {
                double dual = basis.dualFunction[i].eval(uOut[0], vOut[0]); // = dual basis f * area
                McradP.getTopLevelPatchReceivedRad(patch)[i].addScaled(
                    McradP.getTopLevelPatchReceivedRad(patch)[i], (float)dual, accumRad);

                if ( StochasticRelaxation.activeState().continuousRandomWalk == 0 ) {
                    double basf = basis.function[i].eval(uin[0], vin[0]);
                    r += basf * dual;
                }
            }
            McradP.topLevelStochasticRadiosityElement(patch).ng++;

            accumRad.scale((float)(r / node.probability));
            accumRad.add(accumRad, McradP.topLevelStochasticRadiosityElement(patch).sourceRad);
        }
    }

    private static void randomWalkRadiosityGatheringUpdate(Patch patch, double w) {
        if ( w < -1 ) {
            // Keep C++ signature.
        }
        // Use un-shot rad for accumulating sum of contributions
        Coefficientsmcrad.stochasticRadiosityAddCoefficients(
            McradP.getTopLevelPatchUnShotRad(patch),
            McradP.getTopLevelPatchReceivedRad(patch),
            McradP.getTopLevelPatchBasis(patch));
        Coefficientsmcrad.stochasticRadiosityCopyCoefficients(
            McradP.getTopLevelPatchRad(patch),
            McradP.getTopLevelPatchUnShotRad(patch),
            McradP.getTopLevelPatchBasis(patch));

        // Divide by nr of samples
        if ( McradP.topLevelStochasticRadiosityElement(patch).ng > 0 ) {
            Coefficientsmcrad.stochasticRadiosityScaleCoefficients(
                (1.0f / McradP.topLevelStochasticRadiosityElement(patch).ng),
                McradP.getTopLevelPatchRad(patch),
                McradP.getTopLevelPatchBasis(patch));
        }

        // Add source radiance (source term estimation suppression!)
        McradP.getTopLevelPatchRad(patch)[0].add(
            McradP.getTopLevelPatchRad(patch)[0], McradP.topLevelStochasticRadiosityElement(patch).sourceRad);

        if ( StochasticRelaxation.activeState().constantControlVariate != 0 ) {
            // Add constant control radiosity value
            ColorRgb cr = new ColorRgb(
                StochasticRelaxation.activeState().controlRadiance.r,
                StochasticRelaxation.activeState().controlRadiance.g,
                StochasticRelaxation.activeState().controlRadiance.b);
            if ( StochasticRelaxation.activeState().indirectOnly != 0 ) {
                ColorRgb Rd = McradP.topLevelStochasticRadiosityElement(patch).Rd;
                cr.scalarProduct(Rd, StochasticRelaxation.activeState().controlRadiance);
            }
            McradP.getTopLevelPatchRad(patch)[0].add(McradP.getTopLevelPatchRad(patch)[0], cr);
        }

        Coefficientsmcrad.stochasticRadiosityClearCoefficients(
            McradP.getTopLevelPatchReceivedRad(patch), McradP.getTopLevelPatchBasis(patch));
    }

    private static void randomWalkRadiosityDoGatheringIteration(
        VoxelGrid sceneWorldVoxelGrid,
        ArrayList<Patch> scenePatches)
    {
        long numberOfWalks = StochasticRelaxation.activeState().initialNumberOfRays;
        if ( StochasticRelaxation.activeState().continuousRandomWalk != 0 ) {
            numberOfWalks *= StochasticRadiosityBasisState.activeState()
                .approxDesc[StochasticRelaxation.activeState().approximationOrderType.ordinal()].basis_size;
        } else {
            numberOfWalks *= (long)Math.pow(
                StochasticRadiosityBasisState.activeState()
                    .approxDesc[StochasticRelaxation.activeState().approximationOrderType.ordinal()].basis_size,
                1.0 / (1.0 - Statistics.instance().radiance.averageReflectivity.maximumComponent()));
        }

        if ( StochasticRelaxation.activeState().constantControlVariate != 0
            && StochasticRelaxation.activeState().currentIteration == 1 ) {
            // Constant control variate for gathering random walk radiosity
            StochasticRelaxation.activeState().controlRadiance =
                randomWalkRadiosityDetermineGatheringControlRadiosity(scenePatches);
            randomWalkRadiosityReduceSource(scenePatches); // Do this only once!
        }

        System.err.printf(
            "Collision gathering iteration %d (%d paths, approximately %d rays)\n",
            StochasticRelaxation.activeState().currentIteration,
            numberOfWalks,
            (long)Math.floor((double)numberOfWalks / (1.0 - Statistics.instance().radiance.averageReflectivity.maximumComponent())));

        Tracepath.tracePaths(
            sceneWorldVoxelGrid,
            numberOfWalks,
            RandomWalkRadianceMethod::randomWalkRadiosityPatchArea,
            RandomWalkRadianceMethod::randomWalkRadiosityScalarReflectance,
            RandomWalkRadianceMethod::randomWalkRadiosityCollisionGatheringScore,
            RandomWalkRadianceMethod::randomWalkRadiosityGatheringUpdate,
            scenePatches);
    }

    private static void randomWalkRadiosityUpdateSourceIllumination(StochasticRadiosityElement elem, double w) {
        if ( w < -1 ) {
            // Keep C++ signature.
        }
        Coefficientsmcrad.stochasticRadiosityCopyCoefficients(elem.radiance, elem.receivedRadiance, elem.basis);
        elem.sourceRad.set(elem.receivedRadiance[0].r, elem.receivedRadiance[0].g, elem.receivedRadiance[0].b);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
        Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
    }

    private static void randomWalkRadiosityDoFirstShot(
        VoxelGrid sceneWorldVoxelGrid,
        ArrayList<Patch> scenePatches,
        RenderOptions renderOptions)
    {
        long numberOfRays = StochasticRelaxation.activeState().initialNumberOfRays *
            StochasticRadiosityBasisState.activeState()
                .approxDesc[StochasticRelaxation.activeState().approximationOrderType.ordinal()].basis_size;
        System.err.printf("First shot (%d rays):\n", numberOfRays);
        StochasticJacobi.doStochasticJacobiIteration(
            sceneWorldVoxelGrid,
            numberOfRays,
            RandomWalkRadianceMethod::randomWalkRadiosityGetSelfEmittedRadiance,
            null,
            RandomWalkRadianceMethod::randomWalkRadiosityUpdateSourceIllumination,
            scenePatches,
            renderOptions);
        randomWalkRadiosityPrintStats();
    }

    @Override
    public void terminate(ArrayList<Patch> scenePatches) {
        StochasticRelaxation.setActiveState(stochasticRelaxationState);
        ElementHierarchyState.setActiveState(elementHierarchyState);
        StochasticRadiosityBasisState.setActiveState(stochasticRadiosityBasisState);
        Mcrad.monteCarloRadiosityTerminate(scenePatches);
    }

    @Override
    public boolean doStep(Scene scene, RenderOptions renderOptions) {
        StochasticRelaxation.setActiveState(stochasticRelaxationState);
        ElementHierarchyState.setActiveState(elementHierarchyState);
        StochasticRadiosityBasisState.setActiveState(stochasticRadiosityBasisState);
        Mcrad.monteCarloRadiosityPreStep(scene, renderOptions);

        if ( StochasticRelaxation.activeState().currentIteration == 1
            && StochasticRelaxation.activeState().indirectOnly != 0 ) {
            randomWalkRadiosityDoFirstShot(scene.voxelGrid, scene.patchList, renderOptions);
        }

        switch ( StochasticRelaxation.activeState().randomWalkEstimatorType ) {
            case RW_SHOOTING:
                randomWalkRadiosityDoShootingIteration(scene.voxelGrid, scene.patchList);
                break;
            case RW_GATHERING:
                randomWalkRadiosityDoGatheringIteration(scene.voxelGrid, scene.patchList);
                break;
            default:
                Error.fatal(-1, "randomWalkRadiosityDoStep", "Unknown random walk estimator type %d",
                    StochasticRelaxation.activeState().randomWalkEstimatorType.ordinal());
        }

        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            Mcrad.monteCarloRadiosityPatchComputeNewColor(scene.patchList.get(i));
        }

        return false; // Never converged
    }

    @Override
    public String getStats() {
        StochasticRelaxation.setActiveState(stochasticRelaxationState);
        ElementHierarchyState.setActiveState(elementHierarchyState);
        StochasticRadiosityBasisState.setActiveState(stochasticRadiosityBasisState);

        StringBuilder stats = new StringBuilder(STRING_LENGTH);
        int[] statsOffset = new int[] {0};

        appendRandomWalkStatsText(stats, statsOffset, "Random Walk Radiosity\nStatistics\n\n");
        appendRandomWalkStatsText(stats, statsOffset, "Iteration nr: %d\n", StochasticRelaxation.activeState().currentIteration);
        appendRandomWalkStatsText(stats, statsOffset, "CPU time: %g secs\n", StochasticRelaxation.activeState().cpuSeconds);
        appendRandomWalkStatsText(stats, statsOffset, "Radiance rays: %d\n", StochasticRelaxation.activeState().tracedRays);
        appendRandomWalkStatsText(stats, statsOffset, "Importance rays: %d\n", StochasticRelaxation.activeState().importanceTracedRays);

        return stats.toString();
    }
}
