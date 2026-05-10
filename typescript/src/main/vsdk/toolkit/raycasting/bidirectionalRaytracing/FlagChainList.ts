import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { CircularList } from "../../common/dataStructures/CircularList";
import { CircularListIterator } from "../../common/dataStructures/CircularListIterator";
import { BiPath } from "./BiPath";
import { FlagChain } from "./FlagChain";

export class FlagChainList extends CircularList<FlagChain> {
  public length: number;
  public count: number;

  public constructor() {
    super();
    this.count = 0;
    this.length = 0;
  }

  public add(list: FlagChainList): void;
  public override add(chain: FlagChain): void;
  public override add(listOrChain: FlagChainList | FlagChain): void {
    if (listOrChain instanceof FlagChainList) {
      const iter = new CircularListIterator<FlagChain>(listOrChain);
      let tmpChain: FlagChain | null;

      while ((tmpChain = iter.nextOnSequence()) !== null) {
        this.add(tmpChain);
      }
      return;
    }

    const chain = listOrChain;
    if (this.count > 0) {
      if (chain.length !== this.length) {
        VsdkLogger.error("CChainList::add", "Wrong length flag chain inserted!");
        return;
      }
    }
    else {
      this.length = chain.length;
    }

    this.count++;
    this.append(new FlagChain(chain));
  }

  public addDisjoint(chain: FlagChain): void {
    if (this.count > 0) {
      if (chain.length !== this.length) {
        VsdkLogger.error("CChainList::add", "Wrong length flag chain inserted!");
        return;
      }
    }
    else {
      this.length = chain.length;
    }

    const iter = new CircularListIterator<FlagChain>(this);
    let tmpChain: FlagChain | null;
    let found = false;

    while ((tmpChain = iter.nextOnSequence()) !== null && !found) {
      found = FlagChain.compare(tmpChain, chain);
    }

    if (!found) {
      this.count++;
      this.append(new FlagChain(chain));
    }
  }

  public compute(path: BiPath): ColorRgb {
    const result = new ColorRgb();
    result.clear();

    const iter = new CircularListIterator<FlagChain>(this);
    let chain: FlagChain | null;

    while ((chain = iter.nextOnSequence()) !== null) {
      const tmpCol = chain.compute(path);
      result.add(tmpCol, result);
    }

    return result;
  }

  public simplify(): FlagChainList {
    const newList = new FlagChainList();
    const iter = new CircularListIterator<FlagChain>(this);

    let c1 = iter.nextOnSequence();

    if (c1 !== null) {
      let c2: FlagChain | null;
      while ((c2 = iter.nextOnSequence()) !== null) {
        const cCombined = FlagChain.combine(c1, c2);
        if (cCombined !== null) {
          c1 = cCombined;
        }
        else {
          newList.add(c1);
          c1 = c2;
        }
      }

      newList.add(c1);
    }

    return newList;
  }
}
