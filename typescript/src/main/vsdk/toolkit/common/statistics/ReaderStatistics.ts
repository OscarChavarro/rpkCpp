export class ReaderStatistics {
  public numberOfGeometries: number;
  public numberOfCompounds: number;
  public numberOfSurfaces: number;
  public numberOfVertices: number;
  public numberOfPatches: number;
  public numberOfElements: number;
  public numberOfLightSources: number;

  public constructor() {
    this.numberOfGeometries = 0;
    this.numberOfCompounds = 0;
    this.numberOfSurfaces = 0;
    this.numberOfVertices = 0;
    this.numberOfPatches = 0;
    this.numberOfElements = 0;
    this.numberOfLightSources = 0;
  }

  public reset(): void {
    this.numberOfGeometries = 0;
    this.numberOfCompounds = 0;
    this.numberOfSurfaces = 0;
    this.numberOfVertices = 0;
    this.numberOfPatches = 0;
    this.numberOfElements = 0;
    this.numberOfLightSources = 0;
  }
}
