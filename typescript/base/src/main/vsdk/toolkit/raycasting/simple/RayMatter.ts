/**
Original version by Vincent Masselus adapted by Pieter Peers (2001-06-01)
*/

import { ArrayList } from "../../../../java/util/ArrayList";
import { ColorRgb } from "../../common/color/ColorRgb";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Ray } from "../../common/linealAlgebra/Ray";
import { Statistics } from "../../common/statistics/Statistics";
import { ImageOutputHandle } from "../../io/image/ImageOutputHandle";
import { BoxFilter } from "../common/BoxFilter";
import { NormalFilter } from "../common/NormalFilter";
import { PixelFilter } from "../common/PixelFilter";
import { RayTools } from "../common/RayTools";
import { RayTracer } from "../common/RayTracer";
import { TentFilter } from "../common/TentFilter";
import { ScreenBuffer } from "../../render/ScreenBuffer";
import { Camera } from "../../scene/Camera";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { Scene } from "../../scene/Scene";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { Patch } from "../../environment/geometry/elements/Patch";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { RayMatterFilterType } from "./RayMatterFilterType";
import { RayMatterState } from "./RayMatterState";

export class RayMatter extends RayTracer {
  private static rayMatter: RayMatter | null = null;
  private static readonly NAME = "Ray Matting";
  private screenBuffer: ScreenBuffer;
  private pixelFilter: PixelFilter | null;
  private doDeleteScreen: boolean;
  private rayMatterState: RayMatterState;

  public constructor(
    screen: ScreenBuffer | null,
    camera: Camera | null,
    inRayMatterState: RayMatterState,
    toneMapOptions: ToneMappingContext
  ) {
    super();
    this.rayMatterState = inRayMatterState;

    if (screen === null) {
      this.screenBuffer = new ScreenBuffer(null, camera, toneMapOptions);
      this.doDeleteScreen = false;
    }
    else {
      this.screenBuffer = screen;
      this.screenBuffer.setToneMappingContext(toneMapOptions);
      this.doDeleteScreen = false;
    }

    this.pixelFilter = null;
    this.screenBuffer.setRgbImage(true);
  }

  public override defaults(): void {
    // Defaults are owned by the caller-provided RayMatterState instance.
  }

  public override getName(): string {
    return RayMatter.NAME;
  }

  public override initialize(lightPatches: ArrayList<Patch>): void {
    void lightPatches;
  }

  public override execute(
    ip: ImageOutputHandle | null,
    scene: Scene,
    radianceMethod: RadianceMethod,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void {
    void radianceMethod;
    void renderOptions;

    if (RayMatter.rayMatter !== null) {
      RayMatter.rayMatter = null;
    }
    RayMatter.rayMatter = new RayMatter(
      null,
      scene.camera,
      this.rayMatterState,
      toneMapOptions
    );
    RayMatter.rayMatter.doMatting(scene.camera, scene.voxelGrid);
    if (ip !== null && RayMatter.rayMatter !== null) {
      RayMatter.rayMatter.save(ip);
    }
  }

  public override saveImage(imageOutputHandle: ImageOutputHandle): boolean {
    if (RayMatter.rayMatter === null) {
      return false;
    }

    RayMatter.rayMatter.save(imageOutputHandle);
    return true;
  }

  public override terminate(): void {
    RayMatter.rayMatter = null;
  }

  public createFilter(): void {
    if (this.pixelFilter !== null) {
      this.pixelFilter = null;
    }

    if (this.rayMatterState.filter === RayMatterFilterType.BOX_FILTER) {
      this.pixelFilter = new BoxFilter();
    }
    if (this.rayMatterState.filter === RayMatterFilterType.TENT_FILTER) {
      this.pixelFilter = new TentFilter();
    }
    if (this.rayMatterState.filter === RayMatterFilterType.GAUSS_FILTER) {
      this.pixelFilter = new NormalFilter();
    }
    if (this.rayMatterState.filter === RayMatterFilterType.GAUSS2_FILTER) {
      this.pixelFilter = new NormalFilter(0.5, 1.5);
    }
  }

  public doMatting(camera: Camera | null, sceneWorldVoxelGrid: VoxelGrid | null): void {
    if (camera === null || sceneWorldVoxelGrid === null) {
      return;
    }

    const t = process.hrtime.bigint();

    this.createFilter();

    // Main loop for ray matter
    for (let y = 0; y < camera.ySize; y++) {
      for (let x = 0; x < camera.xSize; x++) {
        let hits = 0.0;

        for (let i = 0; i < this.rayMatterState.samplesPerPixel; i++) {
          // Uniform random var
          const dx = [globalThis.Math.random()];
          const dy = [globalThis.Math.random()];

          // Insert non-uniform sampling here
          if (this.pixelFilter !== null) {
            this.pixelFilter.sample(dx, dy);
          }

          // Generate ray
          const ray = new Ray();
          ray.position = camera.eyePosition;
          ray.direction = this.screenBuffer.getPixelVector(x, y, dx[0] ?? 0.0, dy[0] ?? 0.0);
          ray.direction.normalize(Numeric.EPSILON_FLOAT);

          // Check if hit
          if (RayTools.findRayIntersection(sceneWorldVoxelGrid, ray, null, null, null) !== null) {
            hits++;
          }
        }

        // Add matte value to screen buffer
        let value = hits / this.rayMatterState.samplesPerPixel;
        if (value > 1.0) {
          value = 1.0;
        }

        const matte = new ColorRgb();
        matte.set(value, value, value);
        this.screenBuffer.add(x, y, matte);
      }

      this.screenBuffer.renderScanline(y);
    }

    Statistics.instance().rayTracer.totalTime = Number(process.hrtime.bigint() - t) / 1_000_000_000.0;
    Statistics.instance().rayTracer.rayCount = 0;
    Statistics.instance().rayTracer.pixelCount = 0;
  }

  public display(): void {
    this.screenBuffer.render();
  }

  public save(ip: ImageOutputHandle): void {
    this.screenBuffer.writeFile(ip);
  }
}
