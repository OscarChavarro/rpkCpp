export class ScrambledHalton {
  private static readonly MAX_DIM = 10;
  private static readonly prime = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29];
  private static readonly sample = new Array<number>(ScrambledHalton.MAX_DIM).fill(0.0);

  public static scrambledHalton(nextN: number, dim: number): number[] {
    for (let i = 0; i < dim; i++) {
      const b = ScrambledHalton.prime[i]!;
      const bi = 1.0 / b;
      let fi = 0.0;
      let bp = 1.0;
      let m = 0;

      for (let j = nextN; j > 0; j = globalThis.Math.trunc(j / b)) {
        bp = bp * bi;
        let a = j % b;
        a = (a + m) % b;
        fi = fi + a * bp;
        m += 1;
      }
      ScrambledHalton.sample[i] = fi;
    }

    return ScrambledHalton.sample;
  }

  private constructor() {
  }
}
