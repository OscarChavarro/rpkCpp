import { CircularListLink } from "./CircularListLink";

export class CircularListNode<T> extends CircularListLink {
  public data: T;

  public constructor(inData: T) {
    super();
    this.data = inData;
  }
}
