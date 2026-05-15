import { FilePositionContext } from "./FilePositionContext";
import { TransformArrayContext } from "./TransformArrayContext";

export class TransformSequenceContext {
  public static readonly TRANSFORM_MAXIMUM_DIMENSIONS = 8;

  public startingPosition: FilePositionContext;
  public numberOfDimensions: number;
  public transformArguments: TransformArrayContext[];

  public constructor() {
    this.startingPosition = new FilePositionContext();
    this.numberOfDimensions = 0;
    this.transformArguments = new Array<TransformArrayContext>(TransformSequenceContext.TRANSFORM_MAXIMUM_DIMENSIONS);
    for (let i = 0; i < this.transformArguments.length; i++) {
      this.transformArguments[i] = new TransformArrayContext();
    }
  }
}
