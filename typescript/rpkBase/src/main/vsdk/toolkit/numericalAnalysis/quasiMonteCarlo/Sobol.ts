export class Sobol {
  private static readonly MAX_DIM = 5;
  private static readonly V_MAX = 30;
  private static dim = 0;
  private static nextN = 0;
  private static x = new Array<number>(Sobol.MAX_DIM).fill(0);
  private static v = Sobol.create2DArray(Sobol.MAX_DIM, Sobol.V_MAX);
  private static skip = 0;
  private static recip = 0.0;
  private static readonly nextSobolSample = new Array<number>(Sobol.MAX_DIM).fill(0.0);
  private static readonly sobolSample = new Array<number>(Sobol.MAX_DIM).fill(0.0);

  private static create2DArray(rows: number, cols: number): number[][] {
    const out = new Array<number[]>(rows);
    for (let i = 0; i < rows; i++) {
      out[i] = new Array<number>(cols).fill(0);
    }
    return out;
  }

  private static nextSobol(): number[] {
    let c = 1;
    let save = Sobol.nextN;
    while ((save % 2) === 1) {
      c += 1;
      save = globalThis.Math.trunc(save / 2);
    }
    for (let i = 0; i < Sobol.dim; i++) {
      Sobol.x[i] = Sobol.x[i]! ^ (Sobol.v[i]![c - 1]! << (Sobol.V_MAX - c));
      Sobol.nextSobolSample[i] = Sobol.x[i]! * Sobol.recip;
    }
    Sobol.nextN += 1;

    return Sobol.nextSobolSample;
  }

  private static sobolGray(n: number): number {
    return n ^ (n >> 1);
  }

  public static sobol(seed: number): number[] {
    seed += Sobol.skip + 1;
    for (let i = 0; i < Sobol.dim; i++) {
      Sobol.x[i] = 0;
      let c = 1;
      let gray = Sobol.sobolGray(seed);
      while (gray !== 0) {
        if ((gray & 1) !== 0) {
          Sobol.x[i] = Sobol.x[i]! ^ (Sobol.v[i]![c - 1]! << (Sobol.V_MAX - c));
        }
        c++;
        gray >>= 1;
      }

      Sobol.sobolSample[i] = Sobol.x[i]! * Sobol.recip;
    }

    return Sobol.sobolSample;
  }

  public static initSobol(iDim: number): void {
    const d = new Array<number>(Sobol.MAX_DIM).fill(0);
    const poly = new Array<number>(Sobol.MAX_DIM).fill(0);

    Sobol.nextN = 0;
    Sobol.dim = iDim;
    Sobol.recip = 1.0 / globalThis.Math.pow(2.0, Sobol.V_MAX);

    poly[0] = 3;
    d[0] = 1;
    poly[1] = 7;
    d[1] = 2;
    poly[2] = 11;
    d[2] = 3;
    poly[3] = 19;
    d[3] = 4;
    poly[4] = 37;
    d[4] = 5;

    for (let i = 0; i < Sobol.dim; i++) {
      const degree = d[i]!;
      for (let j = 0; j < degree; j++) {
        Sobol.v[i]![j] = 1;
      }
    }

    for (let i = 0; i < Sobol.dim; i++) {
      const degree = d[i]!;
      const row = Sobol.v[i]!;
      for (let j = degree; j < Sobol.V_MAX; j++) {
        row[j] = row[j - degree]!;
        let save = poly[i]!;
        let m = globalThis.Math.trunc(globalThis.Math.pow(2.0, degree));
        for (let k = degree; k > 0; k--) {
          row[j] = row[j]! ^ (m * (save % 2) * row[j - k]!);
          save = globalThis.Math.trunc(save / 2);
          m = globalThis.Math.trunc(m / 2);
        }
      }
    }

    for (let i = 0; i < Sobol.dim; i++) {
      Sobol.x[i] = 0;
    }
    Sobol.skip = globalThis.Math.trunc(globalThis.Math.pow(2.0, 6.0));
    for (let i = 1; i <= Sobol.skip; i++) {
      Sobol.nextSobol();
    }
  }

  private constructor() {
  }
}
