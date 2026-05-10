import { OutputStream } from "../../../java/io/OutputStream";
import { ArrayList } from "../../../java/util/ArrayList";
import { OptionsGroupRaytracing } from "./options/OptionsGroupRaytracing";
import { Logger as VsdkLogger } from "../common/logging/Logger";
import { RenderOptions } from "../common/RenderOptions";
import { ImageOutputHandle } from "../io/image/ImageOutputHandle";
import { BidirectionalPathRaytracer } from "../raycasting/bidirectionalRaytracing/BidirectionalPathRaytracer";
import { BidirectionalPathTracingState } from "../raycasting/bidirectionalRaytracing/BidirectionalPathTracingState";
import { LightList } from "../raycasting/bidirectionalRaytracing/LightList";
import { RayTracer } from "../raycasting/common/RayTracer";
import { RayCaster } from "../raycasting/simple/RayCaster";
import { RayMatter } from "../raycasting/simple/RayMatter";
import { RayMatterState } from "../raycasting/simple/RayMatterState";
import { StochasticRaytracer } from "../raycasting/stochasticRaytracing/StochasticRaytracer";
import { StochasticRayTracingState } from "../raycasting/stochasticRaytracing/StochasticRayTracingState";
import { Canvas } from "../render/Canvas";
import { RadianceMethod } from "../scene/RadianceMethod";
import { Scene } from "../scene/Scene";
import { Patch } from "../skin/Patch";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";

export class Raytrace {
  private constructor() {
  }

  private static toArrayList(input: Patch[] | null): ArrayList<Patch> {
    const out = new ArrayList<Patch>();
    for (let i = 0; input !== null && i < input.length; i++) {
      out.add(input[i]);
    }
    return out;
  }

  /**
  This routine sets the current raytracing method to be used
  */
  private static rayTraceSetMethod(
    rayTracer: RayTracer | null,
    lightSourcePatches: Patch[] | null,
    lightList: Array<LightList | null> | null
  ): void {
    if (lightList !== null) {
      // Parameter intentionally unused in C++ implementation.
    }
    if (rayTracer !== null) {
      rayTracer.initialize(Raytrace.toArrayList(lightSourcePatches));
    }
  }

  private static rayTraceCreateRayTracerFromName(
    rayTracerName: string,
    scene: Scene,
    toneMapOptions: ToneMappingContext,
    rayMatterState: RayMatterState,
    bidirectionalPathState: BidirectionalPathTracingState,
    stochasticRayTracingState: StochasticRayTracingState,
    lightList: Array<LightList | null> | null
  ): RayTracer | null {
    let newRaytracer: RayTracer | null;
    if (rayTracerName === "RayMatting") {
      newRaytracer = new RayMatter(null, scene.camera, rayMatterState, toneMapOptions);
    }
    else if (rayTracerName === "RayCasting") {
      newRaytracer = new RayCaster(null, scene.camera, toneMapOptions);
    }
    else if (rayTracerName === "BidirectionalPathTracing") {
      newRaytracer = new BidirectionalPathRaytracer(bidirectionalPathState, lightList === null ? null : lightList[0]);
    }
    else if (rayTracerName === "StochasticRaytracing") {
      newRaytracer = new StochasticRaytracer(lightList === null ? null : lightList[0], stochasticRayTracingState);
    }
    else {
      newRaytracer = null;
    }

    Raytrace.rayTraceSetMethod(newRaytracer, scene.lightSourcePatchList, lightList);

    if (newRaytracer === null && (rayTracerName === null || rayTracerName.substring(0, 4).toLowerCase() !== "none")) {
      VsdkLogger.error(null, "Invalid raytracing method name '%s'", rayTracerName);
    }

    return newRaytracer;
  }

  public static rayTraceCreate(
    scene: Scene,
    toneMapOptions: ToneMappingContext,
    rayTracerName: string,
    rayMatterState: RayMatterState,
    bidirectionalPathState: BidirectionalPathTracingState,
    stochasticRayTracingState: StochasticRayTracingState,
    lightList: Array<LightList | null> | null
  ): RayTracer | null {
    const rayTracer = Raytrace.rayTraceCreateRayTracerFromName(
      rayTracerName,
      scene,
      toneMapOptions,
      rayMatterState,
      bidirectionalPathState,
      stochasticRayTracingState,
      lightList
    );

    if (rayTracer !== null) {
      rayTracer.defaults();
    }
    return rayTracer;
  }

  public static rayTraceSaveImage(
    fileName: string,
    stream: OutputStream | null,
    isPipe: number,
    scene: Scene,
    rayTracer: RayTracer | null
  ): void {
    if (stream === null) {
      return;
    }
    if (scene.camera === null) {
      return;
    }

    const t = process.hrtime.bigint();

    const img = ImageOutputHandle.createRadianceImageOutputHandle(
      fileName,
      stream,
      isPipe,
      scene.camera.xSize,
      scene.camera.ySize
    );
    if (img === null) {
      return;
    }

    if (rayTracer === null) {
      VsdkLogger.warning(null, "No ray tracing method active");
    }
    else if (!rayTracer.saveImage(img)) {
      VsdkLogger.warning(null, "No previous %s image available", rayTracer.getName());
    }

    ImageOutputHandle.deleteImageOutputHandle(img);

    process.stdout.write(
      `Raytrace save image: ${Number(process.hrtime.bigint() - t) / 1_000_000_000.0} secs.\n`
    );
  }

  public static rayTraceParseOptions(argc: number[], argv: string[], rayTracerName: string[]): void {
    OptionsGroupRaytracing.parse(argc, argv, rayTracerName);
  }

  public static rayTraceExecute(
    fileName: string | null,
    stream: OutputStream | null,
    isPipe: number,
    scene: Scene,
    radianceMethod: RadianceMethod,
    rayTracer: RayTracer | null,
    toneMapOptions: ToneMappingContext,
    renderOptions: RenderOptions
  ): void {
    renderOptions.renderRayTracedImage = true;
    if (scene.camera !== null) {
      scene.camera.changed = 0;
    }

    Canvas.canvasPushMode();
    RayTracer.rayTrace(
      fileName ?? "",
      stream,
      isPipe,
      rayTracer,
      scene,
      radianceMethod,
      toneMapOptions,
      renderOptions
    );
    Canvas.canvasPullMode();
  }
}
