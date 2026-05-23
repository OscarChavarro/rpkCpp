import { File } from "vitral/dist/java/io/File";
import { FileInputStream } from "vitral/dist/java/io/FileInputStream";
import { BatchOptions } from "./options/BatchOptions";
import { OptionsGroupCore } from "./options/OptionsGroupCore";
import { Cie } from "vitral/dist/vsdk/toolkit/common/color/Cie";
import { ColorRgb } from "vitral/dist/vsdk/toolkit/common/color/ColorRgb";
import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { Statistics } from "vitral/dist/vsdk/toolkit/common/statistics/Statistics";
import { BinaryModelDeserializer } from "vitral/dist/vsdk/toolkit/io/bin/reader/BinaryModelDeserializer";
import { BinaryModelSerializer } from "vitral/dist/vsdk/toolkit/io/bin/writer/BinaryModelSerializer";
import { ParseRuntimeContext } from "vitral/dist/vsdk/toolkit/io/context/ParseRuntimeContext";
import { ParseSnapshotContext } from "vitral/dist/vsdk/toolkit/io/context/ParseSnapshotContext";
import { MgfParserLoader } from "vitral/dist/vsdk/toolkit/io/mgf/MgfParserLoader";
import { BsdfComponent } from "vitral/dist/vsdk/toolkit/material/BsdfComponent";
import { Material } from "vitral/dist/vsdk/toolkit/material/Material";
import { XxdfComponentFlag } from "vitral/dist/vsdk/toolkit/material/XxdfComponentFlag";
import { MeshSurfaceVisitor } from "vitral/dist/vsdk/toolkit/numericalAnalysis/MeshSurfaceVisitor";
import { PatchVisitor } from "vitral/dist/vsdk/toolkit/numericalAnalysis/PatchVisitor";
import { RenderHookList } from "vitral/dist/vsdk/toolkit/render/RenderHookList";
import { Background } from "vitral/dist/vsdk/toolkit/scene/Background";
import { PatchClusterOctreeNode } from "vitral/dist/vsdk/toolkit/scene/PatchClusterOctreeNode";
import { Scene } from "vitral/dist/vsdk/toolkit/scene/Scene";
import { VoxelGrid } from "vitral/dist/vsdk/toolkit/scene/VoxelGrid";
import { Compound } from "vitral/dist/vsdk/toolkit/skin/Compound";
import { Geometry } from "vitral/dist/vsdk/toolkit/skin/Geometry";
import { GeometryClassId } from "vitral/dist/vsdk/toolkit/skin/GeometryClassId";
import { MeshSurface } from "vitral/dist/vsdk/toolkit/skin/MeshSurface";
import { Patch } from "vitral/dist/vsdk/toolkit/environment/geometry/elements/Patch";
import { ToneMappingContext } from "vitral/dist/vsdk/toolkit/tonemap/ToneMappingContext";
import { Vector3D } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3D";
import { Adaptation } from "./Adaptation";
import { Batch } from "./Batch";
import { Radiance } from "./Radiance";

export class SceneBuilder {
  private static readonly ALL_XXDF_COMPONENTS =
    XxdfComponentFlag.DIFFUSE_COMPONENT
    | XxdfComponentFlag.GLOSSY_COMPONENT
    | XxdfComponentFlag.SPECULAR_COMPONENT;

  private static readonly ALL_BSDF_COMPONENTS =
    BsdfComponent.BRDF_DIFFUSE_COMPONENT
    | BsdfComponent.BRDF_GLOSSY_COMPONENT
    | BsdfComponent.BRDF_SPECULAR_COMPONENT
    | BsdfComponent.BTDF_DIFFUSE_COMPONENT
    | BsdfComponent.BTDF_GLOSSY_COMPONENT
    | BsdfComponent.BTDF_SPECULAR_COMPONENT;

  private constructor() {
  }

  private static sceneBuilderPatchAccumulateStats(patch: Patch): void {
    const E = PatchVisitor.averageEmittance(patch, SceneBuilder.ALL_XXDF_COMPONENTS);
    const R = PatchVisitor.averageNormalAlbedo(patch, SceneBuilder.ALL_BSDF_COMPONENTS);
    const power = new ColorRgb();

    Statistics.instance().radiance.totalArea += patch.area;
    power.scaledCopy(patch.area, E);
    Statistics.instance().radiance.totalEmittedPower.add(Statistics.instance().radiance.totalEmittedPower, power);
    Statistics.instance().radiance.averageReflectivity.addScaled(Statistics.instance().radiance.averageReflectivity, patch.area, R);
    E.scale(1.0 / globalThis.Math.PI);
    Statistics.instance().radiance.maxSelfEmittedRadiance.maximum(E, Statistics.instance().radiance.maxSelfEmittedRadiance);
    Statistics.instance().radiance.maxSelfEmittedPower.maximum(power, Statistics.instance().radiance.maxSelfEmittedPower);
  }

  private static sceneBuilderComputeStats(scene: Scene): void {
    const zero = new Vector3D();
    const one = new ColorRgb();
    const averageAbsorption = new ColorRgb();

    one.setMonochrome(1.0);
    zero.set(0, 0, 0);

    // Initialize
    Statistics.instance().radiance.totalEmittedPower.clear();
    Statistics.instance().radiance.averageReflectivity.clear();
    Statistics.instance().radiance.maxSelfEmittedRadiance.clear();
    Statistics.instance().radiance.maxSelfEmittedPower.clear();
    Statistics.instance().radiance.totalArea = 0.0;

    // Accumulate
    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      SceneBuilder.sceneBuilderPatchAccumulateStats(scene.patchList[i]!);
    }

    // Averages
    Statistics.instance().radiance.averageReflectivity.scaleInverse(
      Statistics.instance().radiance.totalArea,
      Statistics.instance().radiance.averageReflectivity
    );
    averageAbsorption.subtract(one, Statistics.instance().radiance.averageReflectivity);
    Statistics.instance().radiance.estimatedAverageRadiance.scaleInverse(
      globalThis.Math.PI * Statistics.instance().radiance.totalArea,
      Statistics.instance().radiance.totalEmittedPower
    );

    // Include background radiation
    const BP = Background.backgroundPower(scene.background, zero);
    BP.scale(1.0 / (4.0 * globalThis.Math.PI));
    Statistics.instance().radiance.totalEmittedPower.add(Statistics.instance().radiance.totalEmittedPower, BP);
    Statistics.instance().radiance.estimatedAverageRadiance.add(Statistics.instance().radiance.estimatedAverageRadiance, BP);
    Statistics.instance().radiance.estimatedAverageRadiance.divide(Statistics.instance().radiance.estimatedAverageRadiance, averageAbsorption);

    Statistics.instance().potential.totalDirectPotential = 0.0;
    Statistics.instance().potential.maxDirectPotential = 0.0;
    Statistics.instance().potential.averageDirectPotential = 0.0;
    Statistics.instance().potential.maxDirectImportance = 0.0;
  }

  /**
  Adds the background to the global light source patch list
  */
  private static sceneBuilderAddBackgroundToLightSourceList(scene: Scene): void {
    if (scene.background !== null && scene.background.bkgPatch !== null && scene.lightSourcePatchList !== null) {
      scene.lightSourcePatchList.push(scene.background.bkgPatch);
      Statistics.instance().reader.numberOfLightSources++;
    }
  }

  /**
  Adds the patch to the global light source patch list if the patch is on
  a light source (i.e. when the surfaces material has a non-null edf)
  */
  private static sceneBuilderAddPatchToLightSourceListIfLightSource(lights: Patch[], patch: Patch): void {
    if (
      patch !== null
      && patch.material !== null
      && patch.material.getEdf() !== null
    ) {
      lights.push(patch);
      Statistics.instance().reader.numberOfLightSources++;
    }
  }

  /**
  Build the global light source patch list
  */
  private static sceneBuilderFillLightSourcePatchList(scene: Scene): void {
    const lights: Patch[] = [];
    Statistics.instance().reader.numberOfLightSources = 0;

    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      SceneBuilder.sceneBuilderAddPatchToLightSourceListIfLightSource(lights, scene.patchList[i]!);
    }

    scene.lightSourcePatchList = lights;
    SceneBuilder.sceneBuilderAddBackgroundToLightSourceList(scene);
  }

  /**
  Creates a hierarchical model of the discrete scene (the patches in the scene) using the simple
  algorithm described in
  - Per Christensen, "Hierarchical Techniques for Glossy Global Illumination",
    PhD Thesis, University of Washington, 1995, p 116
  This hierarchy is often much more efficient for tracing rays and clustering radiosity algorithms
  than the given hierarchy of bounding boxes. A pointer to the toplevel "cluster" is returned
  */
  private static sceneBuilderCreateClusterHierarchy(patches: Patch[]): Geometry {
    // Create a toplevel cluster containing (references to) all the patches in the scene
    const rootCluster = new PatchClusterOctreeNode(patches);

    // Split the toplevel cluster recursively into sub-clusters
    rootCluster.splitCluster();

    // Convert to a geometry hierarchy, disposing of clusters
    const rootGeometry = rootCluster.convertClusterToGeometry();
    rootCluster.destroy();
    return rootGeometry;
  }

  /**
  Builds a linear list of patches making up all the geometries in the list, whether
  they are primitive or not
  */
  private static sceneBuilderPatchList(geometryList: Geometry[] | null, patchList: Patch[]): void {
    for (let i = 0; geometryList !== null && i < geometryList.length; i++) {
      const geometry = geometryList[i]!;
      if (geometry.isCompound()) {
        // Recursive case
        const compound = geometry as Compound;
        SceneBuilder.sceneBuilderPatchList(compound.children, patchList);
      }
      else {
        // Trivial case
        const patchesFromNonCompounds = Geometry.patchListReference(geometry!);

        for (let j = 0; patchesFromNonCompounds !== null && j < patchesFromNonCompounds.length; j++) {
          const patch = patchesFromNonCompounds[j]!;
          if (patch !== null) {
            patchList.push(patch);
          }
        }
      }
    }
  }

  private static sceneBuilderFillFacesBackPointers(geometryList: Geometry[] | null): void {
    if (geometryList === null) {
      return;
    }
    for (let i = 0; i < geometryList.length; i++) {
      const geometry = geometryList[i]!;
      if (geometry === null) {
        continue;
      }
      if (geometry.isCompound()) {
        const compound = geometry as Compound;
        SceneBuilder.sceneBuilderFillFacesBackPointers(compound.children);
        continue;
      }
      if (geometry.className === GeometryClassId.SURFACE_MESH) {
        MeshSurfaceVisitor.fillFacesBackPointers(geometry as MeshSurface);
      }
    }
  }

  private static sceneBuilderCollectGeometriesRecursive(
    source: Geometry[] | null,
    target: Geometry[] | null
  ): void {
    if (source === null || target === null) {
      return;
    }

    for (let i = 0; i < source.length; i++) {
      const geometry = source[i]!;
      if (geometry === null) {
        continue;
      }

      let alreadyInTarget = false;
      for (let j = 0; j < target.length; j++) {
        if (target[j] === geometry) {
          alreadyInTarget = true;
          break;
        }
      }
      if (!alreadyInTarget) {
        target.push(geometry);
      }

      if (geometry.isCompound()) {
        const compound = geometry as Compound;
        SceneBuilder.sceneBuilderCollectGeometriesRecursive(compound.children, target);
      }
    }
  }

  private static sceneBuilderApplyModelToMgfContext(mgfContext: ParseRuntimeContext, mgfModel: ParseSnapshotContext): void {
    if (mgfContext === null || mgfModel === null) {
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
    mgfContext.warpConeEnds = mgfModel.warpConeEnds;
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
    mgfContext.geometryBuildState.warpConeEnds = mgfContext.warpConeEnds;
    mgfContext.materialState.materials = mgfContext.materials;
    mgfContext.parserConfig.monochrome = mgfContext.monochrome;
    mgfContext.parserConfig.singleSided = mgfContext.singleSided;
    mgfContext.parserConfig.numberOfQuarterCircleDivisions = mgfContext.numberOfQuarterCircleDivisions;
    mgfContext.readerStackState.readerContext = mgfContext.readerContext;
    mgfContext.transformStack.transformContext = mgfContext.transformContext;

    mgfContext.currentMaterial = null;
    if (mgfContext.materials !== null && mgfContext.currentMaterialName !== null) {
      for (let i = 0; i < mgfContext.materials.length; i++) {
      const material = mgfContext.materials[i]!;
        if (
          material !== null
          && material.getName() !== null
          && material.getName() === mgfContext.currentMaterialName
        ) {
          mgfContext.currentMaterial = material;
          break;
        }
      }
    }
    mgfContext.materialState.currentMaterial = mgfContext.currentMaterial;

    if (mgfContext.allGeometries !== null) {
      mgfContext.allGeometries.length = 0;
    }
    mgfContext.allGeometries = [];
    mgfContext.geometryBuildState.allGeometries = mgfContext.allGeometries;
    SceneBuilder.sceneBuilderCollectGeometriesRecursive(mgfModel.currentGeometryList, mgfContext.allGeometries);
    if (mgfModel.geometries !== mgfModel.currentGeometryList) {
      SceneBuilder.sceneBuilderCollectGeometriesRecursive(mgfModel.geometries, mgfContext.allGeometries);
    }
  }

  private static removeEmptyMeshSurfaces(mgfContext: ParseRuntimeContext, geometryList: Geometry[] | null): void {
    if (geometryList === null) {
      return;
    }

    for (let i = 0; i < geometryList.length; i++) {
      const geometry = geometryList[i]!;
      if (geometry.className === GeometryClassId.SURFACE_MESH) {
        const mesh = geometry as MeshSurface;
        if (mesh.vertices !== null && mesh.vertices.length === 0) {
          if (mgfContext.allGeometries !== null) {
            for (let j = 0; j < mgfContext.allGeometries.length; j++) {
              const deletionCandidate = mgfContext.allGeometries[j];
              if (deletionCandidate === geometry) {
                mgfContext.allGeometries.splice(j, 1);
                break;
              }
            }
          }
          Geometry.destroy(geometry);
          geometryList.splice(i, 1);
          i--;
        }
      }
    }
  }

  private static sceneBuilderHasExtension(fileName: string | null, extension: string | null): boolean {
    if (fileName === null || extension === null) {
      return false;
    }

    const fileNameLength = fileName.length;
    const extensionLength = extension.length;
    if (fileNameLength < extensionLength) {
      return false;
    }

    return fileName.substring(fileNameLength - extensionLength).toLowerCase() === extension.toLowerCase();
  }

  private static sceneBuilderBuildBinaryFallbackPath(mgfFileName: string): string | null {
    if (!SceneBuilder.sceneBuilderHasExtension(mgfFileName, ".mgf")) {
      return null;
    }

    return mgfFileName.substring(0, mgfFileName.length - 4) + ".bin";
  }

  private static sceneBuilderIsReadableRegularFile(fileName: string | null): boolean {
    if (fileName === null || fileName.length === 0) {
      return false;
    }

    const file = new File(fileName);
    if (!file.exists() || !file.isFile() || !file.canRead()) {
      file.dispose();
      return false;
    }
    file.dispose();

    const input = new FileInputStream(fileName);
    try {
      const firstByte = input.read();
      return firstByte >= 0;
    }
    catch (_ignored) {
      return false;
    }
    finally {
      input.close();
    }
  }

  private static sceneBuilderValidateReadableFile(
    fileName: string,
    fileRole: string
  ): boolean {
    const file = new File(fileName);
    if (!file.exists()) {
      VsdkLogger.error(
        "SceneBuilder::sceneBuilderReadFile",
        "Requested %s file '%s' does not exist",
        fileRole,
        fileName
      );
      file.dispose();
      return false;
    }
    if (!file.isFile()) {
      VsdkLogger.error(
        "SceneBuilder::sceneBuilderReadFile",
        "Requested %s file '%s' is not a regular file",
        fileRole,
        fileName
      );
      file.dispose();
      return false;
    }
    if (!file.canRead()) {
      VsdkLogger.error(
        "SceneBuilder::sceneBuilderReadFile",
        "Requested %s file '%s' is not readable",
        fileRole,
        fileName
      );
      file.dispose();
      return false;
    }
    file.dispose();

    const input = new FileInputStream(fileName);
    try {
      const firstByte = input.read();
      if (firstByte < 0) {
        VsdkLogger.error(
          "SceneBuilder::sceneBuilderReadFile",
          "Requested %s file '%s' is empty",
          fileRole,
          fileName
        );
        return false;
      }
    }
    catch (_ignored) {
      VsdkLogger.error(
        "SceneBuilder::sceneBuilderReadFile",
        "Requested %s file '%s' is not readable",
        fileRole,
        fileName
      );
      return false;
    }
    finally {
      input.close();
    }

    return true;
  }

  /**
  Tries to read the scene in the given file. Returns false if not successful.
  Returns true if successful
  */
  private static sceneBuilderReadFile(
    fileName: string,
    mgfContext: ParseRuntimeContext,
    scene: Scene,
    toneMapOptions: ToneMappingContext
  ): boolean {
    const batchOptions = Batch.batchGetOptions();
    const importBinaryOption =
      batchOptions !== null
      && batchOptions.importBinary
      && batchOptions.binaryInputFilename !== null
      && batchOptions.binaryInputFilename.length > 0;
    const requestedInputName = importBinaryOption ? batchOptions.binaryInputFilename : fileName;

    let readBinaryModel =
      importBinaryOption
      || SceneBuilder.sceneBuilderHasExtension(requestedInputName, ".bin");

    let fallbackBinaryInputName: string | null = null;
    let inputName = requestedInputName;

    if (
      !readBinaryModel
      && SceneBuilder.sceneBuilderHasExtension(requestedInputName, ".mgf")
      && !SceneBuilder.sceneBuilderIsReadableRegularFile(requestedInputName)
    ) {
      fallbackBinaryInputName = SceneBuilder.sceneBuilderBuildBinaryFallbackPath(requestedInputName);
      if (
        fallbackBinaryInputName !== null
        && SceneBuilder.sceneBuilderIsReadableRegularFile(fallbackBinaryInputName)
      ) {
        readBinaryModel = true;
        inputName = fallbackBinaryInputName;
      }
    }

    // Check whether the file can be opened/read
    if (readBinaryModel && !SceneBuilder.sceneBuilderValidateReadableFile(inputName, "binary model")) {
      return false;
    }

    if (!readBinaryModel && fileName !== null && fileName.length > 0 && fileName.charAt(0) !== "#") {
      if (!SceneBuilder.sceneBuilderValidateReadableFile(fileName, "scene")) {
        return false;
      }
    }

    // Get current directory from the filename
    let currentDirectory = "";
    if (inputName !== null) {
      const slash = inputName.lastIndexOf("/");
      if (slash >= 0) {
        currentDirectory = inputName.substring(0, slash);
      }
    }
    if (currentDirectory.length > 0) {
      // Kept for parity with C++ flow; currently unused.
    }

    // Prepare if errors occur when reading the new scene will abort
    scene.geometryList = null;

    Patch.setNextId(1);
    scene.background = OptionsGroupCore.createBackground();

    // Read the source scene description into a ParseSnapshotContext snapshot
    process.stderr.write(`Reading the scene from file '${inputName}' ... \n`);
    let last = process.hrtime.bigint();
    let mgfModel: ParseSnapshotContext | null = null;

    if (readBinaryModel) {
      mgfModel = BinaryModelDeserializer.read(inputName);
      if (mgfModel !== null) {
        SceneBuilder.sceneBuilderApplyModelToMgfContext(mgfContext, mgfModel);
      }
    }
    else {
      mgfModel = MgfParserLoader.readMgf(fileName, mgfContext);
      if (
        mgfModel !== null
        && batchOptions !== null
        && batchOptions.exportBinary
        && batchOptions.binaryOutputFilename !== null
        && batchOptions.binaryOutputFilename.length > 0
      ) {
        process.stderr.write(
          `Exporting loaded ParseSnapshotContext to binary '${batchOptions.binaryOutputFilename}' ... `
        );
        const binarySaved = BinaryModelSerializer.write(
          mgfModel,
          batchOptions.binaryOutputFilename
        );
        if (binarySaved) {
          process.stderr.write("done.\n");
        }
        else {
          process.stderr.write("failed.\n");
          VsdkLogger.error(
            "SceneBuilder::sceneBuilderReadFile",
            "Could not export ParseSnapshotContext binary to '%s'",
            batchOptions.binaryOutputFilename
          );
        }
      }
    }

    scene.geometryList = mgfModel === null ? null : mgfModel.geometries;
    SceneBuilder.sceneBuilderFillFacesBackPointers(scene.geometryList);

    let t = process.hrtime.bigint();
    process.stderr.write(`Reading took ${Number(t - last) / 1_000_000_000.0} secs.\n`);
    last = t;

    // Check for errors
    if (scene.geometryList === null || scene.geometryList.length === 0) {
      return false; // Not successful
    }

    // Build the new patch list, this is duplicating already available
    // information and as such potentially dangerous, but we need it
    // so many times
    process.stderr.write("Building patch list ... ");

    scene.patchList = [];
    SceneBuilder.sceneBuilderPatchList(scene.geometryList, scene.patchList);

    t = process.hrtime.bigint();
    process.stderr.write(`${Number(t - last) / 1_000_000_000.0} secs.\n`);
    last = t;

    // Build the list of patches on light sources from the patch list
    process.stderr.write("Building light source patch list ... ");

    SceneBuilder.sceneBuilderFillLightSourcePatchList(scene);

    t = process.hrtime.bigint();
    process.stderr.write(`${Number(t - last) / 1_000_000_000.0} secs.\n`);
    last = t;

    // Build a cluster hierarchy for the new scene
    process.stderr.write("Building cluster hierarchy ... ");

    scene.clusteredRootGeometry = SceneBuilder.sceneBuilderCreateClusterHierarchy(scene.patchList);

    if (scene.clusteredRootGeometry.className !== GeometryClassId.COMPOUND) {
      VsdkLogger.warning(null, "Strange clusters for this world ...");
    }

    t = process.hrtime.bigint();
    process.stderr.write(`${Number(t - last) / 1_000_000_000.0} secs.\n`);
    last = t;

    // Create the scene level voxel grid
    scene.voxelGrid = new VoxelGrid(scene.clusteredRootGeometry);

    t = process.hrtime.bigint();
    process.stderr.write(`Voxel grid creation took ${Number(t - last) / 1_000_000_000.0} secs.\n`);
    last = t;

    // Estimate average radiance, for radiance to display RGB conversion
    process.stderr.write("Computing some scene statistics ... ");

    Statistics.instance().reader.numberOfPatches = Statistics.instance().reader.numberOfElements;
    SceneBuilder.sceneBuilderComputeStats(scene);
    Statistics.instance().radiance.referenceLuminance =
      5.42
      * (
        (1.0 - Cie.spectrumGray(
          Statistics.instance().radiance.averageReflectivity.r,
          Statistics.instance().radiance.averageReflectivity.g,
          Statistics.instance().radiance.averageReflectivity.b
        ))
        * Cie.spectrumLuminance(
          Statistics.instance().radiance.estimatedAverageRadiance.r,
          Statistics.instance().radiance.estimatedAverageRadiance.g,
          Statistics.instance().radiance.estimatedAverageRadiance.b
        )
      );

    t = process.hrtime.bigint();
    process.stderr.write(`${Number(t - last) / 1_000_000_000.0} secs.\n`);
    last = t;

    // Initialize tone mapping
    process.stderr.write("Initializing tone mapping ... ");

    Adaptation.initSceneAdaptation(scene.patchList, toneMapOptions);

    t = process.hrtime.bigint();
    process.stderr.write(`${Number(t - last) / 1_000_000_000.0} secs.\n`);
    last = t;

    // Print statistics report
    process.stdout.write(
      `\nStats: radiance.totalEmittedPower ................: ${Cie.spectrumGray(Statistics.instance().radiance.totalEmittedPower.r, Statistics.instance().radiance.totalEmittedPower.g, Statistics.instance().radiance.totalEmittedPower.b)} W\n`
      + `         radiance.estimatedAverageRadiance .........: ${Cie.spectrumGray(Statistics.instance().radiance.estimatedAverageRadiance.r, Statistics.instance().radiance.estimatedAverageRadiance.g, Statistics.instance().radiance.estimatedAverageRadiance.b)} W / sr\n`
      + `         averageReflectivity ..............: ${Cie.spectrumGray(Statistics.instance().radiance.averageReflectivity.r, Statistics.instance().radiance.averageReflectivity.g, Statistics.instance().radiance.averageReflectivity.b)}\n`
      + `         radiance.maxSelfEmittedRadiance ...........: ${Cie.spectrumGray(Statistics.instance().radiance.maxSelfEmittedRadiance.r, Statistics.instance().radiance.maxSelfEmittedRadiance.g, Statistics.instance().radiance.maxSelfEmittedRadiance.b)} W / sr\n`
      + `         radiance.maxSelfEmittedPower ..............: ${Cie.spectrumGray(Statistics.instance().radiance.maxSelfEmittedPower.r, Statistics.instance().radiance.maxSelfEmittedPower.g, Statistics.instance().radiance.maxSelfEmittedPower.b)} W\n`
      + `         toneMapOptions.realWorldAdaptionLuminance .........: ${toneMapOptions.realWorldAdaptionLuminance} cd / m2\n`
      + `         totalArea ........................: ${Statistics.instance().radiance.totalArea} m2\n`
    );

    // Initialize radiance for the freshly loaded scene
    process.stderr.write("Initializing radiance method ... ");

    Radiance.setRadianceMethod(mgfContext.radianceMethod, scene, toneMapOptions);

    t = process.hrtime.bigint();
    process.stderr.write(`${Number(t - last) / 1_000_000_000.0} secs.\n`);

    // Remove possible render hooks
    RenderHookList.removeAllRenderHooks();

    SceneBuilder.removeEmptyMeshSurfaces(mgfContext, scene.geometryList);

    process.stderr.write("Initialisations done.\n");

    return true;
  }

  public static sceneBuilderCreateModel(
    argc: number[],
    argv: string[],
    mgfContext: ParseRuntimeContext,
    scene: Scene,
    toneMapOptions: ToneMappingContext
  ): void {
    const batchOptions = Batch.batchGetOptions();
    if (
      batchOptions !== null
      && batchOptions.importBinary
      && batchOptions.binaryInputFilename !== null
      && batchOptions.binaryInputFilename.length > 0
    ) {
      if (!SceneBuilder.sceneBuilderReadFile(batchOptions.binaryInputFilename, mgfContext, scene, toneMapOptions)) {
        process.exit(1);
      }
      return;
    }

    // All options should have disappeared from argv now
    if (argc !== null && argc.length > 0 && argc[0]! > 1) {
      if (argv[1] !== null && argv[1]!.startsWith("-")) {
        VsdkLogger.error(null, "Unrecognized option '%s'", argv[1]);
      }
      else if (!SceneBuilder.sceneBuilderReadFile(argv[1]!, mgfContext, scene, toneMapOptions)) {
        process.exit(1);
      }
    }
  }
}
