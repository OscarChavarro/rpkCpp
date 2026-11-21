/**
Answer of a `SpatialIndex` query: the accepted primitive and the ray
parameter of its hit, or no primitive (`NO_PRIMITIVE`). An immutable value.
*/
export class SpatialQueryResult {
  public static readonly NO_PRIMITIVE = -1;

  private readonly primitive: number;
  private readonly parameter: number;

  /** Without arguments: no accepted hit */
  public constructor(inPrimitive: number = SpatialQueryResult.NO_PRIMITIVE, inParameter = 0.0) {
    this.primitive = inPrimitive;
    this.parameter = inParameter;
  }

  public hasHit(): boolean {
    return this.primitive !== SpatialQueryResult.NO_PRIMITIVE;
  }

  public getPrimitive(): number {
    return this.primitive;
  }

  /** Ray parameter of the accepted hit; meaningless without a hit */
  public getParameter(): number {
    return this.parameter;
  }
}
