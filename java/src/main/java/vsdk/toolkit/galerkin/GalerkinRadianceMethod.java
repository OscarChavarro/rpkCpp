/**
Galerkin radiosity, with the following variants:
- With or without hierarchical refinement
- With or without clusters
- With Jacobi, Gauss-Seidel or South well iterations
- With potential-driven or not
*/

package vsdk.toolkit.galerkin;

import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Locale;
import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.io.wrl.VrmlWriter;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.numericalAnalysis.PatchVisitor;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.RadianceMethodAlgorithm;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.Element;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class GalerkinRadianceMethod extends RadianceMethod {
    private static final int STRING_LENGTH = 2000;
    private static final int ALL_XXDF_COMPONENTS =
        XxdfComponentFlag.DIFFUSE_COMPONENT
            | XxdfComponentFlag.GLOSSY_COMPONENT
            | XxdfComponentFlag.SPECULAR_COMPONENT;
    private static final int ALL_BSDF_COMPONENTS =
        BsdfComponent.BRDF_DIFFUSE_COMPONENT
            | BsdfComponent.BRDF_GLOSSY_COMPONENT
            | BsdfComponent.BRDF_SPECULAR_COMPONENT
            | BsdfComponent.BTDF_DIFFUSE_COMPONENT
            | BsdfComponent.BTDF_GLOSSY_COMPONENT
            | BsdfComponent.BTDF_SPECULAR_COMPONENT;

    private Object gatheringStrategy;

    public static GalerkinState galerkinState = new GalerkinState();

    public static void freeMemory() {
        if ( GalerkinRadianceMethod.galerkinState.scratch != null ) {
            GalerkinRadianceMethod.galerkinState.scratch = null;
        }
    }

    public static void recomputePatchColor(Patch patch) {
        if ( patch == null || !(patch.radianceData instanceof GalerkinElement) ) {
            return;
        }

        GalerkinElement element = (GalerkinElement)patch.radianceData;
        ColorRgb reflectivity = element.Rd;
        ColorRgb radVis = new ColorRgb();

        // Compute the patches color based on its radiance + ambient radiance if desired
        if ( galerkinState.useAmbientRadiance != 0 ) {
            radVis.scalarProduct(reflectivity, galerkinState.ambientRadiance);
            radVis.add(radVis, element.radiance[0]);
            ToneMap.radianceToRgb(radVis, patch.color, galerkinState.toneMapOptions);
        }
        else {
            ToneMap.radianceToRgb(element.radiance[0], patch.color, galerkinState.toneMapOptions);
        }
        patch.computeVertexColors();
    }

    public GalerkinRadianceMethod() {
        className = RadianceMethodAlgorithm.GALERKIN;
        gatheringStrategy = null;
    }

    @Override
    public String getRadianceMethodName() {
        return "Galerkin";
    }

    public void setStrategy() {
        if ( galerkinState.clustered != 0 ) {
            gatheringStrategy = new Object();
        }
        else {
            gatheringStrategy = new Object();
        }
    }

    @Override
    public void parseOptions(int[] argc, String[] argv) {
        if ( argc != null || argv != null ) {
            // Nothing to parse in this migration step.
        }
    }

    private static void patchInit(Patch patch) {
        if ( patch == null || !(patch.radianceData instanceof GalerkinElement) ) {
            return;
        }

        GalerkinElement element = (GalerkinElement)patch.radianceData;
        ColorRgb reflectivity = element.Rd;
        ColorRgb selfEmittanceRadiance = element.Ed;

        if ( galerkinState.useConstantRadiance != 0 ) {
            // See Neumann et-al, "The Constant Radiosity Step", Euro-graphics Rendering Workshop
            // '95, Dublin, Ireland, June 1995, p 336-344
            element.radiance[0].scalarProduct(reflectivity, galerkinState.constantRadiance);
            element.radiance[0].add(element.radiance[0], selfEmittanceRadiance);
            if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
                element.unShotRadiance[0].subtract(element.radiance[0], galerkinState.constantRadiance);
            }
        }
        else {
            element.radiance[0].set(selfEmittanceRadiance.r, selfEmittanceRadiance.g, selfEmittanceRadiance.b);
            if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
                element.unShotRadiance[0].set(element.radiance[0].r, element.radiance[0].g, element.radiance[0].b);
            }
        }

        if ( galerkinState.importanceDriven != 0 ) {
            switch ( galerkinState.galerkinIterationMethod ) {
                case GAUSS_SEIDEL:
                case JACOBI:
                    element.potential = patch.directPotential;
                    break;
                case SOUTH_WELL:
                    element.potential = patch.directPotential;
                    element.unShotPotential = patch.directPotential;
                    break;
                default:
                    Error.fatal(-1, "patchInit", "Invalid iteration method");
            }
        }

        recomputePatchColor(patch);
    }

    @Override
    public void initialize(Scene scene, ToneMappingContext toneMapOptions) {
        galerkinState.toneMapOptions = toneMapOptions;
        if ( galerkinState.toneMapOptions == null ) {
            Error.fatal(-1, "GalerkinRadianceMethod::initialize", "Tone mapping context not provided");
        }

        galerkinState.iterationNumber = 0;
        galerkinState.cpuSeconds = 0.0f;

        GalerkinElement.initializeBasis();

        galerkinState.constantRadiance = Statistics.instance().radiance.estimatedAverageRadiance;
        if ( galerkinState.useConstantRadiance != 0 ) {
            galerkinState.ambientRadiance.clear();
        }
        else {
            galerkinState.ambientRadiance = Statistics.instance().radiance.estimatedAverageRadiance;
        }

        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            patchInit(scene.patchList.get(i));
        }

        galerkinState.topCluster = null;
        galerkinState.lastClusterId = -1;
        galerkinState.lastEye.set(Numeric.HUGE_FLOAT_VALUE, Numeric.HUGE_FLOAT_VALUE, Numeric.HUGE_FLOAT_VALUE);
    }

    @Override
    public boolean doStep(Scene scene, RenderOptions renderOptions) {
        if ( scene == null || renderOptions == null ) {
            // Parameters intentionally unused in this reduced Java migration.
        }

        if ( galerkinState.iterationNumber < 0 ) {
            Error.error("doGalerkinOneStep", "method not initialized");
            return true; // Done, don't continue!
        }

        galerkinState.iterationNumber++;

        return true;
    }

    /**
Disposes of the cluster hierarchy
*/
    private static void galerkinDestroyClusterHierarchy(GalerkinElement clusterElement) {
        if ( clusterElement == null || !clusterElement.isCluster() ) {
            return;
        }

        for ( int i = 0;
              clusterElement.irregularSubElements != null && i < clusterElement.irregularSubElements.size();
              i++ ) {
            Element child = clusterElement.irregularSubElements.get(i);
            if ( child instanceof GalerkinElement ) {
                galerkinDestroyClusterHierarchy((GalerkinElement)child);
            }
        }
    }

    @Override
    public void terminate(ArrayList<Patch> scenePatches) {
        if ( scenePatches != null ) {
            for ( int i = 0; i < scenePatches.size(); i++ ) {
                Patch patch = scenePatches.get(i);
                if ( patch != null ) {
                    recomputePatchColor(patch);
                }
            }
        }

        if ( galerkinState.topCluster != null ) {
            galerkinDestroyClusterHierarchy(galerkinState.topCluster);
            galerkinState.topCluster = null;
        }
        galerkinState.iterationNumber = -1;
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
        if ( camera != null || u != 0.0 || v != 0.0 || dir != null || renderOptions != null ) {
            // Parameters intentionally unused in this reduced Java migration.
        }

        if ( patch == null || !(patch.radianceData instanceof GalerkinElement) ) {
            ColorRgb black = new ColorRgb();
            black.clear();
            return black;
        }

        GalerkinElement element = (GalerkinElement)patch.radianceData;
        return element.radiance[0];
    }

    @Override
    public Element createPatchData(Patch patch) {
        GalerkinElement element = new GalerkinElement(patch, galerkinState);
        element.Ed = PatchVisitor.averageEmittance(patch, ALL_XXDF_COMPONENTS);
        element.Rd = PatchVisitor.averageNormalAlbedo(patch, ALL_BSDF_COMPONENTS);
        element.radiance[0].set(element.Ed.r, element.Ed.g, element.Ed.b);
        element.unShotRadiance[0].set(element.Ed.r, element.Ed.g, element.Ed.b);
        return element;
    }

    @Override
    public void destroyPatchData(Patch patch) {
        if ( patch != null ) {
            patch.radianceData = null;
        }
    }

    @Override
    public String getStats() {
        StringBuilder stats = new StringBuilder(STRING_LENGTH);
        stats.append("Galerkin\nStatistics\n\n");
        stats.append(String.format(Locale.US, "Iteration nr: %d\n", galerkinState.iterationNumber));
        stats.append(String.format(Locale.US, "CPU time: %g secs\n", galerkinState.cpuSeconds));
        stats.append(String.format(Locale.US, "Clustered: %d\n", galerkinState.clustered));
        stats.append(String.format(Locale.US, "Importance driven: %d\n", galerkinState.importanceDriven));
        return stats.toString();
    }

    @Override
    public void writeVRML(
        Camera camera,
        OutputStream outputStream,
        RenderOptions renderOptions)
    {
        if ( camera == null || outputStream == null || renderOptions == null ) {
            return;
        }

        VrmlWriter.writeHeader(camera, outputStream, renderOptions);
        VrmlWriter.writeTrailer(outputStream);
    }
}
