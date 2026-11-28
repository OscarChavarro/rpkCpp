/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/
import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Numeric } from "../../skin/Numeric";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinState } from "../GalerkinState";
import { ScratchRendererVisitor } from "./visitors/ScratchRendererVisitor";
import { Camera } from "../../scene/Camera";
import { BoundingBox } from "../../skin/AxisAlignedBoundingBox";
import { ClusterTraversalStrategy } from "./ClusterTraversalStrategy";
import { ScratchVisibilityWorkspace } from "./ScratchVisibilityWorkspace";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

/**
Every operation works on the scratch workspace it is given, the solver's or a
parallel Jacobi worker's, and reads the solver state only
*/
export class ScratchVisibilityStrategy {
  private constructor() {
  }

  /**
  Create a scratch software renderer for various operations on clusters
  */
  public static scratchInit(galerkinState: GalerkinState): void {
    galerkinState.scratchVisibilityWorkspace = new ScratchVisibilityWorkspace(galerkinState.scratchFrameBufferSize);
  }

  /**
  Terminates scratch rendering
  */
  public static scratchTerminate(galerkinState: GalerkinState): void {
    if (galerkinState.scratchVisibilityWorkspace !== null) {
      galerkinState.scratchVisibilityWorkspace = null;
    }
  }

  /**
  Sets up an orthographic projection of the cluster as
  seen from the eye. Renders the element pointers to the elements
  in cluster in the scratch frame buffer of the workspace and returns the bounding
  box containing the size of the virtual screen. The cluster nicely fits
  into the virtual screen.
  TypeScript: the bounding box returned is the workspace's own (C++ returns a
  copy), valid until the next rendering into the workspace
  */
  public static scratchRenderElements(
    cluster: GalerkinElement,
    eye: Vector3Dd,
    scratchWorkspace: ScratchVisibilityWorkspace,
    galerkinState: GalerkinState,
  ): BoundingBox {
    const boundingBox = scratchWorkspace.getBounds();

    if (scratchWorkspace.holdsRendering(cluster, eye)) {
      return boundingBox;
    }

    // Cache previously rendered cluster and eye point in order to
    // avoid re-rendering the same situation next time
    scratchWorkspace.setRendering(cluster, eye);

    const center = cluster.midPoint();
    let up = new Vector3Dd(0.0, 0.0, 1.0);
    let viewDirection = new Vector3Dd();

    viewDirection = center.subtract(eye);
    viewDirection = viewDirection.normalized(Numeric.EPSILON);
    if (globalThis.Math.abs(up.dotProduct(viewDirection)) > 1.0 - VSDK.EPSILON) {
      up = new Vector3Dd(0.0, 1.0, 0.0);
    }

    const lookAt = Camera.lookAtMatrix(eye, center, up);

    Camera.transformBoundingBox((cluster.geometry as NonNullable<GalerkinElement["geometry"]>).getBoundingBox(), lookAt, boundingBox);

    const scratch = scratchWorkspace.getBuffer();
    const projection = Camera.projectionMatrixFromBoundingBox(boundingBox);
    scratch.setTransform(projection.multiply(lookAt));

    // Choose a viewport based on the smallest surface element in the cluster.
    let viewportSize = globalThis.Math.trunc((boundingBox.dx() * boundingBox.dy()) / cluster.minimumArea);
    if (viewportSize < 32) {
      viewportSize = 32;
    }
    if (viewportSize > scratch.getWidth()) {
      viewportSize = scratch.getWidth();
    }
    scratch.setViewport(0, 0, viewportSize, viewportSize);
    scratch.clear();

    const leafVisitor = new ScratchRendererVisitor(eye, scratch);
    ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, cluster, galerkinState);

    return boundingBox;
  }

  /**
  After rendering element pointers in the scratch frame buffer, this routine
  computes the average radiance of the virtual screen
  */
  public static scratchRadiance(scratchWorkspace: ScratchVisibilityWorkspace, galerkinState: GalerkinState): ColorRgbMutable {
    let rad0 = 0.0;
    let rad1 = 0.0;
    let rad2 = 0.0;

    const scratch = scratchWorkspace.getBuffer();
    const viewport = scratch.getViewport();

    let nonBackground = 0;
    for (let y = viewport.getMinY(); y <= viewport.getMaxY(); y++) {
      for (let x = viewport.getMinX(); x <= viewport.getMaxX(); x++) {
        const element = scratch.getSample(x, y);
        if (element !== null) {
          if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
            || galerkinState.isJacobi()) {
            const radiance = (element.radiance as ColorRgbMutable[])[0]!;
            rad0 += radiance.r;
            rad1 += radiance.g;
            rad2 += radiance.b;
          }
          else {
            const unShotRadiance = (element.unShotRadiance as ColorRgbMutable[])[0]!;
            rad0 += unShotRadiance.r;
            rad1 += unShotRadiance.g;
            rad2 += unShotRadiance.b;
          }
          nonBackground++;
        }
      }
    }
    if (nonBackground > 0) {
      const scale = 1.0 / (scratch.getViewportWidth() * scratch.getViewportHeight());
      rad0 *= scale;
      rad1 *= scale;
      rad2 *= scale;
    }
    return new ColorRgbMutable(rad0, rad1, rad2);
  }

  /** Computes the number of covered samples in the scratch viewport. */
  public static scratchNonBackgroundPixels(scratchWorkspace: ScratchVisibilityWorkspace): number {
    const scratch = scratchWorkspace.getBuffer();
    const viewport = scratch.getViewport();
    let nonBackground = 0;

    for (let y = viewport.getMinY(); y <= viewport.getMaxY(); y++) {
      for (let x = viewport.getMinX(); x <= viewport.getMaxX(); x++) {
        if (scratch.getSample(x, y) !== null) {
          nonBackground++;
        }
      }
    }
    return nonBackground;
  }

  /**
  Counts, in the workspace, the samples covered by each of the solver's
  top-level surface elements in the last rendering
  */
  public static scratchPixelsPerElement(scratchWorkspace: ScratchVisibilityWorkspace, galerkinState: GalerkinState): void {
    const scratch = scratchWorkspace.getBuffer();
    const viewport = scratch.getViewport();

    scratchWorkspace.startCounting();
    // Neighbouring samples mostly show the same element: count runs of equal
    // samples and add each run to its element at once
    let runElement: GalerkinElement | null = null;
    let runLength = 0;
    for (let y = viewport.getMinY(); y <= viewport.getMaxY(); y++) {
      for (let x = viewport.getMinX(); x <= viewport.getMaxX(); x++) {
        const element = scratch.getSample(x, y);
        if (element === runElement) {
          runLength++;
        }
        else {
          ScratchVisibilityStrategy.addSamples(scratchWorkspace, runElement, runLength, galerkinState);
          runElement = element;
          runLength = 1;
        }
      }
    }
    ScratchVisibilityStrategy.addSamples(scratchWorkspace, runElement, runLength, galerkinState);
  }

  /** Adds the samples of a run showing the element (none for the background) */
  private static addSamples(
    scratchWorkspace: ScratchVisibilityWorkspace,
    element: GalerkinElement | null,
    samples: number,
    galerkinState: GalerkinState,
  ): void {
    if (element === null || samples === 0) {
      return;
    }
    // Cluster leaves are the same top-level elements bound to the scene faces.
    const topLevel = galerkinState.topLevelElement(element.patch!);
    scratchWorkspace.addSamples(topLevel, topLevel.id, samples);
  }

  /** Area of the virtual screen that one sample of the last rendering covers */
  public static scratchPixelArea(boundingBox: BoundingBox, scratchWorkspace: ScratchVisibilityWorkspace): number {
    const scratch = scratchWorkspace.getBuffer();
    return boundingBox.dx() * boundingBox.dy()
      / (scratch.getViewportWidth() * scratch.getViewportHeight());
  }
}
