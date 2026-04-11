export class CircularListLink {
  public nextLink: CircularListLink | null;

  public constructor() {
    this.nextLink = null;
  }
}
