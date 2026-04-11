export class RayTracerStatistics {
  public totalTime: number;
  public rayCount: number;
  public pixelCount: number;

  public constructor() {
    this.totalTime = 0.0;
    this.rayCount = 0;
    this.pixelCount = 0;
  }

  public resetCounters(): void {
    this.totalTime = 0.0;
    this.rayCount = 0;
    this.pixelCount = 0;
  }
}
