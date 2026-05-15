import { CircularListBase } from "./CircularListBase";
import { CircularListLink } from "./CircularListLink";

export class CircularListBaseIterator {
  private currentElement: CircularListLink | null;
  private currentList: CircularListBase | null;

  public constructor(list: CircularListBase) {
    this.currentElement = null;
    this.currentList = null;
    this.init(list);
  }

  public init(list: CircularListBase): void {
    this.currentList = list;
    this.currentElement = this.currentList.lastLink();
  }

  public next(): CircularListLink | null {
    let response: CircularListLink | null;

    if (this.currentElement === null) {
      response = null;
    }
    else {
      this.currentElement = this.currentElement.nextLink;
      response = this.currentElement;
    }

    if (this.currentList !== null && this.currentElement === this.currentList.lastLink()) {
      this.currentElement = null;
    }

    return response;
  }
}
