export abstract class PixelFilter {
  public constructor() {
  }

  public abstract sample(xi1: number[], xi2: number[]): void;
}
