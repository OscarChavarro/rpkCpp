import { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd";
import { Vertex } from "../../skin/Vertex";

export class VertexContext {
  public p: Vector3Dd;
  public n: Vector3Dd;
  public xid: number;
  public clock: number;
  public vertex: Vertex | null;

  public constructor();
  public constructor(inP: Vector3Dd, inN: Vector3Dd, inXid: number, inClock: number, inVertex: Vertex | null);
  public constructor(inP?: Vector3Dd, inN?: Vector3Dd, inXid?: number, inClock?: number, inVertex?: Vertex | null) {
    this.p = new Vector3Dd();
    this.n = new Vector3Dd();
    this.xid = 0;
    this.clock = 0;
    this.vertex = null;

    if (inP !== undefined && inN !== undefined && inXid !== undefined && inClock !== undefined) {
      this.p.copy(inP);
      this.n.copy(inN);
      this.xid = inXid;
      this.clock = inClock;
      this.vertex = inVertex ?? null;
    }
  }

  public copy(source: VertexContext | null): void {
    if (source === null) {
      return;
    }
    this.p.copy(source.p);
    this.n.copy(source.n);
    this.xid = source.xid;
    this.clock = source.clock;
    this.vertex = source.vertex;
  }
}
