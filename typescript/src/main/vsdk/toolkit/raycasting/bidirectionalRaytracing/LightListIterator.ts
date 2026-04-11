import { CircularListIterator } from "../../common/dataStructures/CircularListIterator";
import { Patch } from "../../skin/Patch";
import { LightInfo } from "./LightInfo";
import { LightList } from "./LightList";

export class LightListIterator {
  private iterator: CircularListIterator<LightInfo>;

  public constructor(list: LightList) {
    this.iterator = new CircularListIterator<LightInfo>(list.entries());
  }

  public First(list: LightList): Patch | null {
    this.iterator.init(list.entries());

    const li = this.iterator.nextOnSequence();
    if (li !== null) {
      return li.light;
    }
    return null;
  }

  public Next(): Patch | null {
    const li = this.iterator.nextOnSequence();
    if (li !== null) {
      return li.light;
    }
    return null;
  }
}
