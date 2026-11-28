import { OutputStream } from "vitral/dist/java/io/OutputStream";
import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { Logger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { Numeric } from "../skin/Numeric";
import { Vector2Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector2Dd";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { ImageOutputTarget } from "./ImageOutputTarget";
import { RGBImageFloatUncompressed } from "vitral/dist/vsdk/toolkit/media/RGBImageFloatUncompressed";
import { RGBImageUncompressed } from "vitral/dist/vsdk/toolkit/media/RGBImageUncompressed";
import { FileUncompressWrapper } from "../io/wrapper/FileUncompressWrapper";
import { Camera } from "../scene/Camera";
import { ToneMap } from "../tonemap/ToneMap";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";
import { SoftIds } from "./SoftIds";

export class ScreenBuffer {
  private radiance: ColorRgbMutable[] | null;
  private rgbColor: ColorRgbMutable[] | null;
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

    target.eyePosition = source.eyePosition;
    target.lookPosition = source.lookPosition;
    target.upDirection = source.upDirection;
    target.viewDistance = source.viewDistance;
    target.fieldOfVision = source.fieldOfVision;
    target.horizontalFov = source.horizontalFov;
    target.verticalFov = source.verticalFov;
    target.near = source.near;
    target.far = source.far;
    target.xSize = source.xSize;
    target.ySize = source.ySize;
    target.X = source.X;
    target.Y = source.Y;
    target.Z = source.Z;
    target.background.set(source.background.r, source.background.g, source.background.b);
    target.changed = source.changed;
    target.pixelWidth = source.pixelWidth;
    target.pixelHeight = source.pixelHeight;
    target.pixelWidthTangent = source.pixelWidthTangent;
    target.pixelHeightTangent = source.pixelHeightTangent;
    for (let i = 0; i < Camera.NUMBER_OF_VIEW_PLANES; i++) {
      target.viewPlanes[i]!.normal = source.viewPlanes[i]!.normal;
      target.viewPlanes[i]!.d = source.viewPlanes[i]!.d;
    }

    return target;
  }

  private init(inCamera: Camera | null, defaultCamera: Camera | null): void {
    if (inCamera === null) {
      inCamera = defaultCamera;
    }

    if (inCamera === null) {
      Logger.reportMessage(this, Logger.FATAL_ERROR, "ScreenBuffer::init", "Camera not set");
      return;
    }

    if ((this.radiance !== null) && (inCamera.xSize !== this.camera.xSize || inCamera.ySize !== this.camera.ySize)) {
      this.rgbColor = null;
      this.radiance = null;
    }

    this.camera = ScreenBuffer.copyCamera(inCamera) as Camera;

    if (this.radiance === null) {
      this.radiance = new Array<ColorRgbMutable>(this.camera.xSize * this.camera.ySize);
      this.rgbColor = new Array<ColorRgbMutable>(this.camera.xSize * this.camera.ySize);
      for (let i = 0; i < this.camera.xSize * this.camera.ySize; i++) {
        this.radiance[i] = new ColorRgbMutable();
        this.rgbColor[i] = new ColorRgbMutable();
        this.radiance[i]!.clear();
        this.rgbColor[i]!.clear();
      }
    }

    const black = new ColorRgbMutable(0.0, 0.0, 0.0);
    for (let i = 0; i < this.camera.xSize * this.camera.ySize; i++) {
      (this.radiance as ColorRgbMutable[])[i]!.setMonochrome(0.0);
      (this.rgbColor as ColorRgbMutable[])[i]!.set(black.r, black.g, black.b);
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
      (this.radiance as ColorRgbMutable[])[i]!.set((source.radiance as ColorRgbMutable[])[i]!.r, (source.radiance as ColorRgbMutable[])[i]!.g, (source.radiance as ColorRgbMutable[])[i]!.b);
    }
    this.synced = false;
  }

  public merge(src1: ScreenBuffer, src2: ScreenBuffer, defaultCamera: Camera | null): void {
    this.init(src1.camera, defaultCamera);
    this.rgbImage = src1.isRgbImage();

    if ((this.getHRes() !== src2.getHRes()) || (this.getVRes() !== src2.getVRes())) {
      Logger.reportMessage(this, Logger.ERROR, "ScreenBuffer::merge", "Incompatible screen buffer sources");
      return;
    }

    const n = this.getVRes() * this.getHRes();
    for (let i = 0; i < n; i++) {
      (this.radiance as ColorRgbMutable[])[i]!.add((src1.radiance as ColorRgbMutable[])[i]!, (src2.radiance as ColorRgbMutable[])[i]!);
    }
  }

  public add(x: number, y: number, inRadiance: ColorRgbMutable): void {
    const index = x + (this.camera.ySize - y - 1) * this.camera.xSize;
    (this.radiance as ColorRgbMutable[])[index]!.addScaled((this.radiance as ColorRgbMutable[])[index]!, this.addFactor, inRadiance);
    this.synced = false;
  }

  public set(x: number, y: number, inRadiance: ColorRgbMutable): void {
    const index = x + (this.camera.ySize - y - 1) * this.camera.xSize;
    (this.radiance as ColorRgbMutable[])[index]!.scaledCopy(this.addFactor, inRadiance);
    this.synced = false;
  }

  public get(x: number, y: number): ColorRgbMutable {
    const index = x + (this.camera.ySize - y - 1) * this.camera.xSize;
    return (this.radiance as ColorRgbMutable[])[index]!;
  }

  public render(): void {
    if (!this.synced) {
      this.sync();
    }

    SoftIds.softRenderPixels(
      this.camera.xSize,
      this.camera.ySize,
      this.rgbColor as ColorRgbMutable[],
      this.requireToneMappingContext()
    );
  }

  private static javaIntCast(value: number): number {
    if (Number.isNaN(value)) {
      return 0;
    }
    if (value >= 2147483647.0) {
      return 2147483647;
    }
    if (value <= -2147483648.0) {
      return -2147483648;
    }
    return globalThis.Math.trunc(value);
  }

  public static gammaCorrect(rgb: ColorRgbMutable, gamma: number[]): void {
    rgb.r = gamma[0]! === 1.0 ? rgb.r : globalThis.Math.pow(rgb.r, 1.0 / gamma[0]!);
    rgb.g = gamma[1]! === 1.0 ? rgb.g : globalThis.Math.pow(rgb.g, 1.0 / gamma[1]!);
    rgb.b = gamma[2]! === 1.0 ? rgb.b : globalThis.Math.pow(rgb.b, 1.0 / gamma[2]!);
  }

  /**
  Writes the image on the target: the radiances themselves for a high dynamic
  range format, or else display RGB values (radiances tone mapped, then gamma
  corrected). The rows are written from the top of the image, the last row of
  this buffer.
  */
  public writeFile(target: ImageOutputTarget | null): void;
  public writeFile(fileName: string, outputStream: OutputStream | null, isPipe: number): void;
  public writeFile(fileName: string): void;
  public writeFile(
    targetOrFileName: ImageOutputTarget | string | null,
    outputStream?: OutputStream | null,
    isPipe?: number
  ): void {
    if (typeof targetOrFileName !== "string") {
      const target = targetOrFileName;
      if (target === null) {
        return;
      }

      if (!this.synced) {
        this.sync();
      }

      process.stderr.write(`Writing ${target.getDriverName()} file ... `);

      const width = this.camera.xSize;
      const height = this.camera.ySize;
      const radiance = this.radiance as ColorRgbMutable[];
      if (target.getFormat() === ImageOutputTarget.Format.RADIANCE_RGBE) {
        if (this.isRgbImage()) {
          process.stderr.write(`${target.getDriverName()} does not support display RGB output.\n`);
        }
        else {
          const image = new RGBImageFloatUncompressed();
          if (image.initNoFill(width, height)) {
            for (let row = 0; row < height; row++) {
              const i = height - 1 - row;
              for (let x = 0; x < width; x++) {
                const color = radiance[i * width + x]!;
                image.putPixel(x, row, color.r, color.g, color.b);
              }
            }
            target.writeRadianceImage(image);
          }
        }
      }
      else {
        const activeToneMapOptions = this.requireToneMappingContext();
        // For default radiance -> display RGB
        const gamma = [activeToneMapOptions.gamma.r, activeToneMapOptions.gamma.g, activeToneMapOptions.gamma.b];
        const image = new RGBImageUncompressed();
        if (image.initNoFill(width, height)) {
          for (let row = 0; row < height; row++) {
            const i = height - 1 - row;
            for (let x = 0; x < width; x++) {
              const color = radiance[i * width + x]!;
              const displayRgb = new ColorRgbMutable(color.r, color.g, color.b);
              if (!this.isRgbImage()) {
                // Convert RGB radiance to display RGB
                ToneMap.radianceToRgb(color, displayRgb, activeToneMapOptions);
              }
              ScreenBuffer.gammaCorrect(displayRgb, gamma);
              image.putPixel(
                x,
                row,
                ScreenBuffer.javaIntCast(displayRgb.r * 255.0) & 0xFF,
                ScreenBuffer.javaIntCast(displayRgb.g * 255.0) & 0xFF,
                ScreenBuffer.javaIntCast(displayRgb.b * 255.0) & 0xFF
              );
            }
          }
          target.writeDisplayImage(image);
        }
      }

      process.stderr.write("done.\n");
      return;
    }

    const fileName = targetOrFileName;
    if (outputStream !== undefined) {
      if (outputStream === null) {
        return;
      }

      const target = ImageOutputTarget.create(fileName, outputStream, isPipe as number);
      this.writeFile(target);
      return;
    }

    const isPipeArray = [0];
    const localOutputStream = FileUncompressWrapper.openOutputStreamCompressWrapper(fileName, isPipeArray);
    if (localOutputStream === null) {
      return;
    }

    this.writeFile(fileName, localOutputStream, isPipeArray[0]!);
    FileUncompressWrapper.closeOutputStream(localOutputStream);
  }

  public renderScanline(y: number): void {
    y = this.camera.ySize - y - 1;

    if (!this.synced) {
      this.syncLine(y);
    }

    const scanline = new Array<ColorRgbMutable>(this.camera.xSize);
    const rowStart = y * this.camera.xSize;
    for (let i = 0; i < this.camera.xSize; i++) {
      scanline[i] = (this.rgbColor as ColorRgbMutable[])[rowStart + i]!;
    }
    SoftIds.softRenderPixels(this.camera.xSize, 1, scanline, this.requireToneMappingContext());
  }

  public sync(): void {
    const tmpRad = new ColorRgbMutable();
    const activeToneMapOptions = this.requireToneMappingContext();

    for (let i = 0; i < this.camera.xSize * this.camera.ySize; i++) {
      tmpRad.scaledCopy(this.factor, (this.radiance as ColorRgbMutable[])[i]!);
      if (!this.isRgbImage()) {
        ToneMap.radianceToRgb(tmpRad, (this.rgbColor as ColorRgbMutable[])[i]!, activeToneMapOptions);
      }
      else {
        tmpRad.set((this.rgbColor as ColorRgbMutable[])[i]!.r, (this.rgbColor as ColorRgbMutable[])[i]!.g, (this.rgbColor as ColorRgbMutable[])[i]!.b);
      }
    }

    this.synced = true;
  }

  protected syncLine(lineNumber: number): void {
    let tmpRad = new ColorRgbMutable();
    const activeToneMapOptions = this.requireToneMappingContext();

    for (let i = 0; i < this.camera.xSize; i++) {
      const idx = lineNumber * this.camera.xSize + i;
      tmpRad.scaledCopy(this.factor, (this.radiance as ColorRgbMutable[])[idx]!);
      if (!this.isRgbImage()) {
        ToneMap.radianceToRgb(tmpRad, (this.rgbColor as ColorRgbMutable[])[idx]!, activeToneMapOptions);
      }
      else {
        tmpRad = (this.rgbColor as ColorRgbMutable[])[idx]!;
      }
    }
  }

  protected requireToneMappingContext(): ToneMappingContext {
    if (this.toneMapOptions === null) {
      Logger.reportMessage(this, Logger.FATAL_ERROR, "ScreenBuffer::requireToneMappingContext", "Tone mapping context not set");
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

  public getPixelPoint(nx: number, ny: number, xOffset: number, yOffset: number): Vector2Dd;
  public getPixelPoint(nx: number, ny: number): Vector2Dd;
  public getPixelPoint(nx: number, ny: number, xOffset = 0.5, yOffset = 0.5): Vector2Dd {
    return new Vector2Dd(
      this.getScreenXMin() + (nx + xOffset) * this.getPixXSize(),
      this.getScreenYMin() + (ny + yOffset) * this.getPixYSize()
    );
  }

  public getPixelCenter(nx: number, ny: number): Vector2Dd {
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

  public getPixelVector(nx: number, ny: number, xOffset: number, yOffset: number): Vector3Dd;
  public getPixelVector(nx: number, ny: number): Vector3Dd;
  public getPixelVector(nx: number, ny: number, xOffset = 0.5, yOffset = 0.5): Vector3Dd {
    const pix = this.getPixelPoint(nx, ny, xOffset, yOffset);
    let dir = new Vector3Dd();
    dir = this.camera.Z.add(this.camera.X.multiply(pix.x)).add(this.camera.Y.multiply(pix.y));
    return dir;
  }

  public getHRes(): number {
    return this.camera.xSize;
  }

  public getVRes(): number {
    return this.camera.ySize;
  }

  public static computeFluxToRadFactor(camera: Camera, pixX: number, pixY: number): number {
    let dir = new Vector3Dd();
    const h = camera.pixelWidth;
    const v = camera.pixelHeight;

    const x = -h * camera.xSize / 2.0 + pixX * h;
    const y = -v * camera.ySize / 2.0 + pixY * v;

    const xSample = x + h * 0.5;
    const ySample = y + v * 0.5;

    dir = camera.Z.add(camera.X.multiply(xSample)).add(camera.Y.multiply(ySample));
    const distPixel2 = dir.lengthSquared();
    const distPixel = globalThis.Math.sqrt(distPixel2);
    dir = dir.multiplyByInverse(distPixel, Numeric.EPSILON);

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

  public getBiLinear(x: number, y: number): ColorRgbMutable {
    const nx0v = [0];
    const ny0v = [0];
    let nx1: number;
    let ny1: number;
    let center: Vector2Dd;
    const color = new ColorRgbMutable();

    this.getPixel(x, y, nx0v, ny0v);
    const nx0 = nx0v[0]!;
    const ny0 = ny0v[0]!;
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
      (this.radiance as ColorRgbMutable[])[i]!.scale(inFactor);
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
