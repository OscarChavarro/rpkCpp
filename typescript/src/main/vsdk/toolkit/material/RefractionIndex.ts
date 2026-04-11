export class RefractionIndex {
  private nr: number;
  private ni: number;

  public constructor() {
    this.nr = 0.0;
    this.ni = 0.0;
  }

  public complexToGeometricRefractionIndex(): number {
    let f1 = (this.nr - 1.0);
    f1 = f1 * f1 + this.ni * this.ni;

    let f2 = (this.nr + 1.0);
    f2 = f2 * f2 + this.ni * this.ni;

    const sqrtF = globalThis.Math.sqrt(f1 / f2);
    return (1.0 + sqrtF) / (1.0 - sqrtF);
  }

  public getNr(): number {
    return this.nr;
  }

  public getNi(): number {
    return this.ni;
  }

  public set(inNr: number, inNi: number): void {
    this.nr = inNr;
    this.ni = inNi;
  }
}
