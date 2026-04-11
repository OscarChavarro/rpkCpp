import { Patch } from "../../skin/Patch";

export class LightInfo {
  public emittedFlux: number;
  public importance: number;
  public light: Patch | null;

  public constructor() {
    this.emittedFlux = 0.0;
    this.importance = 0.0;
    this.light = null;
  }
}
