import { OutputStream } from "../../../../java/io/OutputStream.js";
import { InputStream } from "../../../../java/io/InputStream.js";
import { Image } from "../../media/Image.js";
import { RGBImageUncompressed } from "../../media/RGBImageUncompressed.js";
import { RGBAImageUncompressed } from "../../media/RGBAImageUncompressed.js";
import { IndexedColorImageUncompressed } from "../../media/IndexedColorImageUncompressed.js";
import { ImageNotRecognizedException } from "./ImageNotRecognizedException.js";

/**
An ImagePersistenceHelper is any class that implements an specific functionality
to aid the main ImagePersistence class, and that it depends on some
library API as such JOGL or Awt/Swing. The design pattern stablishes a bridge
between ImagePersistence and available APIs. Each derived class from
this class implements a wrapper to legacy code.

Port note: Java's counterpart also declares the `java.io.File` flavors of
`importRGB`, `importRGBA`, `importIndexedColor`, `exportGIF` and `exportPNG`.
A browser sandbox has no access to the local file system, so this
platform-neutral class declares only the stream (in-memory) operations, which
are the ones a web frontend can use to compute image data and ship it to a
remote service. The file-bound flavors belong to the runtime adapter that owns
a file system, `@vitral/fs`.
*/
export abstract class ImagePersistenceHelper {
    public rgbFormatFromInputStreamSupported(_fileExtension: string): boolean {
        return false;
    }

    public rgbaFormatFromInputStreamSupported(_fileExtension: string): boolean {
        return false;
    }

    public rgbFormatSupported(_fileExtension: string): boolean {
        return false;
    }

    public rgbaFormatSupported(_fileExtension: string): boolean {
        return false;
    }

    public indexedColorFormatSupported(_fileExtension: string): boolean {
        return false;
    }

    public gifExportSupported(): boolean {
        return false;
    }

    public jpgExportSupported(): boolean {
        return false;
    }

    public pngExportSupported(): boolean {
        return false;
    }

    public importRGB(_is: InputStream): RGBImageUncompressed {
        throw new ImageNotRecognizedException("Not implemented in helper", null);
    }

    public importRGBA(_is: InputStream): RGBAImageUncompressed {
        throw new ImageNotRecognizedException("Not implemented in helper", null);
    }

    public importIndexedColor(_is: InputStream): IndexedColorImageUncompressed {
        throw new ImageNotRecognizedException("Not implemented in helper", null);
    }

    public exportJPG(_os: OutputStream, _img: Image): void {
        throw new Error("No helper supported");
    }

    public exportPNG_24bitRgb(_os: OutputStream, _img: Image): void {
        throw new Error("No helper supported");
    }

    public exportPNG(_os: OutputStream, _img: Image): void {
        throw new Error("No helper supported");
    }
}
