import { CircularList } from "./CircularList";
import { CircularListBaseIterator } from "./CircularListBaseIterator";
import { CircularListNode } from "./CircularListNode";

export class CircularListIterator<T> extends CircularListBaseIterator {
  public constructor(list: CircularList<T>) {
    super(list.baseList());
  }

  public nextOnSequence(): T | null {
    const link = this.next() as CircularListNode<T> | null;
    return link !== null ? link.data : null;
  }

  public override init(list: CircularList<T>): void {
    super.init(list.baseList());
  }
}
