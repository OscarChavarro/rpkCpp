import { OutputStream } from "../../../java/io/OutputStream";
import { ColorRgb } from "../common/color/ColorRgb";
import { Error } from "../common/Error";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector2D } from "../common/linealAlgebra/Vector2D";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { ImageOutputHandle } from "../io/image/ImageOutputHandle";
import { FileUncompressWrapper } from "../io/wrapper/FileUncompressWrapper";
import { Camera } from "../scene/Camera";
import { ToneMap } from "../tonemap/ToneMap";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";
import { SoftIds } from "./SoftIds";

export class ScreenBuffer {
  private radiance: ColorRgb[] | null;
  private rgbColor: ColorRgb[] | null;
  private camera: Camera;

  private synced: boolean;
  private factor: number;
  private addFactor: number;
  private rgbImage: boolean;
  private toneMapOptions: ToneMappingContext | null;

  private static copyCamera(source: Camera | null): Camera | null {
    if (source === null) {
      return null;
    }

    const target = new Camera();

    target.eyePosition.copy(source.eyePosition);
    target.lookPosition.copy(source.lookPosition);
    target.upDirection.copy(source.upDirection);
    target.viewDistance = source.viewDistance;
    target.fieldOfVision = source.fieldOfVision;
    target.horizontalFov = source.horizontalFov;
    target.verticalFov = source.verticalFov;
    target.near = source.near;
    target.far = source.far;
    target.xSize = source.xSize;
    target.ySize = source.ySize;
    target.X.copy(source.X);
    target.Y.copy(source.Y);
    target.Z.copy(source.Z);
    target.background.set(source.background.r, source.background.g, source.background.b);
    target.changed = source.changed;
    target.pixelWidth = source.pixelWidth;
    target.pixelHeight = source.pixelHeight;
    target.pixelWidthTangent = source.pixelWidthTangent;
    target.pixelHeightTangent = source.pixelHeightTangent;
    for (let i = 0; i < Camera.NUMBER_OF_VIEW_PLANES; i++) {
      target.viewPlanes[i].normal.copy(source.viewPlanes[i].normal);
      target.viewPlanes[i].d = source.viewPlanes[i].d;
    }

    return target;
  }

  private init(inCamera: Camera | null, defaultCamera: Camera | null): void {
    if (inCamera === null) {
      inCamera = defaultCamera;
    }

    if (inCamera === null) {
      Error.fatal(-1, "ScreenBuffer::init", "Camera not set");
      return;
    }

    if ((this.radiance !== null) && (inCamera.xSize !== this.camera.xSize || inCamera.ySize !== this.camera.ySize)) {
      this.rgbColor = null;
      this.radiance = null;
    }

    this.camera = ScreenBuffer.copyCamera(inCamera) as Camera;

    if (this.radiance === null) {
      this.radiance = new Array<ColorRgb>(this.camera.xSize * this.camera.ySize);
      this.rgbColor = new Array<ColorRgb>(this.camera.xSize * this.camera.ySize);
      for (let i = 0; i < this.camera.xSize * this.camera.ySize; i++) {
        this.radiance[i] = new ColorRgb();
        this.rgbColor[i] = new ColorRgb();
        this.radiance[i].clear();
        this.rgbColor[i].clear();
      }
    }

    const black = new ColorRgb(0.0, 0.0, 0.0);
    for (let i = 0; i < this.camera.xSize * this.camera.ySize; i++) {
      (this.radiance as ColorRgb[])[i].setMonochrome(0.0);
      (this.rgbColor as ColorRgb[])[i].set(black.r, black.g, black.b);
    }

    this.factor = 1.0;
    this.addFactor = 1.0;
    this.synced = true;
    this.rgbImage = false;
  }

  public constructor(camera: Camera | null, defaultCamera: Camera | null, inToneMapOptions: ToneMappingContext | null);
  public constructor(camera: Camera | null, defaultCamera: Camera | null);
  public constructor(camera: Camera | null, defaultCamera: Camera | null, inToneMapOptions: ToneMappingContext | null = null) {
    this.radiance = null;
    this.rgbColor = null;
    this.camera = new Camera();
    this.toneMapOptions = inToneMapOptions;
    this.init(camera, defaultCamera);
    this.synced = false;
    this.factor = 1.0;
    this.addFactor = 1.0;
    this.rgbImage = false;
  }

  public isRgbImage(): boolean {
    return this.rgbImage;
  }

  public copy(source: ScreenBuffer, defaultCamera: Camera | null): void {
    this.init(source.camera, defaultCamera);
    this.rgbImage = source.isRgbImage();

    for (let i = 0; i < this.camera.xSize * this.camera.ySize; i++) {
      (this.radiance as ColorRgb[])[i].set((source.radiance as ColorRgb[])[i].r, (source.radiance as ColorRgb[])[i].g, (source.radiance as ColorRgb[])[i].b);
    }
    this.synced = false;
  }

  public merge(src1: ScreenBuffer, src2: ScreenBuffer, defaultCamera: Camera | null): void {
    this.init(src1.camera, defaultCamera);
    this.rgbImage = src1.isRgbImage();

    if ((this.getHRes() !== src2.getHRes()) || (this.getVRes() !== src2.getVRes())) {
      Error.error("ScreenBuffer::merge", "Incompatible screen buffer sources");
      return;
    }

    const n = this.getVRes() * this.getHRes();
    for (let i = 0; i < n; i++) {
      (this.radiance as ColorRgb[])[i].add((src1.radiance as ColorRgb[])[i], (src2.radiance as ColorRgb[])[i]);
    }
  }

  public add(x: number, y: number, inRadiance: ColorRgb): void {
    const index = x + (this.camera.ySize - y - 1) * this.camera.xSize;
    (this.radiance as ColorRgb[])[index].addScaled((this.radiance as ColorRgb[])[index], this.addFactor, inRadiance);
    this.synced = false;
  }

  public set(x: number, y: number, inRadiance: ColorRgb): void {
    const index = x + (this.camera.ySize - y - 1) * this.camera.xSize;
    (this.radiance as ColorRgb[])[index].scaledCopy(this.addFactor, inRadiance);
    this.synced = false;
  }

  public get(x: number, y: number): ColorRgb {
    const index = x + (this.camera.ySize - y - 1) * this.camera.xSize;
    return (this.radiance as ColorRgb[])[index];
  }

  public render(): void {
    if (!this.synced) {
      this.sync();
    }

    SoftIds.softRenderPixels(
      this.camera.xSize,
      this.camera.ySize,
      this.rgbColor as ColorRgb[],
      this.requireToneMappingContext()
    );
  }

  public writeFile(ip: ImageOutputHandle | null): void;
  public writeFile(fileName: string, outputStream: OutputStream | null, isPipe: number): void;
  public writeFile(fileName: string): void;
  public writeFile(
    ipOrFileName: ImageOutputHandle | string | null,
    outputStream?: OutputStream | null,
    isPipe?: number
  ): void {
    if (typeof ipOrFileName !== "string") {
      const ip = ipOrFileName;
      if (ip === null) {
        return;
      }

      if (!this.synced) {
        this.sync();
      }

      process.stderr.write(`Writing ${ip.driverName} file ... `);

      const activeToneMapOptions = this.requireToneMappingContext();
      ip.setToneMappingContext(activeToneMapOptions);
      ip.gamma[0] = activeToneMapOptions.gamma.r;
      ip.gamma[1] = activeToneMapOptions.gamma.g;
      ip.gamma[2] = activeToneMapOptions.gamma.b;

      for (let i = this.camera.ySize - 1; i >= 0; i--) {
        if (!this.isRgbImage()) {
          const scanline = new Array<ColorRgb>(this.camera.xSize);
          const rowStart = i * this.camera.xSize;
          for (let j = 0; j < this.camera.xSize; j++) {
            scanline[j] = (this.radiance as ColorRgb[])[rowStart + j];
          }
          ip.writeRadianceRGB(scanline);
        }
        else {
          const rgbFloatArray = new Array<number>(3 * this.camera.xSize);
          const rowStart = i * this.camera.xSize;
          for (let j = 0; j < this.camera.xSize; j++) {
            const color = (this.radiance as ColorRgb[])[rowStart + j];
            const base = 3 * j;
            rgbFloatArray[base] = color.r;
            rgbFloatArray[base + 1] = color.g;
            rgbFloatArray[base + 2] = color.b;
          }
          ip.writeDisplayRGB(rgbFloatArray);
        }
      }

      process.stderr.write("done.\n");
      return;
    }

    const fileName = ipOrFileName;
    if (outputStream !== undefined) {
      if (outputStream === null) {
        return;
      }

      const ip = ImageOutputHandle.createRadianceImageOutputHandle(
        fileName,
        outputStream,
        isPipe as number,
        this.camera.xSize,
        this.camera.ySize
      );

      this.writeFile(ip);
      if (ip !== null) {
        ImageOutputHandle.deleteImageOutputHandle(ip);
      }
      return;
    }

    const isPipeArray = [0];
    const localOutputStream = FileUncompressWrapper.openOutputStreamCompressWrapper(fileName, isPipeArray);
    if (localOutputStream === null) {
      return;
    }

    this.writeFile(fileName, localOutputStream, isPipeArray[0]);
    FileUncompressWrapper.closeOutputStream(localOutputStream);
  }

  public renderScanline(y: number): void {
    y = this.camera.ySize - y - 1;

    if (!this.synced) {
      this.syncLine(y);
    }

    const scanline = new Array<ColorRgb>(this.camera.xSize);
    const rowStart = y * this.camera.xSize;
    for (let i = 0; i < this.camera.xSize; i++) {
      scanline[i] = (this.rgbColor as ColorRgb[])[rowStart + i];
    }
    SoftIds.softRenderPixels(this.camera.xSize, 1, scanline, this.requireToneMappingContext());
  }

  public sync(): void {
    const tmpRad = new ColorRgb();
    const activeToneMapOptions = this.requireToneMappingContext();

    for (let i = 0; i < this.camera.xSize * this.camera.ySize; i++) {
      tmpRad.scaledCopy(this.factor, (this.radiance as ColorRgb[])[i]);
      if (!this.isRgbImage()) {
        ToneMap.radianceToRgb(tmpRad, (this.rgbColor as ColorRgb[])[i], activeToneMapOptions);
      }
      else {
        tmpRad.set((this.rgbColor as ColorRgb[])[i].r, (this.rgbColor as ColorRgb[])[i].g, (this.rgbColor as ColorRgb[])[i].b);
      }
    }

    this.synced = true;
  }

  protected syncLine(lineNumber: number): void {
    let tmpRad = new ColorRgb();
    const activeToneMapOptions = this.requireToneMappingContext();

    for (let i = 0; i < this.camera.xSize; i++) {
      const idx = lineNumber * this.camera.xSize + i;
      tmpRad.scaledCopy(this.factor, (this.radiance as ColorRgb[])[idx]);
      if (!this.isRgbImage()) {
        ToneMap.radianceToRgb(tmpRad, (this.rgbColor as ColorRgb[])[idx], activeToneMapOptions);
      }
      else {
        tmpRad = (this.rgbColor as ColorRgb[])[idx];
      }
    }
  }

  protected requireToneMappingContext(): ToneMappingContext {
    if (this.toneMapOptions === null) {
      Error.fatal(-1, "ScreenBuffer::requireToneMappingContext", "Tone mapping context not set");
    }
    return this.toneMapOptions as ToneMappingContext;
  }

  public setToneMappingContext(inToneMapOptions: ToneMappingContext): void {
    this.toneMapOptions = inToneMapOptions;
  }

  public getScreenXMin(): number {
    return -this.camera.pixelWidth * this.camera.xSize / 2.0;
  }

  public getScreenYMin(): number {
    return -this.camera.pixelHeight * this.camera.ySize / 2.0;
  }

  public getPixXSize(): number {
    return this.camera.pixelWidth;
  }

  public getPixYSize(): number {
    return this.camera.pixelHeight;
  }

  public getPixelPoint(nx: number, ny: number, xOffset: number, yOffset: number): Vector2D;
  public getPixelPoint(nx: number, ny: number): Vector2D;
  public getPixelPoint(nx: number, ny: number, xOffset = 0.5, yOffset = 0.5): Vector2D {
    return new Vector2D(
      this.getScreenXMin() + (nx + xOffset) * this.getPixXSize(),
      this.getScreenYMin() + (ny + yOffset) * this.getPixYSize()
    );
  }

  public getPixelCenter(nx: number, ny: number): Vector2D {
    return this.getPixelPoint(nx, ny, 0.5, 0.5);
  }

  public getNx(x: number): number {
    return globalThis.Math.floor((x - this.getScreenXMin()) / this.getPixXSize());
  }

  public getNy(y: number): number {
    return globalThis.Math.floor((y - this.getScreenYMin()) / this.getPixYSize());
  }

  public getPixel(x: number, y: number, nx: number[] | null, ny: number[] | null): void {
    if (nx !== null && nx.length > 0) {
      nx[0] = this.getNx(x);
    }
    if (ny !== null && ny.length > 0) {
      ny[0] = this.getNy(y);
    }
  }

  public getPixelVector(nx: number, ny: number, xOffset: number, yOffset: number): Vector3D;
  public getPixelVector(nx: number, ny: number): Vector3D;
  public getPixelVector(nx: number, ny: number, xOffset = 0.5, yOffset = 0.5): Vector3D {
    const pix = this.getPixelPoint(nx, ny, xOffset, yOffset);
    const dir = new Vector3D();
    dir.combine3(this.camera.Z, pix.x, this.camera.X, pix.y, this.camera.Y);
    return dir;
  }

  public getHRes(): number {
    return this.camera.xSize;
  }

  public getVRes(): number {
    return this.camera.ySize;
  }

  public static computeFluxToRadFactor(camera: Camera, pixX: number, pixY: number): number {
    const dir = new Vector3D();
    const h = camera.pixelWidth;
    const v = camera.pixelHeight;

    const x = -h * camera.xSize / 2.0 + pixX * h;
    const y = -v * camera.ySize / 2.0 + pixY * v;

    const xSample = x + h * 0.5;
    const ySample = y + v * 0.5;

    dir.combine3(camera.Z, xSample, camera.X, ySample, camera.Y);
    const distPixel2 = dir.norm2();
    const distPixel = globalThis.Math.sqrt(distPixel2);
    dir.inverseScaledCopy(distPixel, dir, Numeric.EPSILON_FLOAT);

    let factor = 1.0 / (h * v);
    factor *= distPixel2;
    factor /= globalThis.Math.pow(dir.dotProduct(camera.Z), 2);

    return factor;
  }

  public getScreenXMax(): number {
    return this.camera.pixelWidth * this.camera.xSize / 2.0;
  }

  public getScreenYMax(): number {
    return this.camera.pixelHeight * this.camera.ySize / 2.0;
  }

  public getBiLinear(x: number, y: number): ColorRgb {
    const nx0v = [0];
    const ny0v = [0];
    let nx1: number;
    let ny1: number;
    let center: Vector2D;
    const color = new ColorRgb();

    this.getPixel(x, y, nx0v, ny0v);
    const nx0 = nx0v[0];
    const ny0 = ny0v[0];
    center = this.getPixelCenter(nx0, ny0);

    x = (x - center.x) / this.getPixXSize();
    y = (y - center.y) / this.getPixYSize();

    if (x < 0) {
      x = -x;
      nx1 = globalThis.Math.max(nx0 - 1, 0);
    }
    else {
      nx1 = globalThis.Math.min(this.getHRes(), nx0 + 1);
    }

    if (y < 0) {
      y = -y;
      ny1 = globalThis.Math.max(ny0 - 1, 0);
    }
    else {
      ny1 = globalThis.Math.min(this.getVRes(), ny0 + 1);
    }

    const c0 = this.get(nx0, ny0);
    const c1 = this.get(nx1, ny0);
    const c2 = this.get(nx1, ny1);
    const c3 = this.get(nx0, ny1);

    color.interpolateBiLinear(c0, c1, c2, c3, x, y);
    return color;
  }

  public scaleRadiance(inFactor: number): void {
    for (let i = 0; i < this.camera.xSize * this.camera.ySize; i++) {
      (this.radiance as ColorRgb[])[i].scale(inFactor);
    }
    this.synced = false;
  }

  public setAddScaleFactor(inFactor: number): void {
    this.addFactor = inFactor;
  }

  public setFactor(inFactor: number): void {
    this.factor = inFactor;
  }

  public setRgbImage(isRGB: boolean): void {
    this.rgbImage = isRGB;
  }
}
