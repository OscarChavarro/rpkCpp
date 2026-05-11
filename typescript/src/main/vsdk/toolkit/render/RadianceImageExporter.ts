import { OutputStream } from "../../../java/io/OutputStream";
import { ColorRgb } from "../common/color/ColorRgb";
import { Logger } from "../common/logging/Logger";
import { RendererConfiguration } from "../material/RendererConfiguration";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { ImageOutputHandle } from "../io/image/ImageOutputHandle";
import { Camera } from "../scene/Camera";
import { RadianceMethod } from "../scene/RadianceMethod";
import { Scene } from "../scene/Scene";
import { Patch } from "../environment/geometry/elements/Patch";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";
import { ScreenBuffer } from "./ScreenBuffer";
import { SoftIdsWrapper } from "./SoftIdsWrapper";

export class RadianceImageExporter {
  private constructor() {
  }

  public static exportImage(
    fileName: string,
    outputStream: OutputStream | null,
    isPipe: number,
    scene: Scene | null,
    radianceMethod: RadianceMethod,
    toneMapOptions: ToneMappingContext | null,
    renderOptions: RendererConfiguration
  ): void {
    if (outputStream === null || scene === null || scene.camera === null) {
      return;
    }

    if (toneMapOptions === null) {
      Logger.error("RadianceImageExporter::exportImage", "Tone mapping context not provided for image export");
      return;
    }

    const screenBuffer = new ScreenBuffer(null, scene.camera, toneMapOptions);
    const idRenderer = new SoftIdsWrapper(scene, renderOptions);

    const width = [0];
    const height = [0];
    idRenderer.getSize(width, height);
    if (width[0] !== screenBuffer.getHRes() || height[0] !== screenBuffer.getVRes()) {
      Logger.error("RadianceImageExporter::exportImage", "ID buffer size does not match screen size");
      return;
    }

    for (let y = 0; y < height[0]; y++) {
      for (let x = 0; x < width[0]; x++) {
        const patch = idRenderer.getPatchAtPixel(x, y);
        if (patch !== null) {
          const radiance = RadianceImageExporter.getRadianceAtPixel(
            screenBuffer,
            scene.camera,
            x,
            y,
            patch,
            radianceMethod,
            renderOptions
          );
          screenBuffer.add(x, y, radiance);
        }
      }
    }

    const imageOutputHandle = ImageOutputHandle.createRadianceImageOutputHandle(
      fileName,
      outputStream,
      isPipe,
      scene.camera.xSize,
      scene.camera.ySize
    );
    if (imageOutputHandle === null) {
      return;
    }

    screenBuffer.writeFile(imageOutputHandle);
    ImageOutputHandle.deleteImageOutputHandle(imageOutputHandle);
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

  private static getRadianceAtPixel(
    screenBuffer: ScreenBuffer | null,
    camera: Camera | null,
    x: number,
    y: number,
    patch: Patch | null,
    radianceMethod: RadianceMethod | null,
    renderOptions: RendererConfiguration | null
  ): ColorRgb {
    const radiance = new ColorRgb();
    radiance.clear();

    if (screenBuffer === null || camera === null || patch === null || radianceMethod === null || renderOptions === null) {
      return radiance;
    }

    const rayDirection = screenBuffer.getPixelVector(x, y);
    rayDirection.normalize(Numeric.EPSILON_FLOAT);

    const denominator = patch.normal.dotProduct(rayDirection);
    if (denominator <= Numeric.EPSILON_FLOAT && denominator >= -Numeric.EPSILON_FLOAT) {
      return radiance;
    }

    const distance = -(patch.normal.dotProduct(camera.eyePosition) + patch.planeConstant) / denominator;
    const hitPoint = new Vector3D();
    hitPoint.sumScaled(camera.eyePosition, distance, rayDirection);

    const u = [0.0];
    const v = [0.0];
    patch.uv(hitPoint, u, v);
    RadianceImageExporter.clipUv(patch.numberOfVertices, u, v);

    const eyeDirection = new Vector3D(-rayDirection.x, -rayDirection.y, -rayDirection.z);
    return radianceMethod.getRadiance(camera, patch, u[0], v[0], eyeDirection, renderOptions);
  }
}
