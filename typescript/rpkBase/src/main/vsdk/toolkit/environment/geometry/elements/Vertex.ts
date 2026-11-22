import { ColorRgbMutable } from "../../../common/color/ColorRgbMutable";
import type { FaceColorTable } from "../../../scene/FaceColorTable";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { Statistics } from "../../../common/statistics/Statistics";
import { VertexCompareFlags } from "./VertexCompareFlags";
import type { Element } from "./Element";
import type { Patch } from "./Patch";

export class Vertex {
  private static currentComparisonFlags = VertexCompareFlags.VERTEX_COMPARE_LOCATION
    | VertexCompareFlags.VERTEX_COMPARE_NORMAL
    | VertexCompareFlags.VERTEX_COMPARE_TEXTURE_COORDINATE;

  public id: number;
  public point: Vector3Df;
  public normal: Vector3Df | null;
  public textureCoordinates: Vector3Df | null;
  public color: ColorRgbMutable;
  public radianceData: Element[] | null;
  public back: Vertex | null;
  public patches: Patch[] | null;
  public tmp: number;

  public constructor(
    inPoint: Vector3Df,
    inNormal: Vector3Df | null,
    inTextureCoordinates: Vector3Df | null,
    inPatches: Patch[] | null
  ) {
    this.id = Statistics.instance().reader.numberOfVertices++;
    this.point = inPoint;
    this.normal = inNormal;
    this.textureCoordinates = inTextureCoordinates;
    this.patches = inPatches;
    this.color = new ColorRgbMutable();
    this.color.set(0.0, 0.0, 0.0);
    this.radianceData = null;
    this.back = null;
    this.tmp = 0;
  }

  public destroy(): void {
    Statistics.instance().reader.numberOfVertices--;
    this.patches = null;
  }

  public computeColor(faceColors: FaceColorTable | null): void {
    this.color.set(0.0, 0.0, 0.0);
    let numberOfPatches = 0;

    if (this.patches !== null) {
      let r = 0.0;
      let g = 0.0;
      let b = 0.0;
      for (let i = 0; i < this.patches.length; i++) {
        // Before the scene's face table exists (loading) every face is black
        if (faceColors !== null) {
          const patchColor = faceColors.get(this.patches[i]!.getFaceIndex());
          r += patchColor.r;
          g += patchColor.g;
          b += patchColor.b;
        }
      }
      this.color.set(r, g, b);
      numberOfPatches = this.patches.length;
    }

    if (numberOfPatches > 0) {
      const invPatchCount = 1.0 / numberOfPatches;
      this.color.set(
        this.color.r * invPatchCount,
        this.color.g * invPatchCount,
        this.color.b * invPatchCount);
    }
  }

  public static setCompareFlags(flags: number): number {
    const oldFlags = Vertex.currentComparisonFlags;
    Vertex.currentComparisonFlags = flags;
    return oldFlags;
  }
}
