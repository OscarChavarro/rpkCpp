/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/
import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { RendererConfiguration } from "../material/RendererConfiguration";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { PolygonVisibilityBuffer } from "vitral/dist/vsdk/toolkit/render/raster/polygonScanConverter/PolygonVisibilityBuffer";
import { Camera } from "../scene/Camera";
import { Scene } from "../scene/Scene";
import { Patch } from "../environment/geometry/elements/Patch";
import { ToneMap } from "../tonemap/ToneMap";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";
import { FaceIndexSample, type FaceVisibilityBuffer } from "./FaceVisibilityBuffer";

export class SoftIds {
  private constructor() {
  }

  private static drawPatch(
    patch: Patch,
    camera: Camera,
    renderOptions: RendererConfiguration,
    target: FaceVisibilityBuffer,
    vertices: Vector3Dd[],
  ): void {
    if (renderOptions.backfaceCulling &&
      patch.getNormal().dotProduct(camera.eyePosition) + patch.getPlaneConstant() < VSDK.EPSILON) {
      return;
    }

    const count = patch.numberOfVertices;
    for (let i = 0; i < count; i++) {
      vertices[i] = patch.vertex[i]!.point;
    }
    target.drawPolygon(count, vertices, new FaceIndexSample(patch.getFaceIndex()));
  }

  /** Renders into a cleared target with the camera's dimensions and full viewport. */
  public static renderVisibleFaces(scene: Scene, renderOptions: RendererConfiguration | null, target: FaceVisibilityBuffer): void {
    if (renderOptions === null) {
      return;
    }

    const camera = scene.camera!;
    const projection = Camera.perspectiveMatrix(
      camera.fieldOfVision * 2.0 * globalThis.Math.PI / 180.0,
      camera.xSize / camera.ySize,
      camera.near,
      camera.far,
    );
    const view = Camera.lookAtMatrix(camera.eyePosition, camera.lookPosition, camera.upDirection);
    target.setTransform(projection.multiply(view));

    // TypeScript: the vertices of the patch drawn, reused from one patch to the next
    const vertices = new Array<Vector3Dd>(4);
    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      const patch = scene.patchList[i];
      if (patch !== null && patch !== undefined) {
        SoftIds.drawPatch(patch, camera, renderOptions, target, vertices);
      }
    }
  }

  /**
  Legacy patch ID export. Zero represents the background.
  */
  public static softRenderIds(width: number[], height: number[], scene: Scene, renderOptions: RendererConfiguration | null): number[] {
    const target: FaceVisibilityBuffer = new PolygonVisibilityBuffer<FaceIndexSample>(
      scene.camera!.xSize, scene.camera!.ySize, new FaceIndexSample());
    SoftIds.renderVisibleFaces(scene, renderOptions, target);

    width[0] = target.getWidth();
    height[0] = target.getHeight();
    const ids = new Array<number>(width[0] * height[0]);
    for (let y = 0; y < height[0]; y++) {
      const rowStart = y * width[0];
      for (let x = 0; x < width[0]; x++) {
        const face = target.getSample(x, y);
        ids[rowStart + x] = face.isBackground() ? 0 : scene.getFace(face.getIndex()).id;
      }
    }
    return ids;
  }

  /**
  Renders in memory an image of m lines of n pixels at column x on row y (= lower
  left corner of image, relative to the lower left corner of the window)
  */
  public static softRenderPixels(width: number, height: number, rgb: ColorRgbMutable[], toneMapOptions: ToneMappingContext): void {
    const rowLength = ((4 * width * Uint8Array.BYTES_PER_ELEMENT + 7) & ~7);
    const c = new Uint8Array(height * rowLength + 8);

    for (let j = 0; j < height; j++) {
      const rowRgbStart = j * width;
      const rowStart = j * rowLength;
      for (let i = 0; i < width; i++) {
        const correctedRgb = new ColorRgbMutable(
          rgb[rowRgbStart + i]!.r,
          rgb[rowRgbStart + i]!.g,
          rgb[rowRgbStart + i]!.b
        );
        ToneMap.toneMappingGammaCorrection(correctedRgb, toneMapOptions);
        const pixelOffset = rowStart + 4 * i;
        c[pixelOffset] = globalThis.Math.trunc(correctedRgb.r * 255.0) & 0xFF;
        c[pixelOffset + 1] = globalThis.Math.trunc(correctedRgb.g * 255.0) & 0xFF;
        c[pixelOffset + 2] = globalThis.Math.trunc(correctedRgb.b * 255.0) & 0xFF;
        c[pixelOffset + 3] = 255;
      }
    }
  }
}
