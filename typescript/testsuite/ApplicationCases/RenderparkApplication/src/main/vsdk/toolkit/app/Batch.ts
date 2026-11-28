import { OutputStream } from "vitral/dist/java/io/OutputStream";
import { Geometry } from "rpk-base/dist/vsdk/toolkit/skin/Geometry";
import { BoundingBox } from "rpk-base/dist/vsdk/toolkit/skin/AxisAlignedBoundingBox";
import { BatchOptions } from "./options/BatchOptions";
import { OptionsGroupBatch } from "./options/OptionsGroupBatch";
import { RendererConfiguration } from "rpk-base/dist/vsdk/toolkit/material/RendererConfiguration";
import { ImageOutputTarget } from "rpk-base/dist/vsdk/toolkit/render/ImageOutputTarget";
import { FileUncompressWrapper } from "rpk-base/dist/vsdk/toolkit/io/wrapper/FileUncompressWrapper";
import { RayTracer } from "rpk-base/dist/vsdk/toolkit/raycasting/common/RayTracer";
import { Canvas } from "rpk-base/dist/vsdk/toolkit/render/Canvas";
import { RadianceImageExporter } from "rpk-base/dist/vsdk/toolkit/render/RadianceImageExporter";
import { RadianceMethod } from "rpk-base/dist/vsdk/toolkit/scene/RadianceMethod";
import { Scene } from "rpk-base/dist/vsdk/toolkit/scene/Scene";
import { ToneMappingContext } from "rpk-base/dist/vsdk/toolkit/tonemap/ToneMappingContext";
import { Raytrace } from "./Raytrace";
import { RefinementReplicas } from "rpk-base/dist/vsdk/toolkit/galerkin/processing/RefinementReplicas";

type ProcessFileCallback = (
  fileName: string,
  outputStream: OutputStream | null,
  isPipe: number,
  scene: Scene,
  radianceMethod: RadianceMethod | null,
  rayTracer: RayTracer | null,
  toneMapOptions: ToneMappingContext,
  renderOptions: RendererConfiguration
) => void;

export class Batch {
  private static readonly batchOptions = new BatchOptions();
  private static currentRayTracer: RayTracer | null = null;

  private constructor() {
  }

  /**
  Fits the camera's near and far clipping distances to the scene geometry, the
  same way with or without an OpenGL display
  */
  private static fitCameraClippingPlanes(scene: Scene): void {
    if (scene.camera === null) {
      return;
    }
    if (scene.geometryList === null || scene.geometryList.length === 0) {
      scene.camera.far = 10.0;
      scene.camera.near = 0.1;
      return;
    }
    const bounds = new BoundingBox();
    Geometry.listBounds(scene.geometryList, bounds);
    scene.camera.fitClippingPlanes(bounds);
  }

  public static batchGetOptions(): BatchOptions {
    return Batch.batchOptions;
  }

  public static generalParseOptions(argc: number[], argv: string[]): void {
    OptionsGroupBatch.parse(argc, argv, Batch.batchOptions);
  }

  private static batchRayTraceSaveImage(
    fileName: string,
    outputStream: OutputStream | null,
    isPipe: number,
    scene: Scene,
    radianceMethod: RadianceMethod | null,
    rayTracer: RayTracer | null,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void {
    if (radianceMethod !== null || toneMapOptions !== null || renderOptions !== null) {
      // Parameters intentionally unused in C++ implementation.
    }
    Raytrace.rayTraceSaveImage(fileName, outputStream, isPipe, scene, rayTracer);
  }

  /**
  This routine was copied from uit.c, leaving out all interface related things
  */
  private static batchProcessFile(
    fileName: string,
    processFileCallback: ProcessFileCallback,
    scene: Scene,
    radianceMethod: RadianceMethod | null,
    rayTracer: RayTracer | null,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void {
    const isPipe = [0];
    const outputStream = FileUncompressWrapper.openOutputStreamCompressWrapper(fileName, isPipe);

    // Call the user supplied procedure to process the file
    processFileCallback(
      fileName,
      outputStream,
      isPipe[0]!,
      scene,
      radianceMethod,
      rayTracer,
      toneMapOptions,
      renderOptions
    );

    FileUncompressWrapper.closeOutputStream(outputStream);
  }

  private static batchSaveRadianceImage(
    fileName: string,
    outputStream: OutputStream | null,
    isPipe: number,
    scene: Scene,
    radianceMethod: RadianceMethod | null,
    rayTracer: RayTracer | null,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void {
    if (rayTracer !== null) {
      // Parameter intentionally unused in C++ implementation.
    }

    if (outputStream === null) {
      return;
    }

    Canvas.canvasPushMode();

    const extension = ImageOutputTarget.imageFileExtension(fileName);
    if (extension !== null && extension.substring(0, 6).toLowerCase() === "logluv") {
      process.stdout.write(`Saving LOGLUV image to file '${fileName}' ....... `);
    }
    else {
      process.stdout.write(`Saving RGB image to file '${fileName}' .......... `);
    }

    const t = process.hrtime.bigint();

    RadianceImageExporter.exportImage(
      fileName,
      outputStream,
      isPipe,
      scene,
      radianceMethod as RadianceMethod,
      toneMapOptions,
      renderOptions
    );

    process.stdout.write(`${Number(process.hrtime.bigint() - t) / 1_000_000_000.0} secs.\n`);
    Canvas.canvasPullMode();
  }

  private static batchSaveRadianceModel(
    fileName: string,
    outputStream: OutputStream | null,
    isPipe: number,
    scene: Scene,
    radianceMethod: RadianceMethod | null,
    rayTracer: RayTracer | null,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void {
    if (isPipe !== 0 || rayTracer !== null || toneMapOptions !== null) {
      // Parameters intentionally unused in C++ implementation.
    }

    if (outputStream === null) {
      return;
    }

    Canvas.canvasPushMode();
    process.stdout.write(`Saving VRML model to file '${fileName}' ... `);
    const t = process.hrtime.bigint();

    if (radianceMethod !== null) {
      radianceMethod.writeVRML(scene.camera!, outputStream, renderOptions);
    }

    process.stdout.write(`${Number(process.hrtime.bigint() - t) / 1_000_000_000.0} secs.\n`);
    Canvas.canvasPullMode();
  }

  private static formatFileName(format: string, iterationNumber: number): string {
    if (format === null || format.length === 0) {
      return "";
    }
    try {
      const paddedPattern = /%0(\d+)d/;
      if (paddedPattern.test(format)) {
        return format.replace(paddedPattern, (_m, w) => `${iterationNumber}`.padStart(Number(w), "0"));
      }
      return format.replace(/%d/g, `${iterationNumber}`);
    }
    catch (_e) {
      return format;
    }
  }

  public static batchExecuteRadianceSimulation(
    scene: Scene,
    radianceMethod: RadianceMethod | null,
    rayTracer: RayTracer | null,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void {
    if (scene === null || scene.geometryList === null || scene.geometryList.length === 0) {
      process.stdout.write("Empty world? Missing argument to some command line parameter option?\n");
      return;
    }

    Batch.currentRayTracer = rayTracer;

    let startTime = process.hrtime.bigint();
    let wastedSecs = 0.0;

    if (radianceMethod !== null) {
      process.stdout.write(`Doing ${radianceMethod.getRadianceMethodName()} ...\n`);

      // The importance computations render patch IDs during the first step
      Batch.fitCameraClippingPlanes(scene);

      let done = false;
      for (
        let iterationNumber = 0;
        iterationNumber < Batch.batchOptions.iterations && !done;
        iterationNumber++
      ) {
        process.stdout.write(
          "-----------------------------------\n"
          + `radiance iteration ${`${iterationNumber}`.padStart(4, "0")}\n`
          + "-----------------------------------\n\n"
        );

        Canvas.canvasPushMode();
        done = radianceMethod.doStep(scene, renderOptions);
        Canvas.canvasPullMode();

        process.stdout.write(`${radianceMethod.getStats()}`);

        Batch.fitCameraClippingPlanes(scene);

        const wastedStart = process.hrtime.bigint();

        // With -save-last-only, the image and the model are saved only
        // after the last iteration (the last one asked for, or the one
        // after which the method is done)
        const lastIteration = done || iterationNumber === Batch.batchOptions.iterations - 1;

        // TypeScript adaptation: a parallel Jacobi replica (see
        // RefinementReplicas) is only needed for the iterations (unlike a
        // replica of the Random Walk final tracing), and all it
        // would do after the last one (the images, the model and the ray
        // tracing, written to nowhere) would only take time and memory, as
        // many times as there are threads; it ends there instead
        if (lastIteration && RefinementReplicas.isIterationReplica()) {
          return;
        }
        const saveImage = Batch.batchOptions.saveLastOnly !== 0
          ? lastIteration
          : Batch.batchOptions.saveModulo !== 0 && (iterationNumber % Batch.batchOptions.saveModulo) === 0;
        const saveModel = Batch.batchOptions.saveLastOnly === 0 || lastIteration;

        if (
          saveImage
          && Batch.batchOptions.radianceImageFileNameFormat !== null
          && Batch.batchOptions.radianceImageFileNameFormat.length > 0
        ) {
          const fileName = Batch.formatFileName(Batch.batchOptions.radianceImageFileNameFormat, iterationNumber);
          Batch.batchProcessFile(
            fileName,
            Batch.batchSaveRadianceImage,
            scene,
            radianceMethod,
            rayTracer,
            toneMapOptions,
            renderOptions
          );
        }

        if (
          saveModel
          && Batch.batchOptions.radianceModelFileNameFormat !== null
          && Batch.batchOptions.radianceModelFileNameFormat.length > 0
        ) {
          const fileName = Batch.formatFileName(Batch.batchOptions.radianceModelFileNameFormat, iterationNumber);
          Batch.batchProcessFile(
            fileName,
            Batch.batchSaveRadianceModel,
            scene,
            radianceMethod,
            rayTracer,
            toneMapOptions,
            renderOptions
          );
        }

        wastedSecs += Number(wastedStart - process.hrtime.bigint()) / 1_000_000_000.0;
      }
    }
    else {
      process.stdout.write("(No world-space radiance computations are being done)\n");
    }

    if (Batch.batchOptions.timings !== 0) {
      process.stdout.write(
        `Radiance total time ${Number(process.hrtime.bigint() - startTime) / 1_000_000_000.0 - wastedSecs} secs.\n`
      );
    }

    if (Batch.currentRayTracer !== null) {
      process.stdout.write(`Doing ${Batch.currentRayTracer.getName()} ...\n`);

      startTime = process.hrtime.bigint();
      Raytrace.rayTraceExecute(
        null,
        null,
        0,
        scene,
        radianceMethod as RadianceMethod,
        Batch.currentRayTracer,
        toneMapOptions,
        renderOptions
      );

      if (Batch.batchOptions.timings !== 0) {
        process.stdout.write(
          `Raytracing total time ${Number(process.hrtime.bigint() - startTime) / 1_000_000_000.0} secs.\n`
        );
      }

      Batch.batchProcessFile(
        Batch.batchOptions.raytracingImageFileName,
        Batch.batchRayTraceSaveImage,
        scene,
        radianceMethod,
        Batch.currentRayTracer,
        toneMapOptions,
        renderOptions
      );
    }
    else {
      process.stdout.write("(No pixel-based radiance computations are being done)\n");
    }

    process.stdout.write("Computations finished.\n");
    Batch.currentRayTracer = null;
  }
}
