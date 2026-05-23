import { DensityHit } from "./DensityHit";

export class DensityHitArray {
  private hits: DensityHit[];
  private maxHits: number;
  private numHits: number;
  private next: DensityHitArray | null;

  public constructor(paramMaxHits: number) {
    this.numHits = 0;
    this.maxHits = paramMaxHits;
    this.hits = new Array<DensityHit>(paramMaxHits);
    for (let i = 0; i < paramMaxHits; i++) {
      this.hits[i] = new DensityHit();
    }
    this.next = null;
  }

  public add(hit: DensityHit): boolean {
    if (this.numHits < this.maxHits) {
      this.hits[this.numHits] = new DensityHit(hit.m_x, hit.m_y, hit.color);
      this.numHits++;
      return true;
    }
    return false;
  }

  public get(i: number): DensityHit {
    const hit = this.hits[i];
    if (hit === undefined) {
      throw new RangeError(`DensityHitArray index out of bounds: ${i}`);
    }
    return hit;
  }

  public getNext(): DensityHitArray | null {
    return this.next;
  }

  public setNext(inNext: DensityHitArray | null): void {
    this.next = inNext;
  }
}
