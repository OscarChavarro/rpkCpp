/**
Philippe Bekaert & Jan Prikryl, October 1998 - March 2000
*/
const zlib = require("node:zlib");
import { OutputStream } from "vitral/dist/java/io/OutputStream";
import { Logger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { ImagePersistence } from "vitral/dist/vsdk/toolkit/io/image/ImagePersistence";
import { ImagePersistencePng } from "vitral/dist/vsdk/toolkit/io/image/ImagePersistencePng";
import { Image } from "vitral/dist/vsdk/toolkit/media/Image";
import { RGBImageFloatUncompressed } from "vitral/dist/vsdk/toolkit/media/RGBImageFloatUncompressed";

// VITRAL's platform-neutral PNG writer stores its data uncompressed unless a
// runtime gives it a DEFLATE implementation; as @vitral/fs does, Node's zlib is
// given here, so the PNG files are compressed as libpng and javax.imageio do in
// the C++ and Java ports (the pixels are the same either way).
ImagePersistencePng.setDeflater((rawBytes: Uint8Array): Uint8Array => {
  const compressed = zlib.deflateSync(rawBytes);
  return new Uint8Array(compressed.buffer, compressed.byteOffset, compressed.byteLength);
});

export enum ImageOutputTargetFormat {
  DISPLAY_PPM,
  DISPLAY_PNG,
  RADIANCE_RGBE,
}

/**
Destination of a rendered image: an open output stream and the file format
written on it, chosen from the extension of the file name. The images are
written by VITRAL's ImagePersistence:
- PPM and PNG: display RGB (tone mapped and gamma corrected by ScreenBuffer)
- PIC: the high dynamic range radiances, in Radiance's RGBE format
The stream belongs to the caller, which closes it when the image is written.
*/
export class ImageOutputTarget {
  public static readonly Format = ImageOutputTargetFormat;

  private readonly outputStream: OutputStream;
  private readonly format: ImageOutputTargetFormat;
  private readonly driverName: string;

  private constructor(inOutputStream: OutputStream, inFormat: ImageOutputTargetFormat, inDriverName: string) {
    this.outputStream = inOutputStream;
    this.format = inFormat;
    this.driverName = inDriverName;
  }

  private static extensionMatches(fileExtension: string | null, extension: string): boolean {
    return fileExtension !== null && fileExtension.substring(0, 3).toLowerCase() === extension;
  }

  /**
  Examines the file name extension in order to decide what file format to
  use (PPM when the stream is a pipe, unless the name is of a PNG file).
  Returns null, after reporting it, for an extension of an unsupported
  format or a null stream.
  */
  public static create(fileName: string | null, outputStream: OutputStream | null, isPipe: number): ImageOutputTarget | null {
    if (outputStream === null) {
      return null;
    }

    // Assume PPM format if pipe, unless it is a PNG file (e.g. compressed)
    let fileExtension = ImageOutputTarget.imageFileExtension(fileName);
    if (ImageOutputTarget.extensionMatches(fileExtension, "png")) {
      return new ImageOutputTarget(outputStream, ImageOutputTargetFormat.DISPLAY_PNG, "PNG");
    }
    if (isPipe !== 0) {
      fileExtension = "ppm";
    }
    if (ImageOutputTarget.extensionMatches(fileExtension, "ppm")) {
      return new ImageOutputTarget(outputStream, ImageOutputTargetFormat.DISPLAY_PPM, "PPM");
    }
    if (ImageOutputTarget.extensionMatches(fileExtension, "pic")) {
      // Olaf: HDR PIC output
      return new ImageOutputTarget(outputStream, ImageOutputTargetFormat.RADIANCE_RGBE, "high dynamic range PIC");
    }
    Logger.reportMessage(
      null,
      Logger.ERROR,
      "ImageOutputTarget::create",
      `Can't save high dynamic range image to a '${fileExtension}' file, format not supported.`
    );
    return null;
  }

  /**
  Returns file name extension. Understands extra suffixes ".Z", ".gz",
  ".bz", and ".bz2".
  */
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
    if (fileExtension === ".Z" || fileExtension === ".gz" || fileExtension === ".bz" || fileExtension === ".bz2") {
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

  public getFormat(): ImageOutputTargetFormat {
    return this.format;
  }

  public getDriverName(): string {
    return this.driverName;
  }

  public writeDisplayImage(image: Image): boolean {
    if (this.format === ImageOutputTargetFormat.DISPLAY_PNG) {
      try {
        ImagePersistence.exportPNG(this.outputStream, image);
      } catch {
        return false;
      }
      return true;
    }
    return ImagePersistence.exportPPM(this.outputStream, image);
  }

  public writeRadianceImage(image: RGBImageFloatUncompressed): boolean {
    return ImagePersistence.exportRGBE(this.outputStream, image);
  }
}
