import { CircularListLink } from "./CircularListLink";

export class CircularListBase {
  private last: CircularListLink | null;

  public constructor() {
    this.last = null;
  }

  public addLink(data: CircularListLink): void {
    if (this.last !== null) {
      data.nextLink = this.last.nextLink;
    }
    else {
      this.last = data;
    }
    this.last.nextLink = data;
  }

  public appendLink(data: CircularListLink): void {
    if (this.last !== null) {
      data.nextLink = this.last.nextLink;
      this.last = this.last.nextLink = data;
    }
    else {
      this.last = data;
      data.nextLink = data;
    }
  }

  public remove(): CircularListLink | null {
    if (this.last === null) {
      return null;
    }

    const first = this.last.nextLink as CircularListLink;
    if (first === this.last) {
      this.last = null;
    }
    else {
      this.last.nextLink = first.nextLink;
    }

    return first;
  }

  public clear(): void {
    this.last = null;
  }

  public lastLink(): CircularListLink | null {
    return this.last;
  }
}
