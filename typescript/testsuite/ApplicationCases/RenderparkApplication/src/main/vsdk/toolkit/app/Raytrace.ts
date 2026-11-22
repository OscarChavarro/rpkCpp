import { String as JavaString } from "vitral/dist/java/lang/String";
import { OutputStream } from "vitral/dist/java/io/OutputStream";
import { System } from "rpk-base/dist/java/lang/System";
import { ArrayList } from "vitral/dist/java/util/ArrayList";
import { OptionsGroupRaytracing } from "./options/OptionsGroupRaytracing";
import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { RendererConfiguration } from "rpk-base/dist/vsdk/toolkit/material/RendererConfiguration";
import { ImageOutputHandle } from "rpk-base/dist/vsdk/toolkit/io/image/ImageOutputHandle";
import { BidirectionalPathRaytracer } from "rpk-base/dist/vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathRaytracer";
import { BidirectionalPathTracingState } from "rpk-base/dist/vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState";
import { LightList } from "rpk-base/dist/vsdk/toolkit/raycasting/bidirectionalRaytracing/LightList";
import { RayTracer } from "rpk-base/dist/vsdk/toolkit/raycasting/common/RayTracer";
import { RayCaster } from "rpk-base/dist/vsdk/toolkit/raycasting/simple/RayCaster";
import { RayMatter } from "rpk-base/dist/vsdk/toolkit/raycasting/simple/RayMatter";
import { RayMatterState } from "rpk-base/dist/vsdk/toolkit/raycasting/simple/RayMatterState";
import { StochasticRaytracer } from "rpk-base/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRaytracer";
import { StochasticRayTracingState } from "rpk-base/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRayTracingState";
import { Canvas } from "rpk-base/dist/vsdk/toolkit/render/Canvas";
import { RadianceMethod } from "rpk-base/dist/vsdk/toolkit/scene/RadianceMethod";
import { Scene } from "rpk-base/dist/vsdk/toolkit/scene/Scene";
import { Patch } from "rpk-base/dist/vsdk/toolkit/environment/geometry/elements/Patch";
import { ToneMappingContext } from "rpk-base/dist/vsdk/toolkit/tonemap/ToneMappingContext";

export class Raytrace {
  private constructor() {
  }

  private static toArrayList(input: Patch[] | null): ArrayList<Patch> {
    const out = new ArrayList<Patch>();
    for (let i = 0; input !== null && i < input.length; i++) {
      out.add(input[i]!);
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
      newRaytracer = new BidirectionalPathRaytracer(bidirectionalPathState, lightList === null ? null : lightList[0]!);
    }
    else if (rayTracerName === "StochasticRaytracing") {
      newRaytracer = new StochasticRaytracer(lightList === null ? null : lightList[0]!, stochasticRayTracingState);
    }
    else {
      newRaytracer = null;
    }

    Raytrace.rayTraceSetMethod(newRaytracer, scene.lightSourcePatchList, lightList);

    if (newRaytracer === null && (rayTracerName === null || rayTracerName.substring(0, 4).toLowerCase() !== "none")) {
      VsdkLogger.reportMessage(null, VsdkLogger.ERROR, "", JavaString.vformat("Invalid raytracing method name '%s'", [rayTracerName]));
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
      VsdkLogger.reportMessage(null, VsdkLogger.WARNING, "", "No ray tracing method active");
    }
    else if (!rayTracer.saveImage(img)) {
      VsdkLogger.reportMessage(null, VsdkLogger.WARNING, "", JavaString.vformat("No previous %s image available", [rayTracer.getName()]));
    }

    ImageOutputHandle.deleteImageOutputHandle(img);

    System.out.printf(
      "Raytrace save image: %g secs.\n",
      Number(process.hrtime.bigint() - t) / 1_000_000_000.0
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
    renderOptions: RendererConfiguration
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
