import { Geometry } from "../skin/Geometry";

export class OctreeChild {
  public geometry: Geometry | null;
  public distance: number;

  public constructor() {
    this.geometry = null;
    this.distance = 0.0;
  }
}
