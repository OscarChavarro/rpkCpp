import { ColorRgb } from "../common/ColorRgb";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { Statistics } from "../common/statistics/Statistics";
import { VertexCompareFlags } from "./VertexCompareFlags";
import type { Element } from "./Element";
import type { Patch } from "./Patch";

export class Vertex {
  private static currentComparisonFlags = VertexCompareFlags.VERTEX_COMPARE_LOCATION
    | VertexCompareFlags.VERTEX_COMPARE_NORMAL
    | VertexCompareFlags.VERTEX_COMPARE_TEXTURE_COORDINATE;

  public id: number;
  public point: Vector3D;
  public normal: Vector3D | null;
  public textureCoordinates: Vector3D | null;
  public color: ColorRgb;
  public radianceData: Element[] | null;
  public back: Vertex | null;
  public patches: Patch[] | null;
  public tmp: number;

  public constructor(
    inPoint: Vector3D,
    inNormal: Vector3D | null,
    inTextureCoordinates: Vector3D | null,
    inPatches: Patch[] | null
  ) {
    this.id = Statistics.instance().reader.numberOfVertices++;
    this.point = inPoint;
    this.normal = inNormal;
    this.textureCoordinates = inTextureCoordinates;
    this.patches = inPatches;
    this.color = new ColorRgb();
    this.color.set(0.0, 0.0, 0.0);
    this.radianceData = null;
    this.back = null;
    this.tmp = 0;
  }

  public destroy(): void {
    Statistics.instance().reader.numberOfVertices--;
    this.patches = null;
  }

  public computeColor(): void {
    this.color.set(0.0, 0.0, 0.0);
    let numberOfPatches = 0;

    if (this.patches !== null) {
      for (let i = 0; i < this.patches.length; i++) {
        const patch = this.patches[i];
        this.color.r += patch.color.r;
        this.color.g += patch.color.g;
        this.color.b += patch.color.b;
      }
      numberOfPatches = this.patches.length;
    }

    if (numberOfPatches > 0) {
      this.color.r /= numberOfPatches;
      this.color.g /= numberOfPatches;
      this.color.b /= numberOfPatches;
    }
  }

  public static setCompareFlags(flags: number): number {
    const oldFlags = Vertex.currentComparisonFlags;
    Vertex.currentComparisonFlags = flags;
    return oldFlags;
  }
}
