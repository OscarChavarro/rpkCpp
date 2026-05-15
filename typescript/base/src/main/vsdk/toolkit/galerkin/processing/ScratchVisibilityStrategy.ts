import { ColorRgb } from "../../common/color/ColorRgb";
import { Matrix4x4 } from "../../common/linealAlgebra/Matrix4x4";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinState } from "../GalerkinState";
import { ScratchRendererVisitor } from "./visitors/ScratchRendererVisitor";
import { SglConstants } from "../../render/sgl/SglConstants";
import { SglContext } from "../../render/sgl/SglContext";
import { Camera } from "../../scene/Camera";
import { BoundingBox } from "../../skin/AxisAlignedBoundingBox";
import { Element } from "../../environment/geometry/elements/Element";
import { ClusterTraversalStrategy } from "./ClusterTraversalStrategy";

export class ScratchVisibilityStrategy {
  public static scratchInit(galerkinState: GalerkinState): void {
    galerkinState.scratch = new SglContext(galerkinState.scratchFrameBufferSize, galerkinState.scratchFrameBufferSize);
    galerkinState.scratch.sglDepthTesting(true);
  }

  public static scratchTerminate(galerkinState: GalerkinState): void {
    if (galerkinState.scratch !== null) {
      galerkinState.scratch = null;
    }
  }

  public static scratchRenderElements(cluster: GalerkinElement, eye: Vector3D, galerkinState: GalerkinState): BoundingBox {
    const boundingBox = new BoundingBox();

    if (cluster.id === galerkinState.lastClusterId
      && eye.equals(galerkinState.lastEye, Numeric.EPSILON_FLOAT)) {
      return boundingBox;
    }

    galerkinState.lastClusterId = cluster.id;
    galerkinState.lastEye = eye;

    const center = cluster.midPoint();
    const up = new Vector3D(0.0, 0.0, 1.0);
    const viewDirection = new Vector3D();

    viewDirection.subtraction(center, eye);
    viewDirection.normalize(Numeric.EPSILON_FLOAT);
    if (globalThis.Math.abs(up.dotProduct(viewDirection)) > 1.0 - Numeric.EPSILON) {
      up.set(0.0, 1.0, 0.0);
    }

    const lookAt = Matrix4x4.createLookAtMatrix(eye, center, up);

    Camera.transformBoundingBox((cluster.geometry as NonNullable<GalerkinElement["geometry"]>).getBoundingBox(), lookAt, boundingBox);

    const scratch = galerkinState.scratch as SglContext;
    const o = Camera.projectionMatrixFromBoundingBox(boundingBox);
    scratch.sglLoadMatrix(o);
    scratch.sglMultiplyMatrix(lookAt);

    let vpSize = globalThis.Math.trunc((boundingBox.dx() * boundingBox.dy()) / cluster.minimumArea);
    if (vpSize > scratch.width) {
      vpSize = scratch.width;
    }
    if (vpSize < 32) {
      vpSize = 32;
    }
    scratch.sglViewport(0, 0, vpSize, vpSize);

    scratch.sglClear(0x00, SglConstants.SGL_MAXIMUM_Z);

    const leafVisitor = new ScratchRendererVisitor(eye, scratch);
    ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, cluster, galerkinState);

    return boundingBox;
  }

  public static scratchRadiance(galerkinState: GalerkinState): ColorRgb {
    const rad = new ColorRgb();
    rad.clear();
    let nonBackGround = 0;

    for (let j = 0; j < (galerkinState.scratch as SglContext).vp_height; j++) {
      const rowStart = j * (galerkinState.scratch as SglContext).width;
      for (let i = 0; i < (galerkinState.scratch as SglContext).vp_width; i++) {
        const elementBase = (galerkinState.scratch as SglContext).galerkinElementBuffer[rowStart + i];
        if (elementBase instanceof GalerkinElement) {
          const element = elementBase as GalerkinElement;
          if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
            || galerkinState.galerkinIterationMethod === GalerkinIterationMethod.JACOBI) {
            rad.add(rad, (element.radiance as ColorRgb[])[0]);
          }
          else {
            rad.add(rad, (element.unShotRadiance as ColorRgb[])[0]);
          }
          nonBackGround++;
        }
      }
    }
    if (nonBackGround > 0) {
      rad.scale(1.0 / ((galerkinState.scratch as SglContext).vp_width * (galerkinState.scratch as SglContext).vp_height));
    }
    return rad;
  }

  public static scratchNonBackgroundPixels(galerkinState: GalerkinState): number {
    let nonBackGround = 0;

    for (let j = 0; j < (galerkinState.scratch as SglContext).vp_height; j++) {
      const rowStart = j * (galerkinState.scratch as SglContext).width;
      for (let i = 0; i < (galerkinState.scratch as SglContext).vp_width; i++) {
        const elementBase = (galerkinState.scratch as SglContext).galerkinElementBuffer[rowStart + i];
        if (elementBase !== null) {
          nonBackGround++;
        }
      }
    }
    return nonBackGround;
  }

  public static scratchPixelsPerElement(galerkinState: GalerkinState): void {
    for (let i = 0; i < (galerkinState.scratch as SglContext).vp_height; i++) {
      const rowStart = i * (galerkinState.scratch as SglContext).width;
      for (let j = 0; j < (galerkinState.scratch as SglContext).vp_width; j++) {
        const elementBase = (galerkinState.scratch as SglContext).galerkinElementBuffer[rowStart + j];
        if (elementBase instanceof GalerkinElement) {
          const elem = elementBase as GalerkinElement;
          elem.scratchVisibilityUsageCounter++;
        }
      }
    }
  }
}
