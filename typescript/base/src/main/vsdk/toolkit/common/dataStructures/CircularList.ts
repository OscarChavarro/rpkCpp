import { CircularListBase } from "./CircularListBase";
import { CircularListNode } from "./CircularListNode";

export class CircularList<T> extends CircularListBase {
  public add(data: T): void {
    this.addLink(new CircularListNode<T>(data));
  }

  public append(data: T): void {
    this.appendLink(new CircularListNode<T>(data));
  }

  public removeAll(): void {
    let link = this.remove() as CircularListNode<T> | null;
    while (link !== null) {
      link.nextLink = null;
      link = this.remove() as CircularListNode<T> | null;
    }
  }

  public override clear(): void {
    super.clear();
  }

  public baseList(): CircularListBase {
    return this;
  }
}
