import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { RendererConfiguration } from "../material/RendererConfiguration";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { SglConstants } from "../render/sgl/SglConstants";
import { SglContext } from "../render/sgl/SglContext";
import { Camera } from "../scene/Camera";
import { Scene } from "../scene/Scene";
import { Patch } from "../environment/geometry/elements/Patch";
import { ToneMap } from "../tonemap/ToneMap";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class SoftIds {
  private constructor() {
  }

  public static setupSoftFrameBuffer(camera: Camera): SglContext {
    const sgl = new SglContext(camera.xSize, camera.ySize);
    sgl.sglDepthTesting(true);
    sgl.sglClipping(true);
    sgl.sglClear(0, SglConstants.SGL_MAXIMUM_Z);
    const p = Camera.perspectiveMatrix(
      ((camera.fieldOfVision * 2.0) * globalThis.Math.PI) / 180.0,
      camera.xSize / camera.ySize,
      camera.near,
      camera.far
    );
    sgl.sglLoadMatrix(p);
    const l = Camera.lookAtMatrix(camera.eyePosition, camera.lookPosition, camera.upDirection);
    sgl.sglMultiplyMatrix(l);

    return sgl;
  }

  public static softRenderPatch(
    patch: Patch | null,
    camera: Camera | null,
    renderOptions: RendererConfiguration | null,
    sglContext: SglContext | null
  ): void {
    if (patch === null || camera === null || renderOptions === null || sglContext === null) {
      return;
    }

    const vertices = new Array<Vector3Dd>(4);

    if (renderOptions.backfaceCulling &&
      patch.getNormal().dotProduct(camera.eyePosition) + patch.getPlaneConstant() < VSDK.EPSILON) {
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

  public static softRenderPatches(scene: Scene | null, renderOptions: RendererConfiguration | null, sglContext: SglContext | null): void {
    if (scene === null || renderOptions === null || sglContext === null) {
      return;
    }

    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      SoftIds.softRenderPatch(scene.patchList[i]!, scene.camera, renderOptions, sglContext);
    }
  }

  public static softRenderIds(x: number[] | null, y: number[] | null, scene: Scene, renderOptions: RendererConfiguration): number[] {
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
      const patch = currentSglContext.patchBuffer[i];
      ids[i] = patch == null ? 0 : patch.id;
    }
    return ids;
  }

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
