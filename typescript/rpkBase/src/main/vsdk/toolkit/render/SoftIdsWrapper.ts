import { RendererConfiguration } from "../material/RendererConfiguration";
import { SglContext } from "../render/sgl/SglContext";
import { Scene } from "../scene/Scene";
import { Patch } from "../environment/geometry/elements/Patch";
import { SoftIds } from "./SoftIds";

export class SoftIdsWrapper {
  private sgl: SglContext;

  private init(scene: Scene, renderOptions: RendererConfiguration): void {
    if (scene.camera === null) {
      this.sgl = new SglContext(1, 1);
      return;
    }

    this.sgl = SoftIds.setupSoftFrameBuffer(scene.camera);
    SoftIds.softRenderPatches(scene, renderOptions, this.sgl);
  }

  public constructor(scene: Scene, renderOptions: RendererConfiguration) {
    this.sgl = new SglContext(1, 1);
    this.init(scene, renderOptions);
  }

  public getSize(width: number[] | null, height: number[] | null): void {
    if (width !== null && width.length > 0) {
      width[0] = this.sgl.width;
    }
    if (height !== null && height.length > 0) {
      height[0] = this.sgl.height;
    }
  }

  public getPatchAtPixel(x: number, y: number): Patch | null {
    const index = (this.sgl.height - 1 - y) * this.sgl.width + x;
    return this.sgl.patchBuffer[index] ?? null;
  }
}
