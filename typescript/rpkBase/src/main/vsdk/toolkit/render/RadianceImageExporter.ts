import { OutputStream } from "vitral/dist/java/io/OutputStream";
import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { Logger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { RendererConfiguration } from "../material/RendererConfiguration";
import { Numeric } from "../skin/Numeric";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { ImageOutputTarget } from "./ImageOutputTarget";
import { Camera } from "../scene/Camera";
import { RadianceMethod } from "../scene/RadianceMethod";
import { Scene } from "../scene/Scene";
import { Patch } from "../environment/geometry/elements/Patch";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";
import { ScreenBuffer } from "./ScreenBuffer";
import { SoftIdsWrapper } from "./SoftIdsWrapper";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

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
      Logger.reportMessage(null, Logger.ERROR, "RadianceImageExporter::exportImage", "Tone mapping context not provided for image export");
      return;
    }

    const screenBuffer = new ScreenBuffer(null, scene.camera, toneMapOptions);
    const idRenderer = new SoftIdsWrapper(scene, renderOptions);

    const width = [0];
    const height = [0];
    idRenderer.getSize(width, height);
    if (width[0] !== screenBuffer.getHRes() || height[0] !== screenBuffer.getVRes()) {
      Logger.reportMessage(null, Logger.ERROR, "RadianceImageExporter::exportImage", "ID buffer size does not match screen size");
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

    const imageOutputTarget = ImageOutputTarget.create(fileName, outputStream, isPipe);
    if (imageOutputTarget === null) {
      return;
    }

    screenBuffer.writeFile(imageOutputTarget);
  }

  private static clipUv(numberOfVertices: number, u: number[], v: number[]): void {
    if (u[0]! > 1.0 - VSDK.EPSILON) {
      u[0]! = 1.0 - VSDK.EPSILON;
    }
    if (v[0]! > 1.0 - VSDK.EPSILON) {
      v[0]! = 1.0 - VSDK.EPSILON;
    }
    if (numberOfVertices === 3 && (u[0]! + v[0]!) > 1.0 - VSDK.EPSILON) {
      if (u[0]! > v[0]!) {
        u[0]! = 1.0 - v[0]! - VSDK.EPSILON;
      }
      else {
        v[0]! = 1.0 - u[0]! - VSDK.EPSILON;
      }
    }
    if (u[0]! < VSDK.EPSILON) {
      u[0]! = VSDK.EPSILON;
    }
    if (v[0]! < VSDK.EPSILON) {
      v[0]! = VSDK.EPSILON;
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
  ): ColorRgbMutable {
    const radiance = new ColorRgbMutable();
    radiance.clear();

    if (screenBuffer === null || camera === null || patch === null || radianceMethod === null || renderOptions === null) {
      return radiance;
    }

    let rayDirection = screenBuffer.getPixelVector(x, y);
    rayDirection = rayDirection.normalized(Numeric.EPSILON);

    const denominator = patch.getNormal().dotProduct(rayDirection);
    if (denominator <= Numeric.EPSILON && denominator >= -Numeric.EPSILON) {
      return radiance;
    }

    const distance = -(patch.getNormal().dotProduct(camera.eyePosition) + patch.getPlaneConstant()) / denominator;
    let hitPoint = new Vector3Dd();
    hitPoint = camera.eyePosition.add(rayDirection.multiply(distance));

    const u = [0.0];
    const v = [0.0];
    patch.uv(hitPoint, u, v);
    RadianceImageExporter.clipUv(patch.numberOfVertices, u, v);

    const eyeDirection = new Vector3Dd(-rayDirection.x(), -rayDirection.y(), -rayDirection.z());
    return radianceMethod.getRadiance(camera, patch, u[0]!, v[0]!, eyeDirection, renderOptions);
  }
}
