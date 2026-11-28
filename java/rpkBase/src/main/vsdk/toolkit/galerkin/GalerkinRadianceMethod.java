/**
Galerkin radiosity, with the following variants:
- With or without hierarchical refinement
- With or without clusters
- With Jacobi, Gauss-Seidel or South well iterations
- With potential-driven or not
*/

package vsdk.toolkit.galerkin;

import vsdk.toolkit.environment.geometry.elements.FaceColorTable;
import vsdk.toolkit.scene.DirectPotentialTable;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Locale;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.io.wrl.VrmlWriter;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.numericalAnalysis.PatchVisitor;
import vsdk.toolkit.io.PersistenceElement;
import vsdk.toolkit.galerkin.processing.ClusterCreationStrategy;
import vsdk.toolkit.galerkin.processing.GatheringClusteredStrategy;
import vsdk.toolkit.galerkin.processing.GatheringSimpleStrategy;
import vsdk.toolkit.galerkin.processing.GatheringStrategy;
import vsdk.toolkit.galerkin.processing.ScratchVisibilityStrategy;
import vsdk.toolkit.galerkin.processing.ShootingStrategy;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.RadianceMethodAlgorithm;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.Patch;
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

    private GatheringStrategy gatheringStrategy;
    private static OutputStream vrmlOutputStream = null;
    private static int numberOfWrites = 0;
    private static int vertexId = 0;

    public static GalerkinState galerkinState = new GalerkinState();

    public static void freeMemory() {
        ScratchVisibilityStrategy.scratchTerminate(GalerkinRadianceMethod.galerkinState);
    }

    /**
    Recomputes the color of a patch using ambient radiance term, ... if requested for
    */
    public static void recomputePatchColor(Patch patch, FaceColorTable faceColors) {
        GalerkinElement element = galerkinState.topLevelElement(patch);
        ColorRgb reflectivity = element.Rd;
        ColorRgb radVis = new ColorRgb();

        // Compute the patches color based on its radiance + ambient radiance if desired
        if ( galerkinState.useAmbientRadiance != 0 ) {
            radVis = reflectivity.multiply(galerkinState.ambientRadiance);
            radVis = radVis.add(element.radiance[0]);
            faceColors.set(patch.getFaceIndex(), ToneMap.radianceToRgb(radVis, galerkinState.toneMapOptions));
        }
        else {
            faceColors.set(patch.getFaceIndex(), ToneMap.radianceToRgb(element.radiance[0], galerkinState.toneMapOptions));
        }
        patch.computeVertexColors(faceColors);
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
        gatheringStrategy = null;
        if ( galerkinState.clustered != 0 ) {
            gatheringStrategy = new GatheringClusteredStrategy();
        }
        else {
            gatheringStrategy = new GatheringSimpleStrategy();
        }
    }

    @Override
    public void parseOptions(int[] argc, String[] argv) {
        if ( argc != null || argv != null ) {
            // Nothing to parse in this migration step.
        }
    }

    private static void patchInit(Patch patch, DirectPotentialTable directPotentials, FaceColorTable faceColors) {
        GalerkinElement element = galerkinState.topLevelElement(patch);
        ColorRgb reflectivity = element.Rd;
        ColorRgb selfEmittanceRadiance = element.Ed;

        if ( galerkinState.useConstantRadiance != 0 ) {
            // See Neumann et-al, "The Constant Radiosity Step", Euro-graphics Rendering Workshop
            // '95, Dublin, Ireland, June 1995, p 336-344
            element.radiance[0] = reflectivity.multiply(galerkinState.constantRadiance);
            element.radiance[0] = element.radiance[0].add(selfEmittanceRadiance);
            if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
                element.unShotRadiance[0] = element.radiance[0].subtract(galerkinState.constantRadiance);
            }
        }
        else {
            element.radiance[0] = new ColorRgb(selfEmittanceRadiance.getR(), selfEmittanceRadiance.getG(), selfEmittanceRadiance.getB());
            if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
                element.unShotRadiance[0] = new ColorRgb(element.radiance[0].getR(), element.radiance[0].getG(), element.radiance[0].getB());
            }
        }

        if ( galerkinState.importanceDriven != 0 ) {
            switch ( galerkinState.galerkinIterationMethod ) {
                case GAUSS_SEIDEL:
                case JACOBI:
                case PARALLEL_JACOBI:
                    element.potential = directPotentials.get(patch.getFaceIndex());
                    break;
                case SOUTH_WELL:
                    element.potential = directPotentials.get(patch.getFaceIndex());
                    element.unShotPotential = directPotentials.get(patch.getFaceIndex());
                    break;
                default:
                    Logger.reportMessage(null, Logger.FATAL_ERROR, "patchInit", "Invalid iteration method");
            }
        }

        recomputePatchColor(patch, faceColors);
    }

    @Override
    public void initialize(Scene scene, ToneMappingContext toneMapOptions) {
        galerkinState.setRadianceElements(scene.getRadianceElements());
        galerkinState.toneMapOptions = toneMapOptions;
        if ( galerkinState.toneMapOptions == null ) {
            Logger.reportMessage(this, Logger.FATAL_ERROR, "GalerkinRadianceMethod::initialize", "Tone mapping context not provided");
        }

        galerkinState.iterationNumber = 0;
        galerkinState.cpuSeconds = 0.0;

        GalerkinElement.initializeBasis();

        galerkinState.constantRadiance = Statistics.instance().radiance.estimatedAverageRadiance;
        if ( galerkinState.useConstantRadiance != 0 ) {
            galerkinState.ambientRadiance = new ColorRgb(0.0, 0.0, 0.0);
        }
        else {
            galerkinState.ambientRadiance = Statistics.instance().radiance.estimatedAverageRadiance;
        }

        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            patchInit(scene.patchList.get(i), scene.getDirectPotentials(), scene.getFaceColors());
        }

        galerkinState.topCluster = ClusterCreationStrategy.createClusterHierarchy(
            scene.clusteredRootGeometry,
            galerkinState);

        // Create a scratch software renderer for various operations on clusters
        if ( galerkinState.clusteringStrategy == GalerkinClusteringStrategy.Z_VISIBILITY ) {
            ScratchVisibilityStrategy.scratchInit(galerkinState);
        }
    }

    @Override
    public boolean doStep(Scene scene, RendererConfiguration renderOptions) {
        if ( galerkinState.iterationNumber < 0 ) {
            Logger.reportMessage(this, Logger.ERROR, "doGalerkinOneStep", "method not initialized");
            return true; // Done, don't continue!
        }

        galerkinState.iterationNumber++;
        galerkinState.lastClock = System.nanoTime();

        // And now the real work
        boolean done;

        switch ( galerkinState.galerkinIterationMethod ) {
            case JACOBI:
            case PARALLEL_JACOBI:
            case GAUSS_SEIDEL:
                if ( gatheringStrategy == null ) {
                    setStrategy();
                }
                done = gatheringStrategy.doGatheringIteration(scene, galerkinState, renderOptions);
                break;
            case SOUTH_WELL:
                done = ShootingStrategy.doShootingStep(scene, galerkinState, renderOptions);
                break;
            default:
                Logger.reportMessage(this, Logger.FATAL_ERROR, "doGalerkinOneStep", String.format("Invalid iteration method %s\n", galerkinState.galerkinIterationMethod));
                done = true;
                break;
        }

        updateCpuSecs();
        return done;
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

    private static int regularSubdivisionDepth(GalerkinElement element) {
        if ( element == null || element.regularSubElements == null ) {
            return 0;
        }
        int maxChildDepth = 0;
        for ( int i = 0; i < 4; i++ ) {
            if ( element.regularSubElements[i] instanceof GalerkinElement ) {
                int childDepth = regularSubdivisionDepth((GalerkinElement)element.regularSubElements[i]);
                if ( childDepth > maxChildDepth ) {
                    maxChildDepth = childDepth;
                }
            }
        }
        return 1 + maxChildDepth;
    }

    @Override
    public void terminate(ArrayList<Patch> scenePatches) {
        if ( galerkinState.clusteringStrategy == GalerkinClusteringStrategy.Z_VISIBILITY ) {
            ScratchVisibilityStrategy.scratchTerminate(galerkinState);
        }

        if ( galerkinState.topCluster != null ) {
            galerkinDestroyClusterHierarchy(galerkinState.topCluster);
            galerkinState.topCluster = null;
        }
        galerkinState.iterationNumber = -1;
    }

    private static void updateCpuSecs() {
        final long t = System.nanoTime();
        galerkinState.cpuSeconds += (double)((double)(t - galerkinState.lastClock) / 1000000000.0);
        galerkinState.lastClock = t;
    }

    private ColorRgb computePatchRadiance(Patch patch, double u, double v) {
        if ( patch == null ) {
            ColorRgb black = new ColorRgb();
            black = new ColorRgb(0.0, 0.0, 0.0);
            return black;
        }

        if ( patch.getFace().hasJacobian() ) {
            double[] uu = new double[] {u};
            double[] vv = new double[] {v};
            patch.biLinearToUniform(uu, vv);
            u = uu[0];
            v = vv[0];
        }

        GalerkinElement topLevelElement = GalerkinElement.fromPatch(galerkinState, patch);
        if ( topLevelElement == null ) {
            ColorRgb black = new ColorRgb();
            black = new ColorRgb(0.0, 0.0, 0.0);
            return black;
        }
        double[] uu = new double[] {u};
        double[] vv = new double[] {v};
        GalerkinElement leaf = topLevelElement.regularLeafAtPoint(uu, vv);
        ColorRgb rad = vsdk.toolkit.galerkin.GalerkinBasis.radianceAtPoint(leaf, leaf.radiance, uu[0], vv[0]);

        if ( galerkinState.useAmbientRadiance != 0 ) {
            // Add ambient radiance
            ColorRgb reflectivity = galerkinState.topLevelElement(patch).Rd;
            ColorRgb ambientRadiance = new ColorRgb();
            ambientRadiance = reflectivity.multiply(galerkinState.ambientRadiance);
            rad = rad.add(ambientRadiance);
        }

        return rad;
    }

    @Override
    public ColorRgb getRadiance(
        Camera camera,
        Patch patch,
        double u,
        double v,
        Vector3Dd dir,
        RendererConfiguration renderOptions)
    {
        if ( camera != null || dir != null || renderOptions != null ) {
            // Parameters intentionally unused in this method.
        }
        return computePatchRadiance(patch, u, v);
    }

    @Override
    public Element createPatchData(Patch patch) {
        return new GalerkinElement(patch, galerkinState);
    }

    @Override
    public void destroyPatchData(Patch patch) {
        // Java: the garbage collector disposes of the element once the table drops it
    }

    @Override
    public String getStats() {
        StringBuilder stats = new StringBuilder(STRING_LENGTH);
        stats.append("Galerkin Radiosity Statistics:\n\n");
        stats.append(String.format(Locale.US, "Iteration nr: %d\n", galerkinState.iterationNumber));
        stats.append(String.format(Locale.US, "Nr. elements: %d\n", GalerkinElement.getNumberOfElements()));
        stats.append(String.format(Locale.US, "clusters: %d\n", GalerkinElement.getNumberOfClusters()));
        stats.append(String.format(Locale.US, "surface elements: %d\n", GalerkinElement.getNumberOfSurfaceElements()));
        stats.append(String.format(Locale.US, "Nr. interactions: %d\n", Interaction.getNumberOfInteractions()));
        stats.append(String.format(Locale.US, "cluster to cluster: %d\n", Interaction.getNumberOfClusterToClusterInteractions()));
        stats.append(String.format(Locale.US, "cluster to surface: %d\n", Interaction.getNumberOfClusterToSurfaceInteractions()));
        stats.append(String.format(Locale.US, "surface to cluster: %d\n", Interaction.getNumberOfSurfaceToClusterInteractions()));
        stats.append(String.format(Locale.US, "surface to surface: %d\n", Interaction.getNumberOfSurfaceToSurfaceInteractions()));
        stats.append(String.format(Locale.US, "shadow hits: %d\n", Statistics.instance().shadow.numberOfShadowRays));
        stats.append(String.format(Locale.US, "shadow hits cached: %d\n", Statistics.instance().shadow.numberOfShadowCacheHits));
        stats.append(String.format(Locale.US, "CPU time: %g secs\n", galerkinState.cpuSeconds));
        stats.append(String.format(Locale.US, "Clustered: %d\n", galerkinState.clustered));
        stats.append(String.format(Locale.US, "Importance driven: %d\n", galerkinState.importanceDriven));
        return stats.toString();
    }

    @Override
    public void writeVRML(
        Camera camera,
        OutputStream outputStream,
        RendererConfiguration renderOptions)
    {
        if ( camera == null || outputStream == null || renderOptions == null ) {
            return;
        }

        VrmlWriter.writeHeader(camera, outputStream, renderOptions);
        vrmlOutputStream = outputStream;
        writeCoords();
        writeColors(renderOptions);
        writeCoordIndicesTopCluster();
        VrmlWriter.writeTrailer(outputStream);
    }

    private static void writeFormatted(String format, Object... arguments) {
        if ( vrmlOutputStream == null || format == null ) {
            return;
        }
        String text;
        try {
            text = String.format(Locale.US, format, arguments);
        }
        catch ( Exception ignored ) {
            text = "";
        }
        if ( text.isEmpty() ) {
            return;
        }
        byte[] bytes = text.getBytes(StandardCharsets.UTF_8);
        PersistenceElement.writeBytes(vrmlOutputStream, bytes, bytes.length);
    }

    private static void writeVertexCoord(Vector3Dd p) {
        if ( p == null ) {
            return;
        }
        if ( numberOfWrites > 0 ) {
            writeFormatted("%s", ", ");
        }
        numberOfWrites++;
        if ( numberOfWrites % 4 == 0 ) {
            writeFormatted("%s", "\n\t  ");
        }
        writeFormatted("%g %g %g", p.x(), p.y(), p.z());
        vertexId++;
    }

    private static void writeVertexCoords(Element element) {
        if ( !(element instanceof GalerkinElement) ) {
            return;
        }
        GalerkinElement galerkinElement = (GalerkinElement)element;
        Vector3Dd[] v = new Vector3Dd[] {
            new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd(),
            new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd()
        };
        int numberOfVertices = galerkinElement.vertices(v);
        for ( int i = 0; i < numberOfVertices; i++ ) {
            writeVertexCoord(v[i]);
        }
    }

    private static void writeCoords() {
        if ( galerkinState.topCluster == null ) {
            return;
        }
        numberOfWrites = 0;
        vertexId = 0;
        writeFormatted("%s", "\tcoord Coordinate {\n\t  point [ ");
        galerkinState.topCluster.traverseAllLeafElements(GalerkinRadianceMethod::writeVertexCoords);
        writeFormatted("%s", " ] ");
        writeFormatted("%s", "\n\t}\n");
    }

    private static void writeVertexColor(ColorRgb color) {
        if ( color == null ) {
            return;
        }
        if ( numberOfWrites > 0 ) {
            writeFormatted("%s", ", ");
        }
        numberOfWrites++;
        if ( numberOfWrites % 4 == 0 ) {
            writeFormatted("%s", "\n\t  ");
        }
        writeFormatted("%.3g %.3g %.3g", color.getR(), color.getG(), color.getB());
        vertexId++;
    }

    private static void writeVertexColors(Element element) {
        if ( !(element instanceof GalerkinElement) ) {
            return;
        }
        GalerkinElement galerkinElement = (GalerkinElement)element;
        if ( galerkinElement.patch == null ) {
            return;
        }

        ColorRgb[] vertexRadiosity = new ColorRgb[] {
            new ColorRgb(), new ColorRgb(), new ColorRgb(), new ColorRgb()
        };
        int numberOfVertices = galerkinElement.patch.getNumberOfVertices();
        if ( numberOfVertices == 3 ) {
            vertexRadiosity[0] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 0.0, 0.0);
            vertexRadiosity[1] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 1.0, 0.0);
            vertexRadiosity[2] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 0.0, 1.0);
        }
        else {
            vertexRadiosity[0] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 0.0, 0.0);
            vertexRadiosity[1] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 1.0, 0.0);
            vertexRadiosity[2] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 1.0, 1.0);
            vertexRadiosity[3] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 0.0, 1.0);
        }

        if ( galerkinState.useAmbientRadiance != 0 ) {
            ColorRgb reflectivity = galerkinElement.galerkinState.topLevelElement(galerkinElement.patch).Rd;
            ColorRgb ambient = new ColorRgb();
            ambient = reflectivity.multiply(galerkinState.ambientRadiance);
            for ( int i = 0; i < numberOfVertices; i++ ) {
                vertexRadiosity[i] = vertexRadiosity[i].add(ambient);
            }
        }

        for ( int i = 0; i < numberOfVertices; i++ ) {
            ColorRgb col;
            col = ToneMap.radianceToRgb(vertexRadiosity[i], galerkinState.toneMapOptions);
            writeVertexColor(col);
        }
    }

    private static void writeVertexColorsTopCluster() {
        if ( galerkinState.topCluster == null ) {
            return;
        }
        vertexId = 0;
        numberOfWrites = 0;
        writeFormatted("%s", "\tcolor Color {\n\t  color [ ");
        galerkinState.topCluster.traverseAllLeafElements(GalerkinRadianceMethod::writeVertexColors);
        writeFormatted("%s", " ] ");
        writeFormatted("%s", "\n\t}\n");
    }

    private static void writeColors(RendererConfiguration renderOptions) {
        if ( renderOptions == null ) {
            return;
        }
        if ( !renderOptions.smoothShading ) {
            Logger.reportMessage(null, Logger.WARNING, "", "I assume you want a smooth shaded model ...");
        }
        writeFormatted("\tcolorPerVertex %s\n", "TRUE");
        writeVertexColorsTopCluster();
    }

    private static void writeCoordIndex(int index) {
        numberOfWrites++;
        if ( numberOfWrites % 20 == 0 ) {
            writeFormatted("%s", "\n\t  ");
        }
        writeFormatted("%d ", index);
    }

    private static void writeCoordIndices(Element element) {
        if ( !(element instanceof GalerkinElement) ) {
            return;
        }
        GalerkinElement galerkinElement = (GalerkinElement)element;
        if ( galerkinElement.patch == null ) {
            return;
        }
        for ( int i = 0; i < galerkinElement.patch.getNumberOfVertices(); i++ ) {
            writeCoordIndex(vertexId);
            vertexId++;
        }
        writeCoordIndex(-1);
    }

    private static void writeCoordIndicesTopCluster() {
        if ( galerkinState.topCluster == null ) {
            return;
        }
        vertexId = 0;
        numberOfWrites = 0;
        writeFormatted("%s", "\tcoordIndex [ ");
        galerkinState.topCluster.traverseAllLeafElements(GalerkinRadianceMethod::writeCoordIndices);
        writeFormatted("%s", " ]\n");
    }
}
