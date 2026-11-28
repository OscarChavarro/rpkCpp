import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";

/**
What the light transport over one interaction with clusters takes from the
scratch visibility renderer, computed ahead (by a parallel Jacobi worker)
so that the transport itself, which sums into elements shared by other
interactions, is done afterwards in the serial order: the radiance a source
cluster emits towards the receiver and, for a receiver cluster seen from
outside, the area of a sample of its virtual screen and the samples each of
its visible top-level elements covers. Those counts are a range of a sample
pool of the worker (an ElementSamples[]), given by its first index and
length, so that planning allocates no memory per interaction. A part that
was not computed ahead is computed by the transport itself.
TypeScript: the plans of a batch are reused from one batch to the next
(clear), instead of being created for each batch.
*/
export class ClusterTransportPlan {
  private sourceRadianceKnown: boolean;
  private readonly sourceRadiance: ColorRgbMutable;
  private receiverSamplesKnown: boolean;
  private pixelArea: number;
  private pool: number;
  private firstSample: number;
  private numberOfSamples: number;

  public constructor() {
    this.sourceRadianceKnown = false;
    this.sourceRadiance = new ColorRgbMutable(0.0, 0.0, 0.0);
    this.receiverSamplesKnown = false;
    this.pixelArea = 0.0;
    this.pool = -1;
    this.firstSample = 0;
    this.numberOfSamples = 0;
  }

  /** Back to the state of a new plan: nothing computed ahead */
  public clear(): void {
    this.sourceRadianceKnown = false;
    this.sourceRadiance.set(0.0, 0.0, 0.0);
    this.receiverSamplesKnown = false;
    this.pixelArea = 0.0;
    this.pool = -1;
    this.firstSample = 0;
    this.numberOfSamples = 0;
  }

  public hasSourceRadiance(): boolean {
    return this.sourceRadianceKnown;
  }

  /** The plan's own copy: valid until the plan is changed */
  public getSourceRadiance(): ColorRgbMutable {
    return this.sourceRadiance;
  }

  public setSourceRadiance(radiance: ColorRgbMutable): void {
    this.sourceRadianceKnown = true;
    this.sourceRadiance.set(radiance.r, radiance.g, radiance.b);
  }

  public hasReceiverSamples(): boolean {
    return this.receiverSamplesKnown;
  }

  public getPixelArea(): number {
    return this.pixelArea;
  }

  public getPool(): number {
    return this.pool;
  }

  public getFirstSample(): number {
    return this.firstSample;
  }

  public getNumberOfSamples(): number {
    return this.numberOfSamples;
  }

  public setReceiverSamples(inPixelArea: number, inPool: number, inFirstSample: number, inNumberOfSamples: number): void {
    this.receiverSamplesKnown = true;
    this.pixelArea = inPixelArea;
    this.pool = inPool;
    this.firstSample = inFirstSample;
    this.numberOfSamples = inNumberOfSamples;
  }
}
