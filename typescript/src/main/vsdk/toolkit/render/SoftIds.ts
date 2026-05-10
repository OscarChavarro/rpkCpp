import { ColorRgb } from "../common/color/ColorRgb";
import { RenderOptions } from "../common/RenderOptions";
import { Matrix4x4 } from "../common/linealAlgebra/Matrix4x4";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { SglConstants } from "../render/sgl/SglConstants";
import { SglContext } from "../render/sgl/SglContext";
import { Camera } from "../scene/Camera";
import { Scene } from "../scene/Scene";
import { Patch } from "../skin/Patch";
import { ToneMap } from "../tonemap/ToneMap";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";

export class SoftIds {
  private constructor() {
  }

  public static setupSoftFrameBuffer(camera: Camera): SglContext {
    const sgl = new SglContext(camera.xSize, camera.ySize);
    sgl.sglDepthTesting(true);
    sgl.sglClipping(true);
    sgl.sglClear(0, SglConstants.SGL_MAXIMUM_Z);

    const p = Matrix4x4.createPerspectiveMatrix(
      camera.fieldOfVision * 2.0 * globalThis.Math.PI / 180.0,
      camera.xSize / camera.ySize,
      camera.near,
      camera.far
    );
    sgl.sglLoadMatrix(p);
    const l = Matrix4x4.createLookAtMatrix(camera.eyePosition, camera.lookPosition, camera.upDirection);
    sgl.sglMultiplyMatrix(l);

    return sgl;
  }

  public static softRenderPatch(
    patch: Patch | null,
    camera: Camera | null,
    renderOptions: RenderOptions | null,
    sglContext: SglContext | null
  ): void {
    if (patch === null || camera === null || renderOptions === null || sglContext === null) {
      return;
    }

    const vertices = new Array<Vector3D>(4);

    if (renderOptions.backfaceCulling &&
      patch.normal.dotProduct(camera.eyePosition) + patch.planeConstant < Numeric.EPSILON) {
      return;
    }

    vertices[0] = patch.vertex[0]!.point;
    vertices[1] = patch.vertex[1]!.point;
    vertices[2] = patch.vertex[2]!.point;
    if (patch.numberOfVertices > 3) {
      vertices[3] = patch.vertex[3]!.point;
    }

    sglContext.sglSetPatch(patch);
    sglContext.sglPolygon(patch.numberOfVertices, vertices);
  }

  public static softRenderPatches(scene: Scene | null, renderOptions: RenderOptions | null, sglContext: SglContext | null): void {
    if (scene === null || renderOptions === null || sglContext === null) {
      return;
    }

    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      SoftIds.softRenderPatch(scene.patchList[i], scene.camera, renderOptions, sglContext);
    }
  }

  public static softRenderIds(x: number[] | null, y: number[] | null, scene: Scene, renderOptions: RenderOptions): number[] {
    const camera = scene.camera;
    if (camera === null) {
      if (x !== null && x.length > 0) {
        x[0] = 0;
      }
      if (y !== null && y.length > 0) {
        y[0] = 0;
      }
      return [];
    }

    const currentSglContext = SoftIds.setupSoftFrameBuffer(camera);
    SoftIds.softRenderPatches(scene, renderOptions, currentSglContext);

    if (x !== null && x.length > 0) {
      x[0] = currentSglContext.width;
    }
    if (y !== null && y.length > 0) {
      y[0] = currentSglContext.height;
    }

    const ids = new Array<number>(currentSglContext.width * currentSglContext.height);
    for (let i = 0; i < ids.length; i++) {
      ids[i] = currentSglContext.frameBuffer[i];
    }
    return ids;
  }

  public static softRenderPixels(width: number, height: number, rgb: ColorRgb[], toneMapOptions: ToneMappingContext): void {
    const rowLength = ((4 * width * Uint8Array.BYTES_PER_ELEMENT + 7) & ~7);
    const c = new Uint8Array(height * rowLength + 8);

    for (let j = 0; j < height; j++) {
      const rowRgbStart = j * width;
      const rowStart = j * rowLength;
      for (let i = 0; i < width; i++) {
        const correctedRgb = new ColorRgb(
          rgb[rowRgbStart + i].r,
          rgb[rowRgbStart + i].g,
          rgb[rowRgbStart + i].b
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
