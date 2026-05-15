import { StochasticRaytracingPathNode } from "./StochasticRaytracingPathNode";

/**
A full path, basically an array of 'numberOfNodes' path nodes
*/
export class Path {
  public numberOfNodes: number;
  public nodesAllocated: number;
  public nodes: StochasticRaytracingPathNode[] | null;

  public constructor() {
    this.numberOfNodes = 0;
    this.nodesAllocated = 0;
    this.nodes = null;
  }
}
