import { TransformContext } from "./TransformContext";
import { TransformSequenceContext } from "./TransformSequenceContext";

export class TransformStackContext {
  public xid: number;
  public xac: number;
  public rev: number;
  public ownedArgumentCount: number;
  public xf: TransformContext;
  public transformationArray: TransformSequenceContext | null;
  public ownedArgumentCopies: Array<string | null> | null;
  public prev: TransformStackContext | null;

  public constructor() {
    this.xid = 0;
    this.xac = 0;
    this.rev = 0;
    this.ownedArgumentCount = 0;
    this.xf = new TransformContext();
    this.transformationArray = null;
    this.ownedArgumentCopies = null;
    this.prev = null;
  }
}
