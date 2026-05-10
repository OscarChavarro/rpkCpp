import { OutputStream } from "../../../../java/io/OutputStream";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Error } from "../../common/Error";
import { ToneMap } from "../../tonemap/ToneMap";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";

type PPMOutputHandleConstructor = new (outputStream: OutputStream, w: number, h: number) => ImageOutputHandle;
type PicOutputHandleConstructor = new (fileName: string, w: number, h: number) => ImageOutputHandle;

export class ImageOutputHandle {
  protected width: number;
  protected height: number;

  public driverName: string | null;
  public gamma: number[];
  public toneMapOptions: ToneMappingContext | null;

  public constructor() {
    this.width = 0;
    this.height = 0;
    this.driverName = null;
    this.gamma = [0.0, 0.0, 0.0];
    this.toneMapOptions = null;
  }

  protected init(name: string, widthValue: number, heightValue: number): void {
    this.driverName = name;
    this.width = widthValue;
    this.height = heightValue;
    this.gamma[0] = 1.0;
    this.gamma[1] = 1.0;
    this.gamma[2] = 1.0;
    this.toneMapOptions = null;
  }

  public setToneMappingContext(inToneMapOptions: ToneMappingContext): void {
    this.toneMapOptions = inToneMapOptions;
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
    return value < 0.0 ? globalThis.Math.ceil(value) : globalThis.Math.floor(value);
  }

  private static javaByte(value: number): number {
    return value & 0xFF;
  }

  protected static gammaCorrect(rgb: ColorRgb, gamma: number[]): void {
    rgb.r = gamma[0] === 1.0 ? rgb.r : globalThis.Math.pow(rgb.r, 1.0 / gamma[0]);
    rgb.g = gamma[1] === 1.0 ? rgb.g : globalThis.Math.pow(rgb.g, 1.0 / gamma[1]);
    rgb.b = gamma[2] === 1.0 ? rgb.b : globalThis.Math.pow(rgb.b, 1.0 / gamma[2]);
  }

  public writeDisplayRGB(x: Uint8Array): number;
  public writeDisplayRGB(x: number[]): number;
  public writeDisplayRGB(x: Uint8Array | number[]): number {
    if (x instanceof Uint8Array) {
      process.stderr.write(`${this.driverName} does not support display RGB output.\n`);
      return 0;
    }

    const rgbFloatArray = x;
    const rgb = new Uint8Array(3 * this.width);
    for (let i = 0; i < this.width; i++) {
      const displayRgb = new ColorRgb(
        rgbFloatArray[3 * i],
        rgbFloatArray[3 * i + 1],
        rgbFloatArray[3 * i + 2]
      );
      ImageOutputHandle.gammaCorrect(displayRgb, this.gamma);

      rgb[3 * i] = ImageOutputHandle.javaByte(ImageOutputHandle.javaIntCast(displayRgb.r * 255.0));
      rgb[3 * i + 1] = ImageOutputHandle.javaByte(ImageOutputHandle.javaIntCast(displayRgb.g * 255.0));
      rgb[3 * i + 2] = ImageOutputHandle.javaByte(ImageOutputHandle.javaIntCast(displayRgb.b * 255.0));
    }

    return this.writeDisplayRGB(rgb);
  }

  public writeRadianceRGB(rgbRadiance: ColorRgb[]): number {
    if (this.toneMapOptions === null) {
      Error.fatal(-1, "ImageOutputHandle::writeRadianceRGB", "Tone mapping context not set");
    }

    const rgb = new Uint8Array(3 * this.width);
    for (let i = 0; i < this.width; i++) {
      const displayRgb = new ColorRgb();
      ToneMap.radianceToRgb(rgbRadiance[i], displayRgb, this.toneMapOptions as ToneMappingContext);
      ImageOutputHandle.gammaCorrect(displayRgb, this.gamma);
      rgb[3 * i] = ImageOutputHandle.javaByte(ImageOutputHandle.javaIntCast(displayRgb.r * 255.0));
      rgb[3 * i + 1] = ImageOutputHandle.javaByte(ImageOutputHandle.javaIntCast(displayRgb.g * 255.0));
      rgb[3 * i + 2] = ImageOutputHandle.javaByte(ImageOutputHandle.javaIntCast(displayRgb.b * 255.0));
    }

    return this.writeDisplayRGB(rgb);
  }

  private static extensionMatches(fileExtension: string | null, expected: string): boolean {
    if (fileExtension === null) {
      return false;
    }
    return fileExtension.substring(0, 3).toLowerCase() === expected.toLowerCase();
  }

  public static imageFileExtension(fileName: string | null): string | null {
    const fileNameLength = fileName === null ? 0 : fileName.length;
    if (fileNameLength <= 0) {
      return fileName;
    }
    const safeFileName = fileName as string;

    let extensionDotIndex = fileNameLength - 1;
    while (extensionDotIndex >= 0 && safeFileName.charAt(extensionDotIndex) !== ".") {
      extensionDotIndex--;
    }

    if (extensionDotIndex < 0) {
      return safeFileName;
    }

    const fileExtension = safeFileName.substring(extensionDotIndex);
    if (
      fileExtension === ".Z" ||
      fileExtension === ".gz" ||
      fileExtension === ".bz" ||
      fileExtension === ".bz2"
    ) {
      extensionDotIndex--;
      while (extensionDotIndex >= 0 && safeFileName.charAt(extensionDotIndex) !== ".") {
        extensionDotIndex--;
      }
      if (extensionDotIndex < 0) {
        return safeFileName;
      }
    }

    return safeFileName.substring(extensionDotIndex + 1);
  }

  public static createRadianceImageOutputHandle(
    fileName: string | null,
    outputStream: OutputStream | null,
    isPipe: number,
    width: number,
    height: number
  ): ImageOutputHandle | null {
    if (outputStream !== null) {
      const fileExtension = isPipe !== 0 ? "ppm" : ImageOutputHandle.imageFileExtension(fileName);
      if (ImageOutputHandle.extensionMatches(fileExtension, "ppm")) {
        const module = require("./PPMOutputHandle") as { PPMOutputHandle: PPMOutputHandleConstructor };
        return new module.PPMOutputHandle(outputStream, width, height);
      }
      if (ImageOutputHandle.extensionMatches(fileExtension, "pic")) {
        if (isPipe !== 0) {
          Error.error(
            "createRadianceImageOutputHandle",
            "Can't write PIC output to a pipe.\n"
          );
          return null;
        }

        const module = require("./PicOutputHandle") as { PicOutputHandle: PicOutputHandleConstructor };
        return new module.PicOutputHandle(fileName === null ? "" : fileName, width, height);
      }

      Error.error(
        "createRadianceImageOutputHandle",
        "Can't save high dynamic range image to a '%s' file, format not supported.",
        fileExtension
      );
      return null;
    }
    return null;
  }

  public static createImageOutputHandle(
    fileName: string | null,
    outputStream: OutputStream | null,
    isPipe: number,
    width: number,
    height: number
  ): ImageOutputHandle | null {
    if (outputStream !== null) {
      const fileExtension = isPipe !== 0 ? "ppm" : ImageOutputHandle.imageFileExtension(fileName);

      if (ImageOutputHandle.extensionMatches(fileExtension, "ppm")) {
        const module = require("./PPMOutputHandle") as { PPMOutputHandle: PPMOutputHandleConstructor };
        return new module.PPMOutputHandle(outputStream, width, height);
      }

      Error.error(
        "createImageOutputHandle",
        "Can't save display-RGB images to a '%s' file, format not supported.\n",
        fileExtension
      );
      return null;
    }
    return null;
  }

  public static writeDisplayRGB(img: ImageOutputHandle, data: Uint8Array): number {
    return img.writeDisplayRGB(data);
  }

  public static deleteImageOutputHandle(img: ImageOutputHandle | null): void {
    if (img === null) {
      return;
    }

    const module = require("./PicOutputHandle") as {
      PicOutputHandle: new (...args: any[]) => { closeHandle: () => void };
    };
    if (img instanceof module.PicOutputHandle) {
      (img as unknown as { closeHandle: () => void }).closeHandle();
    }
  }
}
