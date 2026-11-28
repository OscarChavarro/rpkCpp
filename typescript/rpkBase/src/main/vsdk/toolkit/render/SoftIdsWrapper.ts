import { PolygonVisibilityBuffer } from "vitral/dist/vsdk/toolkit/render/raster/polygonScanConverter/PolygonVisibilityBuffer";
import { RendererConfiguration } from "../material/RendererConfiguration";
import { Scene } from "../scene/Scene";
import { Patch } from "../environment/geometry/elements/Patch";
import { FaceIndexSample, type FaceVisibilityBuffer } from "./FaceVisibilityBuffer";
import { SoftIds } from "./SoftIds";

export class SoftIdsWrapper {
  private readonly scene: Scene;
  private readonly target: FaceVisibilityBuffer;

  public constructor(inScene: Scene, renderOptions: RendererConfiguration | null) {
    this.scene = inScene;
    this.target = new PolygonVisibilityBuffer<FaceIndexSample>(
      this.scene.camera!.xSize, this.scene.camera!.ySize, new FaceIndexSample());
    SoftIds.renderVisibleFaces(this.scene, renderOptions, this.target);
  }

  public getSize(width: number[] | null, height: number[] | null): void {
    if (width !== null && width.length > 0) {
      width[0] = this.target.getWidth();
    }
    if (height !== null && height.length > 0) {
      height[0] = this.target.getHeight();
    }
  }

  public getPatchAtPixel(x: number, y: number): Patch | null {
    const face = this.target.getSample(x, this.target.getHeight() - 1 - y);
    return face.isBackground() ? null : this.scene.getFace(face.getIndex());
  }
}
