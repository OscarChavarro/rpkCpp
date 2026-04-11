export class Jacobian {
  public A: number;
  public B: number;
  public C: number;

  public constructor(inA: number, inB: number, inC: number) {
    this.A = inA;
    this.B = inB;
    this.C = inC;
  }
}
