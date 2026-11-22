import { Patch } from "../../environment/geometry/elements/Patch";

export class LightSourceTable {
  public patch: Patch | null;
  public flux: number;

  public constructor(patch: Patch | null = null, flux = 0.0) {
    this.patch = patch;
    this.flux = flux;
  }
}
