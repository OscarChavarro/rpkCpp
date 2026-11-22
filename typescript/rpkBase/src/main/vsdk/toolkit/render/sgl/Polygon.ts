import { PolygonClipResultInfo } from "./PolygonClipResultInfo";
import { PolygonVertex } from "./PolygonVertex";

export class Polygon {
  public n: number;
  public mask: number;
  public vertices: PolygonVertex[];

  public constructor() {
    this.n = 0;
    this.mask = 0;
    this.vertices = new Array<PolygonVertex>(PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON);
    for (let i = 0; i < this.vertices.length; i++) {
      this.vertices[i] = new PolygonVertex();
    }
  }
}
