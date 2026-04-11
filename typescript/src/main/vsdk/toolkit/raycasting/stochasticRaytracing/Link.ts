type StochasticRadiosityElement = any;

export class Link {
  public rcv: StochasticRadiosityElement | null;
  public src: StochasticRadiosityElement | null;

  public constructor() {
    this.rcv = null;
    this.src = null;
  }
}
