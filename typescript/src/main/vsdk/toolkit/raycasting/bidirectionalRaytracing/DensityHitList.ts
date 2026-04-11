import { Error as VsdkError } from "../../common/Error";
import { DensityHit } from "./DensityHit";
import { DensityHitArray } from "./DensityHitArray";

export class DensityHitList {
  protected static readonly DHL_ARRAY_SIZE = 20;
  protected first: DensityHitArray;
  protected last: DensityHitArray;
  protected numHits: number;
  protected cacheLowerLimit: number;
  protected cacheCurrent: DensityHitArray | null;

  public constructor() {
    this.first = new DensityHitArray(DensityHitList.DHL_ARRAY_SIZE);
    this.last = this.first;
    this.cacheCurrent = null;
    this.numHits = 0;
    this.cacheLowerLimit = 0;
  }

  public add(hit: DensityHit): void {
    if (!this.last.add(hit)) {
      this.last.setNext(new DensityHitArray(DensityHitList.DHL_ARRAY_SIZE));
      this.last = this.last.getNext() as DensityHitArray;
      this.last.add(hit);
    }

    this.numHits++;
  }

  public storedHits(): number {
    return this.numHits;
  }

  public get(i: number): DensityHit {
    if (i >= this.numHits) {
      VsdkError.fatal(-1, "DensityHitList::operator[]", "Index 'i' out of getBoundingBox");
    }

    if (this.cacheCurrent === null || i < this.cacheLowerLimit) {
      this.cacheCurrent = this.first;
      this.cacheLowerLimit = 0;
    }

    while (i >= this.cacheLowerLimit + DensityHitList.DHL_ARRAY_SIZE) {
      this.cacheCurrent = this.cacheCurrent!.getNext();
      this.cacheLowerLimit += DensityHitList.DHL_ARRAY_SIZE;
    }

    return this.cacheCurrent!.get(i - this.cacheLowerLimit);
  }
}
