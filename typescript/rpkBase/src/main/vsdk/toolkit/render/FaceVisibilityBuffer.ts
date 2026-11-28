import { PolygonVisibilityBuffer } from "vitral/dist/vsdk/toolkit/render/raster/polygonScanConverter/PolygonVisibilityBuffer";

/** A visible face in the scene's current face table. The default is background. */
export class FaceIndexSample {
  private readonly index: number;

  public constructor(faceIndex = -1) {
    this.index = faceIndex;
  }

  public isBackground(): boolean {
    return this.index < 0;
  }

  public getIndex(): number {
    return this.index;
  }
}

export type FaceVisibilityBuffer = PolygonVisibilityBuffer<FaceIndexSample>;
