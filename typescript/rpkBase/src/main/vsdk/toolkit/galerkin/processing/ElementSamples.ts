import type { GalerkinElement } from "../GalerkinElement";

/** Immutable count of the scratch buffer samples a top-level element covers */
export class ElementSamples {
  private readonly element: GalerkinElement | null;
  private readonly samples: number;

  public constructor();
  public constructor(inElement: GalerkinElement, inSamples: number);
  public constructor(inElement: GalerkinElement | null = null, inSamples = 0) {
    this.element = inElement;
    this.samples = inSamples;
  }

  public getElement(): GalerkinElement {
    return this.element as GalerkinElement;
  }

  public getSamples(): number {
    return this.samples;
  }
}
