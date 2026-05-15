import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";

/**
ScatterInfo includes information about different scattering properties for different bsdf components
This info is used during scattering, but also when weighting or reading storage decisions must be made
*/
export class ScatterInfo {
  public flags: number;
  public nrSamplesBefore: number;
  public nrSamplesAfter: number;

  public constructor() {
    this.flags = 0;
    this.nrSamplesBefore = 0;
    this.nrSamplesAfter = 0;
  }

  public DoneThisBounce(node: SimpleRaytracingPathNode): boolean {
    return node.m_usedComponents === this.flags;
  }

  public DoneSomePreviousBounce(node: SimpleRaytracingPathNode): boolean {
    return (node.m_accUsedComponents & this.flags) === this.flags;
  }
}
