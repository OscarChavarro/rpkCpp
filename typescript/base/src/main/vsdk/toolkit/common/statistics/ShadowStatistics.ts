export class ShadowStatistics {
  public numberOfShadowRays: number;
  public numberOfShadowCacheHits: number;

  public constructor() {
    this.numberOfShadowRays = 0;
    this.numberOfShadowCacheHits = 0;
  }

  public reset(): void {
    this.numberOfShadowRays = 0;
    this.numberOfShadowCacheHits = 0;
  }
}
