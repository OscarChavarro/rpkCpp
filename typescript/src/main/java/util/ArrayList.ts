export class ArrayList<T> {
  private readonly dataArray: T[];

  public constructor(_initialCapacity = 100) {
    this.dataArray = [];
  }

  public dispose(): void {
    this.dataArray.length = 0;
  }

  public size(): number {
    return this.dataArray.length;
  }

  public get(i: number): T {
    return this.dataArray[i];
  }

  public add(elem: T): boolean;
  public add(pos: number, elem: T): void;
  public add(posOrElem: number | T, elem?: T): boolean | void {
    if (typeof posOrElem === "number" && elem !== undefined) {
      const position = posOrElem < 0 ? 0 : posOrElem;
      if (position >= this.dataArray.length) {
        this.dataArray.push(elem);
        return;
      }
      this.dataArray.splice(position, 0, elem);
      return;
    }

    this.dataArray.push(posOrElem as T);
    return true;
  }

  public data(): T[] {
    return this.dataArray;
  }

  public clear(): void {
    this.dataArray.length = 0;
  }

  public remove(pos: number): void;
  public remove(data: T): void;
  public remove(posOrData: number | T): void {
    if (typeof posOrData === "number" && Number.isInteger(posOrData)) {
      const position = posOrData;
      if (position >= 0 && position < this.dataArray.length) {
        this.dataArray.splice(position, 1);
      }
      return;
    }

    const index = this.dataArray.indexOf(posOrData as T);
    if (index >= 0) {
      this.dataArray.splice(index, 1);
    }
  }

  public set(pos: number, elem: T): void {
    this.dataArray[pos] = elem;
  }
}
