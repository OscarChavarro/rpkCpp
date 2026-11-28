import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { PolygonVisibilityBuffer } from "vitral/dist/vsdk/toolkit/render/raster/polygonScanConverter/PolygonVisibilityBuffer";
import { BoundingBox } from "../../skin/AxisAlignedBoundingBox";
import type { GalerkinElement } from "../GalerkinElement";
import { ElementSamples } from "./ElementSamples";

/**
Frame buffer of the scratch renderer: each pixel holds the surface element
seen there, borrowed for the current solve. A null sample is background.
*/
export type ScratchVisibilityBuffer = PolygonVisibilityBuffer<GalerkinElement | null>;

/**
Scratch of the Z-visibility clustering strategy: the software renderer the
elements of a cluster are rendered into, seen from a sample point, with the
virtual screen of the last rendering and the samples each element covers in
it, as a list of the visible elements in the order they first appear.

Workspace: mutable, owned by the solver (the serial one) or by a parallel
Jacobi worker, and passed explicitly to ScratchVisibilityStrategy, so that
workers render at the same time. The last cluster and eye point are kept to
reuse a rendering; they must match exactly, so that the result never depends
on which rendering a workspace did before. The lists keep their storage from
one counting to the next.
*/
export class ScratchVisibilityWorkspace {
  private readonly buffer: ScratchVisibilityBuffer;
  private lastCluster: GalerkinElement | null;
  private lastEye: Vector3Dd;
  private readonly bounds: BoundingBox;

  // Visible elements of the last counting, and for each element id the
  // counting that last saw it (countingStamp) and its index in the list
  private readonly visibleElements: ElementSamples[];
  private readonly countingStamps: number[];
  private readonly visibleIndices: number[];
  private countingStamp: number;

  public constructor(frameBufferSize: number) {
    this.buffer = new PolygonVisibilityBuffer<GalerkinElement | null>(frameBufferSize, frameBufferSize, null);
    this.lastCluster = null;
    this.lastEye = new Vector3Dd();
    this.bounds = new BoundingBox();
    this.visibleElements = [];
    this.countingStamps = [];
    this.visibleIndices = [];
    this.countingStamp = 0;
  }

  public getBuffer(): ScratchVisibilityBuffer {
    return this.buffer;
  }

  /** True when the buffer holds the rendering of this cluster seen from exactly this eye */
  public holdsRendering(cluster: GalerkinElement, eye: Vector3Dd): boolean {
    return cluster === this.lastCluster
      && eye.x() === this.lastEye.x() && eye.y() === this.lastEye.y() && eye.z() === this.lastEye.z();
  }

  public setRendering(cluster: GalerkinElement, eye: Vector3Dd): void {
    this.lastCluster = cluster;
    this.lastEye = eye;
  }

  /** Virtual screen of the last rendering, in the coordinates of its eye */
  public getBounds(): BoundingBox {
    return this.bounds;
  }

  /** Starts a new counting: no element is visible */
  public startCounting(): void {
    this.visibleElements.length = 0;
    this.countingStamp++;
  }

  /** Adds samples to the element of the given id */
  public addSamples(element: GalerkinElement, elementId: number, samples: number): void {
    while (this.countingStamps.length <= elementId) {
      this.countingStamps.push(0);
      this.visibleIndices.push(0);
    }
    if (this.countingStamps[elementId] === this.countingStamp) {
      const index = this.visibleIndices[elementId]!;
      this.visibleElements[index] = new ElementSamples(element, this.visibleElements[index]!.getSamples() + samples);
    }
    else {
      this.countingStamps[elementId] = this.countingStamp;
      this.visibleIndices[elementId] = this.visibleElements.length;
      this.visibleElements.push(new ElementSamples(element, samples));
    }
  }

  /** The elements of the last counting, each with its samples */
  public getVisibleElements(): readonly ElementSamples[] {
    return this.visibleElements;
  }
}
