export class CubatureRule {
  public static readonly MAXIMUM_NODES = 20;

  public description: string | null;
  public numberOfNodes: number;
  public u: number[];
  public v: number[];
  public t: number[];
  public w: number[];

  public constructor();
  public constructor(
    description: string,
    numberOfNodes: number,
    u: number[],
    v: number[],
    t: number[],
    w: number[]
  );
  public constructor(
    description?: string,
    numberOfNodes?: number,
    u?: number[],
    v?: number[],
    t?: number[],
    w?: number[]
  ) {
    this.description = null;
    this.numberOfNodes = 0;
    this.u = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);
    this.v = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);
    this.t = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);
    this.w = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);

    if (
      description !== undefined
      && numberOfNodes !== undefined
      && u !== undefined
      && v !== undefined
      && t !== undefined
      && w !== undefined
    ) {
      this.description = description;
      this.numberOfNodes = numberOfNodes;
      CubatureRule.copyValues(this.u, u);
      CubatureRule.copyValues(this.v, v);
      CubatureRule.copyValues(this.t, t);
      CubatureRule.copyValues(this.w, w);
    }
  }

  private static copyValues(destination: number[] | null, source: number[] | null): void {
    if (destination === null || source === null) {
      return;
    }
    const n = globalThis.Math.min(destination.length, source.length);
    for (let i = 0; i < n; i++) {
      destination[i] = source[i];
    }
  }
}
