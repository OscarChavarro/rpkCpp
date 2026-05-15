import { BidirectionalPathRaytracerConfig } from "./BidirectionalPathRaytracerConfig";
import { Spar } from "./Spar";

export class SparConfig {
  public baseConfig: BidirectionalPathRaytracerConfig | null;
  public leSpar: Spar | null;
  public ldSpar: Spar | null;

  public constructor() {
    this.baseConfig = null;
    this.leSpar = null;
    this.ldSpar = null;
  }
}
