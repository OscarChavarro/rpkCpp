/**
All bases are orthonormal on their standard domain
*/
export class GalerkinBasis {
  public static readonly MAX_BASIS_SIZE = 10;

  public description: string;
  public size: number;

  public function: Array<(u: number, v: number) => number> | null;
  public dualFunction: Array<(u: number, v: number) => number> | null;
  public regularFilter: number[][][] | null;

  public constructor() {
    this.description = "";
    this.size = 0;
    this.function = null;
    this.dualFunction = null;
    this.regularFilter = null;
  }
}
