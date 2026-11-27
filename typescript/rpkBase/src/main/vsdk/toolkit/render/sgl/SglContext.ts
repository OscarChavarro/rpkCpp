import { String as JavaString } from "vitral/dist/java/lang/String";
import { Logger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { Matrix4x4d } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Matrix4x4d";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Vector4Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector4Dd";
import { Element } from "../../environment/geometry/elements/Element";
import { Patch } from "../../environment/geometry/elements/Patch";
import { Poly } from "./Poly";
import { ScreenPolygon } from "./ScreenPolygon";
import { PolygonBox } from "./PolygonBox";
import { PolygonClipResult } from "./PolygonClipResult";
import { PolygonClipResultInfo } from "./PolygonClipResultInfo";
import { PolygonVertex } from "./PolygonVertex";
import { SglConstants } from "./SglConstants";
import { SglPixelContent } from "./SglPixelContent";
import { Window } from "./Window";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class SglContext {
  private static readonly IDENTITY_MATRIX = new Matrix4x4d();

  public transformStack: Matrix4x4d[];
  // Index in transformStack of the current transform (the C++ pointer into it)
  private currentTransformIndex: number;
  public clipping: boolean;
  public vp_x: number;
  public vp_y: number;
  public near: number;
  public far: number;

  public pixelData: SglPixelContent;
  public frameBuffer: number[];
  public patchBuffer: Array<Patch | null>;
  public galerkinElementBuffer: Array<Element | null>;

  public currentPixel: number;
  public currentPatch: Patch | null;
  public currentGalerkinElement: Element | null;

  public depthBuffer: number[] | null;

  public width: number;
  public height: number;
  public vp_width: number;
  public vp_height: number;

  private static clearFrameBuffer(sglContext: SglContext, backgroundColor: number): void {
    const viewportOrigin = sglContext.vp_y * sglContext.width + sglContext.vp_x;
    for (let j = 0; j < sglContext.vp_height; j++) {
      const rowStart = viewportOrigin + j * sglContext.width;
      for (let i = 0; i < sglContext.vp_width; i++) {
        const pixelIndex = rowStart + i;
        sglContext.frameBuffer[pixelIndex] = backgroundColor;
        sglContext.patchBuffer[pixelIndex] = null;
        sglContext.galerkinElementBuffer[pixelIndex] = null;
      }
    }
  }

  private static identityMatrix(): Matrix4x4d {
    return SglContext.IDENTITY_MATRIX;
  }

  public constructor(width: number, height: number) {
    this.transformStack = new Array<Matrix4x4d>(SglConstants.SGL_TRANSFORM_STACK_SIZE);
    for (let i = 0; i < this.transformStack.length; i++) {
      this.transformStack[i] = new Matrix4x4d();
    }

    this.width = width;
    this.height = height;
    this.frameBuffer = new Array<number>(width * height);
    this.patchBuffer = new Array<Patch | null>(width * height);
    this.galerkinElementBuffer = new Array<Element | null>(width * height);
    for (let i = 0; i < width * height; i++) {
      this.frameBuffer[i] = 0;
      this.patchBuffer[i] = null;
      this.galerkinElementBuffer[i] = null;
    }

    this.pixelData = SglPixelContent.PIXEL;
    this.depthBuffer = null;

    this.currentTransformIndex = 0;
    this.transformStack[this.currentTransformIndex] = SglContext.identityMatrix();

    this.currentPixel = 0;
    this.currentPatch = null;
    this.currentGalerkinElement = null;

    this.clipping = true;
    this.vp_x = 0;
    this.vp_y = 0;
    this.vp_width = width;
    this.vp_height = height;
    this.near = 0.0;
    this.far = 1.0;
  }

  public sglClearZBuffer(defZVal: number): void {
    const viewportOrigin = this.vp_y * this.width + this.vp_x;
    for (let j = 0; j < this.vp_height; j++) {
      const rowStart = viewportOrigin + j * this.width;
      for (let i = 0; i < this.vp_width; i++) {
        (this.depthBuffer as number[])[rowStart + i] = defZVal;
      }
    }
  }

  public sglClear(backgroundColor: number, defZVal: number): void {
    SglContext.clearFrameBuffer(this, backgroundColor);
    this.sglClearZBuffer(defZVal);
  }

  public sglDepthTesting(on: boolean): void {
    if (on) {
      if (this.depthBuffer !== null) {
        return;
      }
      this.depthBuffer = new Array<number>(this.width * this.height).fill(0);
    }
    else {
      if (this.depthBuffer !== null) {
        this.depthBuffer = null;
      }
      else {
        return;
      }
    }
  }

  public sglClipping(on: boolean): void {
    this.clipping = on;
  }

  public sglLoadMatrix(xf: Matrix4x4d): void {
    this.transformStack[this.currentTransformIndex] = xf;
  }

  public sglMultiplyMatrix(xf: Matrix4x4d): void {
    this.transformStack[this.currentTransformIndex] = this.transformStack[this.currentTransformIndex]!.multiply(xf);
  }

  public sglSetPatch(patch: Patch | null): void {
    this.pixelData = SglPixelContent.PATCH_POINTER;
    this.currentPatch = patch;
  }

  public sglSetGalerkinElement(galerkinElement: Element | null): void {
    this.pixelData = SglPixelContent.ELEMENT_POINTER;
    this.currentGalerkinElement = galerkinElement;
  }

  public sglViewport(x: number, y: number, viewPortWidth: number, viewPortHeight: number): void {
    this.vp_x = x;
    this.vp_y = y;
    this.vp_width = viewPortWidth;
    this.vp_height = viewPortHeight;
  }

  public sglPolygon(numberOfVertices: number, vertices: Vector3Dd[]): void {
    const pol = new ScreenPolygon();
    const win = new Window();
    const clipBox = new PolygonBox(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    if (numberOfVertices > (this.clipping ? (PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON - 6) : PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON)) {
      Logger.reportMessage(this, Logger.ERROR, "sglPolygon", JavaString.vformat("Too many vertices (max. %d)", [PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON]));
      return;
    }

    for (let i = 0; i < numberOfVertices; i++) {
      const vertex: PolygonVertex = pol.vertices[i]!;
      const v = this.transformStack[this.currentTransformIndex]!.multiply(
        new Vector4Dd(vertices[i]!.x(), vertices[i]!.y(), vertices[i]!.z(), 1.0));
      if (v.w() > -VSDK.EPSILON && v.w() < VSDK.EPSILON) {
        return;
      }
      vertex.sx = v.x();
      vertex.sy = v.y();
      vertex.sz = v.z();
      vertex.sw = v.w();
    }
    pol.n = numberOfVertices;
    pol.mask = 0;

    if (this.clipping) {
      pol.mask = Poly.mask(0) |
        Poly.mask(Float64Array.BYTES_PER_ELEMENT) |
        Poly.mask(2 * Float64Array.BYTES_PER_ELEMENT) |
        Poly.mask(3 * Float64Array.BYTES_PER_ELEMENT);
      if (Poly.clipToBox(pol, clipBox) === PolygonClipResult.POLY_CLIP_OUT) {
        return;
      }
    }

    for (let i = 0; i < pol.n; i++) {
      const vertex = pol.vertices[i]!;
      vertex.sx = this.vp_x + (vertex.sx / vertex.sw + 1.0) * this.vp_width * 0.5;
      vertex.sy = this.vp_y + (vertex.sy / vertex.sw + 1.0) * this.vp_height * 0.5;
      vertex.sz = (this.near + (vertex.sz / vertex.sw + 1.0) * this.far * 0.5) * SglConstants.SGL_MAXIMUM_Z;
    }

    win.x0 = this.vp_x;
    win.y0 = this.vp_y;
    win.x1 = this.vp_x + this.vp_width - 1;
    win.y1 = this.vp_y + this.vp_height - 1;

    if (this.depthBuffer !== null) {
      Poly.scanZ(this, pol, win);
    }
    else {
      Poly.scanFlat(this, pol, win);
    }
  }
}
