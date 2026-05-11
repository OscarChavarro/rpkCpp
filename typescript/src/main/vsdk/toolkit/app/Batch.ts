import { OutputStream } from "../../../java/io/OutputStream";
import { BatchOptions } from "./options/BatchOptions";
import { OptionsGroupBatch } from "./options/OptionsGroupBatch";
import { RendererConfiguration } from "../material/RendererConfiguration";
import { ImageOutputHandle } from "../io/image/ImageOutputHandle";
import { FileUncompressWrapper } from "../io/wrapper/FileUncompressWrapper";
import { RayTracer } from "../raycasting/common/RayTracer";
import { Canvas } from "../render/Canvas";
import { RadianceImageExporter } from "../render/RadianceImageExporter";
import { RenderOpenGL } from "../render/jogl/RenderOpenGL";
import { RadianceMethod } from "../scene/RadianceMethod";
import { Scene } from "../scene/Scene";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";
import { Raytrace } from "./Raytrace";

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
      isPipe[0],
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

    const extension = ImageOutputHandle.imageFileExtension(fileName);
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

        RenderOpenGL.renderGetNearFar(scene.camera, scene.geometryList);

        const wastedStart = process.hrtime.bigint();

        if (
          Batch.batchOptions.saveModulo !== 0
          && (iterationNumber % Batch.batchOptions.saveModulo) === 0
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
          Batch.batchOptions.radianceModelFileNameFormat !== null
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
