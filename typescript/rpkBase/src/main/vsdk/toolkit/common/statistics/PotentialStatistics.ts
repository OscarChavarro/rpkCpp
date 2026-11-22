export class PotentialStatistics {
  public averageDirectPotential: number;
  public maxDirectPotential: number;
  public maxDirectImportance: number;
  public totalDirectPotential: number;

  public constructor() {
    this.averageDirectPotential = 0.0;
    this.maxDirectPotential = 0.0;
    this.maxDirectImportance = 0.0;
    this.totalDirectPotential = 0.0;
  }

  public reset(): void {
    this.averageDirectPotential = 0.0;
    this.maxDirectPotential = 0.0;
    this.maxDirectImportance = 0.0;
    this.totalDirectPotential = 0.0;
  }
}
