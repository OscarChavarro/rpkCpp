package vsdk.toolkit.app;

import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.Locale;
import vsdk.toolkit.app.options.BatchOptions;
import vsdk.toolkit.app.options.OptionsGroupCore;
import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.io.bin.reader.BinaryModelDeserializer;
import vsdk.toolkit.io.bin.writer.BinaryModelSerializer;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.ParseSnapshotContext;
import vsdk.toolkit.io.mgf.MgfParserLoader;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.numericalAnalysis.MeshSurfaceVisitor;
import vsdk.toolkit.numericalAnalysis.PatchVisitor;
import vsdk.toolkit.render.RenderHookList;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.PatchClusterOctreeNode;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.skin.GeometryClassId;
import vsdk.toolkit.skin.MeshSurface;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public final class SceneBuilder {
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

    private SceneBuilder() {
    }

    private static void sceneBuilderPatchAccumulateStats(Patch patch) {
        ColorRgb E = PatchVisitor.averageEmittance(patch, ALL_XXDF_COMPONENTS);
        ColorRgb R = PatchVisitor.averageNormalAlbedo(patch, ALL_BSDF_COMPONENTS);
        ColorRgb power = new ColorRgb();

        Statistics.instance().radiance.totalArea += patch.getArea();
        power = E.multiply(patch.getArea());
        Statistics.instance().radiance.totalEmittedPower = (Statistics.instance().radiance.totalEmittedPower).add(power);
        Statistics.instance().radiance.averageReflectivity = (Statistics.instance().radiance.averageReflectivity).addScaled(patch.getArea(), R);
        E = E.multiply(1.0 / (double)Math.PI);
        Statistics.instance().radiance.maxSelfEmittedRadiance = E.maximum(Statistics.instance().radiance.maxSelfEmittedRadiance);
        Statistics.instance().radiance.maxSelfEmittedPower = power.maximum(Statistics.instance().radiance.maxSelfEmittedPower);
    }

    private static void sceneBuilderComputeStats(Scene scene) {
        Vector3Dd zero = new Vector3Dd();
        ColorRgb one = new ColorRgb();
        ColorRgb averageAbsorption = new ColorRgb();
        ColorRgb BP;

        one = new ColorRgb(1.0, 1.0, 1.0);
        zero = new Vector3Dd(0, 0, 0);

        // Initialize
        Statistics.instance().radiance.totalEmittedPower = new ColorRgb(0.0, 0.0, 0.0);
        Statistics.instance().radiance.averageReflectivity = new ColorRgb(0.0, 0.0, 0.0);
        Statistics.instance().radiance.maxSelfEmittedRadiance = new ColorRgb(0.0, 0.0, 0.0);
        Statistics.instance().radiance.maxSelfEmittedPower = new ColorRgb(0.0, 0.0, 0.0);
        Statistics.instance().radiance.totalArea = 0.0;

        // Accumulate
        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            SceneBuilder.sceneBuilderPatchAccumulateStats(scene.patchList.get(i));
        }

        // Averages
        Statistics.instance().radiance.averageReflectivity = (Statistics.instance().radiance.averageReflectivity).scaleInverse(Statistics.instance().radiance.totalArea);
        averageAbsorption = one.subtract(Statistics.instance().radiance.averageReflectivity);
        Statistics.instance().radiance.estimatedAverageRadiance = (Statistics.instance().radiance.totalEmittedPower).scaleInverse((double)Math.PI * Statistics.instance().radiance.totalArea);

        // Include background radiation
        BP = Background.backgroundPower(scene.background, zero);
        BP = BP.multiply(1.0 / (4.0 * (double)Math.PI));
        Statistics.instance().radiance.totalEmittedPower = (Statistics.instance().radiance.totalEmittedPower).add(BP);
        Statistics.instance().radiance.estimatedAverageRadiance = (Statistics.instance().radiance.estimatedAverageRadiance).add(BP);
        Statistics.instance().radiance.estimatedAverageRadiance = (Statistics.instance().radiance.estimatedAverageRadiance).divide(averageAbsorption);

        Statistics.instance().potential.totalDirectPotential = 0.0;
        Statistics.instance().potential.maxDirectPotential = 0.0;
        Statistics.instance().potential.averageDirectPotential = 0.0;
        Statistics.instance().potential.maxDirectImportance = 0.0;
    }

    /**
Adds the background to the global light source patch list
*/
    private static void sceneBuilderAddBackgroundToLightSourceList(Scene scene) {
        if ( scene.background != null && scene.background.bkgPatch != null ) {
            scene.lightSourcePatchList.add(scene.background.bkgPatch);
            Statistics.instance().reader.numberOfLightSources++;
        }
    }

    /**
Adds the patch to the global light source patch list if the patch is on
a light source (i.e. when the surfaces material has a non-null edf)
*/
    private static void sceneBuilderAddPatchToLightSourceListIfLightSource(ArrayList<Patch> lights, Patch patch) {
        if ( patch != null
             && patch.getMaterial() != null
             && patch.getMaterial().getEdf() != null ) {
            lights.add(patch);
            Statistics.instance().reader.numberOfLightSources++;
        }
    }

    /**
Build the global light source patch list
*/
    private static void sceneBuilderFillLightSourcePatchList(Scene scene) {
        ArrayList<Patch> lights = new ArrayList<>();
        Statistics.instance().reader.numberOfLightSources = 0;

        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            SceneBuilder.sceneBuilderAddPatchToLightSourceListIfLightSource(lights, scene.patchList.get(i));
        }

        SceneBuilder.sceneBuilderAddBackgroundToLightSourceList(scene);
        scene.lightSourcePatchList = lights;
    }

    /**
Creates a hierarchical model of the discrete scene (the patches in the scene) using the simple
algorithm described in
- Per Christensen, "Hierarchical Techniques for Glossy Global Illumination",
  PhD Thesis, University of Washington, 1995, p 116
This hierarchy is often much more efficient for tracing rays and clustering radiosity algorithms
than the given hierarchy of bounding boxes. A pointer to the toplevel "cluster" is returned
*/
    private static RpkGeometry sceneBuilderCreateClusterHierarchy(ArrayList<Patch> patches) {
        PatchClusterOctreeNode rootCluster;
        RpkGeometry rootGeometry;

        // Create a toplevel cluster containing (references to) all the patches in the scene
        rootCluster = new PatchClusterOctreeNode(patches);

        // Split the toplevel cluster recursively into sub-clusters
        rootCluster.splitCluster();
        //rootCluster->print(0);

        // Convert to a geometry hierarchy, disposing of clusters
        rootGeometry = rootCluster.convertClusterToGeometry();

        rootCluster.destroy();
        return rootGeometry;
    }

    /**
Builds a linear list of patches making up all the geometries in the list, whether
they are primitive or not
*/
    private static void sceneBuilderPatchList(ArrayList<RpkGeometry> geometryList, ArrayList<Patch> patchList) {
        for ( int i = 0; geometryList != null && i < geometryList.size(); i++ ) {
            RpkGeometry geometry = geometryList.get(i);
            if ( geometry.isCompound() ) {
                // Recursive case
                Compound compound = (Compound)geometry;
                SceneBuilder.sceneBuilderPatchList(compound.children, patchList);
            }
            else {
                // Trivial case
                ArrayList<Patch> patchesFromNonCompounds = RpkGeometry.patchListReference(geometry);

                for ( int j = 0; patchesFromNonCompounds != null && j < patchesFromNonCompounds.size(); j++ ) {
                    Patch patch = patchesFromNonCompounds.get(j);
                    if ( patch != null ) {
                        patchList.add(patch);
                    }
                }
            }
        }
    }

    private static void sceneBuilderFillFacesBackPointers(ArrayList<RpkGeometry> geometryList) {
        if ( geometryList == null ) {
            return;
        }
        for ( int i = 0; i < geometryList.size(); i++ ) {
            RpkGeometry geometry = geometryList.get(i);
            if ( geometry == null ) {
                continue;
            }
            if ( geometry.isCompound() ) {
                Compound compound = (Compound)geometry;
                SceneBuilder.sceneBuilderFillFacesBackPointers(compound.children);
                continue;
            }
            if ( geometry.className == GeometryClassId.SURFACE_MESH ) {
                MeshSurfaceVisitor.fillFacesBackPointers((MeshSurface)geometry);
            }
        }
    }

    private static void sceneBuilderCollectGeometriesRecursive(
        ArrayList<RpkGeometry> source,
        ArrayList<RpkGeometry> target)
    {
        if ( source == null || target == null ) {
            return;
        }

        for ( int i = 0; i < source.size(); i++ ) {
            RpkGeometry geometry = source.get(i);
            if ( geometry == null ) {
                continue;
            }

            boolean alreadyInTarget = false;
            for ( int j = 0; j < target.size(); j++ ) {
                if ( target.get(j) == geometry ) {
                    alreadyInTarget = true;
                    break;
                }
            }
            if ( !alreadyInTarget ) {
                target.add(geometry);
            }

            if ( geometry.isCompound() ) {
                Compound compound = (Compound)geometry;
                SceneBuilder.sceneBuilderCollectGeometriesRecursive(compound.children, target);
            }
        }
    }

    private static void sceneBuilderApplyModelToMgfContext(ParseRuntimeContext mgfContext, ParseSnapshotContext mgfModel) {
        if ( mgfContext == null || mgfModel == null ) {
            return;
        }

        mgfContext.currentColor = mgfModel.currentColor;
        mgfContext.currentFaceList = mgfModel.currentFaceList;
        mgfContext.currentGeometryList = mgfModel.currentGeometryList;
        mgfContext.currentMaterialName = mgfModel.currentMaterialName;
        mgfContext.currentNormalList = mgfModel.currentNormalList;
        mgfContext.currentObjectName = mgfModel.currentObjectName;
        mgfContext.currentPointList = mgfModel.currentPointList;
        mgfContext.currentVertexList = mgfModel.currentVertexList;
        mgfContext.currentVertexName = mgfModel.currentVertexName;
        mgfContext.geometries = mgfModel.geometries;
        mgfContext.geometryStackHeadIndex = mgfModel.geometryStackHeadIndex;
        mgfContext.inComplex = mgfModel.inComplex;
        mgfContext.inSurface = mgfModel.inSurface;
        mgfContext.materials = mgfModel.materials;
        mgfContext.monochrome = mgfModel.monochrome;
        mgfContext.singleSided = mgfModel.singleSided;
        mgfContext.numberOfQuarterCircleDivisions = mgfModel.numberOfQuarterCircleDivisions;
        mgfContext.readerContext = mgfModel.readerContext;
        mgfContext.transformContext = mgfModel.transformContext;
        mgfContext.model = mgfModel;

        mgfContext.colorRepository.currentColor = mgfContext.currentColor;
        mgfContext.geometryBuildState.currentFaceList = mgfContext.currentFaceList;
        mgfContext.geometryBuildState.currentGeometryList = mgfContext.currentGeometryList;
        mgfContext.materialState.currentMaterialName = mgfContext.currentMaterialName;
        mgfContext.geometryBuildState.currentNormalList = mgfContext.currentNormalList;
        mgfContext.geometryBuildState.currentObjectName = mgfContext.currentObjectName;
        mgfContext.geometryBuildState.currentPointList = mgfContext.currentPointList;
        mgfContext.geometryBuildState.currentVertexList = mgfContext.currentVertexList;
        mgfContext.geometryBuildState.currentVertexName = mgfContext.currentVertexName;
        mgfContext.geometryBuildState.geometries = mgfContext.geometries;
        mgfContext.geometryBuildState.geometryStackHeadIndex = mgfContext.geometryStackHeadIndex;
        mgfContext.geometryBuildState.inComplex = mgfContext.inComplex;
        mgfContext.geometryBuildState.inSurface = mgfContext.inSurface;
        mgfContext.materialState.materials = mgfContext.materials;
        mgfContext.parserConfig.monochrome = mgfContext.monochrome;
        mgfContext.parserConfig.singleSided = mgfContext.singleSided;
        mgfContext.parserConfig.numberOfQuarterCircleDivisions = mgfContext.numberOfQuarterCircleDivisions;
        mgfContext.readerStackState.readerContext = mgfContext.readerContext;
        mgfContext.transformStack.transformContext = mgfContext.transformContext;

        mgfContext.currentMaterial = null;
        if ( mgfContext.materials != null && mgfContext.currentMaterialName != null ) {
            for ( int i = 0; i < mgfContext.materials.size(); i++ ) {
                RpkMaterial material = mgfContext.materials.get(i);
                if ( material != null
                     && material.getName() != null
                     && material.getName().equals(mgfContext.currentMaterialName) ) {
                    mgfContext.currentMaterial = material;
                    break;
                }
            }
        }
        mgfContext.materialState.currentMaterial = mgfContext.currentMaterial;

        if ( mgfContext.allGeometries != null ) {
            mgfContext.allGeometries.clear();
        }
        mgfContext.allGeometries = new ArrayList<>();
        mgfContext.geometryBuildState.allGeometries = mgfContext.allGeometries;
        SceneBuilder.sceneBuilderCollectGeometriesRecursive(mgfModel.currentGeometryList, mgfContext.allGeometries);
        if ( mgfModel.geometries != mgfModel.currentGeometryList ) {
            SceneBuilder.sceneBuilderCollectGeometriesRecursive(mgfModel.geometries, mgfContext.allGeometries);
        }
    }

    private static void removeEmptyMeshSurfaces(ParseRuntimeContext mgfContext, ArrayList<RpkGeometry> geometryList) {
        if ( geometryList == null ) {
            return;
        }

        for ( int i = 0; i < geometryList.size(); i++ ) {
            RpkGeometry geometry = geometryList.get(i);
            if ( geometry.className == GeometryClassId.SURFACE_MESH ) {
                MeshSurface mesh = (MeshSurface)geometry;
                if ( mesh.vertices != null && mesh.vertices.size() == 0 ) {
                    if ( mgfContext.allGeometries != null ) {
                        for ( int j = 0; j < mgfContext.allGeometries.size(); j++ ) {
                            RpkGeometry deletionCandidate = mgfContext.allGeometries.get(j);
                            if ( deletionCandidate == geometry ) {
                                mgfContext.allGeometries.remove(j);
                                break;
                            }
                        }
                    }
                    RpkGeometry.destroy(geometry);
                    geometryList.remove(i);
                    i--;
                }
            }
        }
    }

    private static boolean sceneBuilderHasExtension(String fileName, String extension) {
        if ( fileName == null || extension == null ) {
            return false;
        }

        int fileNameLength = fileName.length();
        int extensionLength = extension.length();
        if ( fileNameLength < extensionLength ) {
            return false;
        }

        return fileName.regionMatches(true, fileNameLength - extensionLength, extension, 0, extensionLength);
    }

    private static String sceneBuilderBuildBinaryFallbackPath(String mgfFileName) {
        if ( !sceneBuilderHasExtension(mgfFileName, ".mgf") ) {
            return null;
        }

        return mgfFileName.substring(0, mgfFileName.length() - 4) + ".bin";
    }

    private static boolean sceneBuilderIsReadableRegularFile(String fileName) {
        if ( fileName == null || fileName.isEmpty() ) {
            return false;
        }

        File file = new File(fileName);
        if ( !file.exists() || !file.isFile() || !file.canRead() ) {
            return false;
        }

        try (FileInputStream input = new FileInputStream(fileName)) {
            int firstByte = input.read();
            return firstByte >= 0;
        }
        catch ( Exception ignored ) {
            return false;
        }
    }

    private static boolean sceneBuilderValidateReadableFile(
        String fileName,
        String fileRole)
    {
        File file = new File(fileName);
        if ( !file.exists() ) {
            Logger.reportMessage(null, Logger.ERROR, "SceneBuilder::sceneBuilderReadFile", String.format("Requested %s file '%s' does not exist", fileRole, fileName));
            return false;
        }
        if ( !file.isFile() ) {
            Logger.reportMessage(null, Logger.ERROR, "SceneBuilder::sceneBuilderReadFile", String.format("Requested %s file '%s' is not a regular file", fileRole, fileName));
            return false;
        }
        if ( !file.canRead() ) {
            Logger.reportMessage(null, Logger.ERROR, "SceneBuilder::sceneBuilderReadFile", String.format("Requested %s file '%s' is not readable", fileRole, fileName));
            return false;
        }

        try (FileInputStream input = new FileInputStream(fileName)) {
            int firstByte = input.read();
            if ( firstByte < 0 ) {
                Logger.reportMessage(null, Logger.ERROR, "SceneBuilder::sceneBuilderReadFile", String.format("Requested %s file '%s' is empty", fileRole, fileName));
                return false;
            }
        }
        catch ( Exception ignored ) {
            Logger.reportMessage(null, Logger.ERROR, "SceneBuilder::sceneBuilderReadFile", String.format("Requested %s file '%s' is not readable", fileRole, fileName));
            return false;
        }

        return true;
    }

    /**
Tries to read the scene in the given file. Returns false if not successful.
Returns true if successful
*/
    private static boolean sceneBuilderReadFile(
        String fileName,
        ParseRuntimeContext mgfContext,
        Scene scene,
        ToneMappingContext toneMapOptions)
    {
        BatchOptions batchOptions = Batch.batchGetOptions();
        boolean importBinaryOption =
            batchOptions != null
                && batchOptions.importBinary
                && batchOptions.binaryInputFilename != null
                && !batchOptions.binaryInputFilename.isEmpty();
        String requestedInputName = importBinaryOption ? batchOptions.binaryInputFilename : fileName;

        boolean readBinaryModel =
            importBinaryOption
                || SceneBuilder.sceneBuilderHasExtension(requestedInputName, ".bin");

        String fallbackBinaryInputName = null;
        String inputName = requestedInputName;

        if ( !readBinaryModel
             && SceneBuilder.sceneBuilderHasExtension(requestedInputName, ".mgf")
             && !SceneBuilder.sceneBuilderIsReadableRegularFile(requestedInputName) ) {
            fallbackBinaryInputName = SceneBuilder.sceneBuilderBuildBinaryFallbackPath(requestedInputName);
            if ( fallbackBinaryInputName != null
                 && SceneBuilder.sceneBuilderIsReadableRegularFile(fallbackBinaryInputName) ) {
                readBinaryModel = true;
                inputName = fallbackBinaryInputName;
            }
        }

        // Check whether the file can be opened/read
        if ( readBinaryModel && !SceneBuilder.sceneBuilderValidateReadableFile(inputName, "binary model") ) {
            return false;
        }

        if ( !readBinaryModel && fileName != null && !fileName.isEmpty() && fileName.charAt(0) != '#' ) {
            if ( !SceneBuilder.sceneBuilderValidateReadableFile(fileName, "scene") ) {
                return false;
            }
        }

        // Get current directory from the filename
        String currentDirectory = "";
        if ( inputName != null ) {
            int slash = inputName.lastIndexOf('/');
            if ( slash >= 0 ) {
                currentDirectory = inputName.substring(0, slash);
            }
        }
        if ( !currentDirectory.isEmpty() ) {
            // Kept for parity with C++ flow; currently unused.
        }

        // Prepare if errors occur when reading the new scene will abort
        scene.geometryList = null;

        Patch.setNextId(1);
        scene.background = OptionsGroupCore.createBackground();

        // Read the source scene description into a ParseSnapshotContext snapshot
        System.err.printf("Reading the scene from file '%s' ... \n", inputName);
        long last = System.nanoTime();
        ParseSnapshotContext mgfModel = null;

        if ( readBinaryModel ) {
            mgfModel = BinaryModelDeserializer.read(inputName);
            if ( mgfModel != null ) {
                SceneBuilder.sceneBuilderApplyModelToMgfContext(mgfContext, mgfModel);
            }
        }
        else {
            mgfModel = MgfParserLoader.readMgf(fileName, mgfContext);
            if ( mgfModel != null
                 && batchOptions != null
                 && batchOptions.exportBinary
                 && batchOptions.binaryOutputFilename != null
                 && !batchOptions.binaryOutputFilename.isEmpty() ) {
                System.err.printf( "Exporting loaded ParseSnapshotContext to binary '%s' ... ", batchOptions.binaryOutputFilename);
                System.err.flush();
                boolean binarySaved = BinaryModelSerializer.write(
                    mgfModel,
                    batchOptions.binaryOutputFilename);
                if ( binarySaved ) {
                    System.err.printf("done.\n");
                }
                else {
                    System.err.printf("failed.\n");
                    Logger.reportMessage(null, Logger.ERROR, "SceneBuilder::sceneBuilderReadFile", String.format("Could not export ParseSnapshotContext binary to '%s'", batchOptions.binaryOutputFilename));
                }
            }
        }

        scene.geometryList = mgfModel == null ? null : mgfModel.geometries;
        SceneBuilder.sceneBuilderFillFacesBackPointers(scene.geometryList);

        long t = System.nanoTime();
        System.err.printf( Locale.US, "Reading took %g secs.\n", (double)(t - last) / 1000000000.0);
        last = t;

        // Check for errors
        if ( scene.geometryList == null || scene.geometryList.size() == 0 ) {
            return false; // Not successful
        }

        // Build the new patch list, this is duplicating already available
        // information and as such potentially dangerous, but we need it
        // so many times
        System.err.printf("Building patch list ... ");
        System.err.flush();

        scene.patchList = new ArrayList<>();
        SceneBuilder.sceneBuilderPatchList(scene.geometryList, scene.patchList);

        t = System.nanoTime();
        System.err.printf( Locale.US, "%g secs.\n", (double)(t - last) / 1000000000.0);
        last = t;

        // Build the list of patches on light sources from the patch list
        System.err.printf("Building light source patch list ... ");
        System.err.flush();

        SceneBuilder.sceneBuilderFillLightSourcePatchList(scene);

        t = System.nanoTime();
        System.err.printf( Locale.US, "%g secs.\n", (double)(t - last) / 1000000000.0);
        last = t;

        // Build a cluster hierarchy for the new scene
        System.err.printf("Building cluster hierarchy ... ");
        System.err.flush();

        scene.clusteredRootGeometry = SceneBuilder.sceneBuilderCreateClusterHierarchy(scene.patchList);

        // The top-level clusters, as RenderPark's ClusteredWorld: the children of
        // the root cluster, or the root itself when it is not a compound
        if ( scene.clusteredRootGeometry.className == GeometryClassId.COMPOUND ) {
            ArrayList<RpkGeometry> children = ((Compound)scene.clusteredRootGeometry).children;
            for ( int i = 0; children != null && i < children.size(); i++ ) {
                scene.clusteredGeometryList.add(children.get(i));
            }
        }
        else {
            scene.clusteredGeometryList.add(scene.clusteredRootGeometry);
            Logger.reportMessage(null, Logger.WARNING, "", "Strange clusters for this world ...");
        }

        // Dense face indices in depth-first order of the cluster hierarchy
        scene.buildFaceTable();

        t = System.nanoTime();
        System.err.printf( Locale.US, "%g secs.\n", (double)(t - last) / 1000000000.0);
        last = t;

        // Create the scene query, over the index the -scene-acceleration option chose
        scene.buildSceneQuery();

        t = System.nanoTime();
        System.err.printf( Locale.US, "Scene index (%s) creation took %g secs.\n", SceneQuery.methodName(scene.getAccelerationMethod()), (double)(t - last) / 1000000000.0);
        last = t;

        final Cie colorSpace = toneMapOptions.getColorSpace();
        // Estimate average radiance, for radiance to display RGB conversion
        System.err.printf("Computing some scene statistics ... ");
        System.err.flush();

        Statistics.instance().reader.numberOfPatches = Statistics.instance().reader.numberOfElements;
        SceneBuilder.sceneBuilderComputeStats(scene);
        Statistics.instance().radiance.referenceLuminance = 5.42 * ((1.0 - colorSpace.spectrumGray(Statistics.instance().radiance.averageReflectivity.getR(), Statistics.instance().radiance.averageReflectivity.getG(), Statistics.instance().radiance.averageReflectivity.getB())) * colorSpace.spectrumLuminance(Statistics.instance().radiance.estimatedAverageRadiance.getR(), Statistics.instance().radiance.estimatedAverageRadiance.getG(), Statistics.instance().radiance.estimatedAverageRadiance.getB()));

        t = System.nanoTime();
        System.err.printf( Locale.US, "%g secs.\n", (double)(t - last) / 1000000000.0);
        last = t;

        // Initialize tone mapping
        System.err.printf("Initializing tone mapping ... ");
        System.err.flush();

        Adaptation.initSceneAdaptation(scene.patchList, toneMapOptions);

        t = System.nanoTime();
        System.err.printf( Locale.US, "%g secs.\n", (double)(t - last) / 1000000000.0);
        last = t;

        // Print statistics report
        System.out.printf(
            Locale.US,
            "\nStats: radiance.totalEmittedPower ................: %f W\n"
                + "         radiance.estimatedAverageRadiance .........: %f W / sr\n"
                + "         averageReflectivity ..............: %f\n"
                + "         radiance.maxSelfEmittedRadiance ...........: %f W / sr\n"
                + "         radiance.maxSelfEmittedPower ..............: %f W\n"
                + "         toneMapOptions.realWorldAdaptionLuminance .........: %f cd / m2\n"
                + "         totalArea ........................: %f m2\n",
            colorSpace.spectrumGray(Statistics.instance().radiance.totalEmittedPower.getR(), Statistics.instance().radiance.totalEmittedPower.getG(), Statistics.instance().radiance.totalEmittedPower.getB()),
            colorSpace.spectrumGray(Statistics.instance().radiance.estimatedAverageRadiance.getR(), Statistics.instance().radiance.estimatedAverageRadiance.getG(), Statistics.instance().radiance.estimatedAverageRadiance.getB()),
            colorSpace.spectrumGray(Statistics.instance().radiance.averageReflectivity.getR(), Statistics.instance().radiance.averageReflectivity.getG(), Statistics.instance().radiance.averageReflectivity.getB()),
            colorSpace.spectrumGray(Statistics.instance().radiance.maxSelfEmittedRadiance.getR(), Statistics.instance().radiance.maxSelfEmittedRadiance.getG(), Statistics.instance().radiance.maxSelfEmittedRadiance.getB()),
            colorSpace.spectrumGray(Statistics.instance().radiance.maxSelfEmittedPower.getR(), Statistics.instance().radiance.maxSelfEmittedPower.getG(), Statistics.instance().radiance.maxSelfEmittedPower.getB()),
            toneMapOptions.realWorldAdaptionLuminance,
            Statistics.instance().radiance.totalArea);
        //scene->print();

        // Initialize radiance for the freshly loaded scene
        System.err.printf("Initializing radiance method ... ");
        System.err.flush();

        Radiance.setRadianceMethod(mgfContext.radianceMethod, scene, toneMapOptions);

        t = System.nanoTime();
        System.err.printf( Locale.US, "%g secs.\n", (double)(t - last) / 1000000000.0);

        // Remove possible render hooks
        RenderHookList.removeAllRenderHooks();

        SceneBuilder.removeEmptyMeshSurfaces(mgfContext, scene.geometryList);

        System.err.printf("Initialisations done.\n");

        return true;
    }

    public static void sceneBuilderCreateModel(
        int[] argc,
        String[] argv,
        ParseRuntimeContext mgfContext,
        Scene scene,
        ToneMappingContext toneMapOptions)
    {
        BatchOptions batchOptions = Batch.batchGetOptions();
        if ( batchOptions != null
             && batchOptions.importBinary
             && batchOptions.binaryInputFilename != null
             && !batchOptions.binaryInputFilename.isEmpty() ) {
            if ( !SceneBuilder.sceneBuilderReadFile(batchOptions.binaryInputFilename, mgfContext, scene, toneMapOptions) ) {
                System.exit(1);
            }
            return;
        }

        // All options should have disappeared from argv now
        if ( argc != null && argc.length > 0 && argc[0] > 1 ) {
            if ( argv[1] != null && argv[1].startsWith("-") ) {
                Logger.reportMessage(null, Logger.ERROR, "", String.format("Unrecognized option '%s'", argv[1]));
            }
            else if ( !SceneBuilder.sceneBuilderReadFile(argv[1], mgfContext, scene, toneMapOptions) ) {
                System.exit(1);
            }
        }
    }
}
