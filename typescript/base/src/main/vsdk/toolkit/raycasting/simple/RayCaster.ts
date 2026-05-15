/**
Ray casting using the SGL library for rendering Patch pointers into
a software frame buffer directly.
*/

import { ArrayList } from "../../../../java/util/ArrayList";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Ray } from "../../common/linealAlgebra/Ray";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Statistics } from "../../common/statistics/Statistics";
import { ImageOutputHandle } from "../../io/image/ImageOutputHandle";
import { RayTracer } from "../common/RayTracer";
import { ScreenBuffer } from "../../render/ScreenBuffer";
import { SoftIdsWrapper } from "../../render/SoftIdsWrapper";
import { Camera } from "../../scene/Camera";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { Scene } from "../../scene/Scene";
import { Patch } from "../../environment/geometry/elements/Patch";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";

export class RayCaster extends RayTracer {
  private static rayCaster: RayCaster | null = null;
  private static readonly NAME = "Ray Casting";
  private screenBuffer: ScreenBuffer;
  private doDeleteScreen: boolean;

  public constructor(inScreen: ScreenBuffer | null, defaultCamera: Camera | null, toneMapOptions: ToneMappingContext) {
    super();
    if (defaultCamera === null) {
      VsdkLogger.fatal(-1, "RayCaster::constructor", "Default camera not set");
    }

    if (inScreen === null) {
      this.screenBuffer = new ScreenBuffer(null, defaultCamera, toneMapOptions);
      this.doDeleteScreen = true;
    }
    else {
      this.screenBuffer = inScreen;
      this.screenBuffer.setToneMappingContext(toneMapOptions);
      this.doDeleteScreen = true;
    }
  }

  public override defaults(): void {
  }

  public override getName(): string {
    return RayCaster.NAME;
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
    if (RayCaster.rayCaster !== null) {
      RayCaster.rayCaster = null;
    }
    RayCaster.rayCaster = new RayCaster(null, scene.camera, toneMapOptions);
    RayCaster.rayCaster.render(scene, radianceMethod, toneMapOptions, renderOptions);
    if (RayCaster.rayCaster !== null && ip !== null) {
      RayCaster.rayCaster.save(ip);
    }
  }

  public override saveImage(imageOutputHandle: ImageOutputHandle): boolean {
    if (RayCaster.rayCaster === null) {
      return false;
    }

    RayCaster.rayCaster.save(imageOutputHandle);
    return true;
  }

  public override terminate(): void {
    RayCaster.rayCaster = null;
  }

  private static clipUv(numberOfVertices: number, u: number[], v: number[]): void {
    if (u[0] > 1.0 - Numeric.EPSILON) {
      u[0] = 1.0 - Numeric.EPSILON;
    }
    if (v[0] > 1.0 - Numeric.EPSILON) {
      v[0] = 1.0 - Numeric.EPSILON;
    }
    if (numberOfVertices === 3 && (u[0] + v[0]) > 1.0 - Numeric.EPSILON) {
      if (u[0] > v[0]) {
        u[0] = 1.0 - v[0] - Numeric.EPSILON;
      }
      else {
        v[0] = 1.0 - u[0] - Numeric.EPSILON;
      }
    }
    if (u[0] < Numeric.EPSILON) {
      u[0] = Numeric.EPSILON;
    }
    if (v[0] < Numeric.EPSILON) {
      v[0] = Numeric.EPSILON;
    }
  }

  /**
  Determines the radiance of the nearest patch visible through the pixel
  (x,y). P shall be the nearest patch visible in the pixel.
  */
  private getRadianceAtPixel(
    camera: Camera,
    x: number,
    y: number,
    patch: Patch,
    radianceMethod: RadianceMethod | null,
    renderOptions: RendererConfiguration
  ): ColorRgb {
    const radiance = new ColorRgb();
    radiance.clear();

    if (radianceMethod !== null) {
      // Ray pointing from the eye through the center of the pixel.
      const ray = new Ray();
      ray.position = camera.eyePosition;
      ray.direction = this.screenBuffer.getPixelVector(x, y);
      ray.direction.normalize(Numeric.EPSILON_FLOAT);

      // Find intersection point of ray with patch
      const point = new Vector3D();
      let dist = patch.normal.dotProduct(ray.direction);
      dist = -(patch.normal.dotProduct(ray.position) + patch.planeConstant) / dist;
      point.sumScaled(ray.position, dist, ray.direction);

      // Find surface coordinates of hit point on patch
      const u = [0.0];
      const v = [0.0];
      patch.uv(point, u, v);

      // Boundary check is necessary because Z-buffer algorithm does
      // not yield exactly the same result as ray tracing at patch boundaries.
      RayCaster.clipUv(patch.numberOfVertices, u, v);

      // Reverse ray direction and get radiance emitted at hit point towards the eye
      const dir = new Vector3D(-ray.direction.x, -ray.direction.y, -ray.direction.z);
      return radianceMethod.getRadiance(camera, patch, u[0], v[0], dir, renderOptions);
    }
    return radiance;
  }

  public render(
    scene: Scene,
    radianceMethod: RadianceMethod | null,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void {
    if (scene.camera === null) {
      VsdkLogger.fatal(-1, "RayCaster::render", "Scene camera not set");
      return;
    }

    this.screenBuffer.setToneMappingContext(toneMapOptions);
    const t = process.hrtime.bigint();

    const idRenderer = new SoftIdsWrapper(scene, renderOptions);

    const width = [0];
    const height = [0];
    idRenderer.getSize(width, height);
    if (width[0] !== this.screenBuffer.getHRes() || height[0] !== this.screenBuffer.getVRes()) {
      VsdkLogger.fatal(-1, "RayCaster::render", "ID buffer size doesn't match screen size");
    }

    // This is the main loop for ray-casting
    for (let y = 0; y < height[0]; y++) {
      for (let x = 0; x < width[0]; x++) {
        const patch = idRenderer.getPatchAtPixel(x, y);
        if (patch !== null) {
          const rad = this.getRadianceAtPixel(scene.camera, x, y, patch, radianceMethod, renderOptions);
          this.screenBuffer.add(x, y, rad);
        }
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
