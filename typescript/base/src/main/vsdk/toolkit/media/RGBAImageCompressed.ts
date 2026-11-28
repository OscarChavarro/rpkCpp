import { CLONEABLE, type Cloneable } from "../../../java/lang/Cloneable.js";
import { Entity } from "../common/Entity.js";
import { VSDK } from "../common/VSDK.js";
import { Logger } from "../common/logging/Logger.js";
import { Image } from "./Image.js";
import { RGBPixel } from "./RGBPixel.js";
import { RGBAPixel } from "./RGBAPixel.js";

/**
Compressed RGBA image blocks (DXT1, DXT3, DXT5), kept as they are read so a
renderer can upload them without decompressing.

Java keeps the blocks twice: as a `byte[]` and as a direct `ByteBuffer` copy.
Here the direct copy is a `Uint8Array`. Java's `getRawImageDirectBuffer()`
fails with a `NullPointerException` after `dettach()`; this one returns null,
which the WebGL renderer checks.
*/
export class RGBAImageCompressed extends Image implements Cloneable {
    // Java `implements Cloneable`, as a run-time marker
    public static readonly [CLONEABLE] = true;

    public static readonly COMPRESSION_UNKNOWN = 0;
    public static readonly COMPRESSION_DXT1 = 1;
    public static readonly COMPRESSION_DXT3 = 3;
    public static readonly COMPRESSION_DXT5 = 5;

    private data: Int8Array | null;
    private directData: Uint8Array | null;
    private xSize: number;
    private ySize: number;
    private compressionFormat: number;
    private compressedDataSize: number;

    public constructor() {
        super();
        this.data = null;
        this.directData = null;
        this.xSize = 0;
        this.ySize = 0;
        this.compressionFormat = RGBAImageCompressed.COMPRESSION_UNKNOWN;
        this.compressedDataSize = 0;
    }

    public dettach(): void {
        this.data = null;
        this.directData = null;
    }

    public override getSizeInBytes(): number {
        const dataSize: number = this.data === null ? 0 : this.data.length;
        return dataSize + 4 * Entity.INT_SIZE_IN_BYTES + Entity.POINTER_SIZE_IN_BYTES;
    }

    public init(width: number, height: number): boolean {
        this.xSize = width;
        this.ySize = height;
        this.compressionFormat = RGBAImageCompressed.COMPRESSION_UNKNOWN;
        this.compressedDataSize = 0;
        this.data = new Int8Array(0);
        this.directData = new Uint8Array(0);
        return true;
    }

    public initNoFill(width: number, height: number): boolean {
        return this.init(width, height);
    }

    public initCompressed(
        width: number,
        height: number,
        compressionFormat: number,
        compressedData: Int8Array | Uint8Array | null,
    ): boolean {
        if (width <= 0 || height <= 0 || compressedData === null) {
            this.data = null;
            this.directData = null;
            this.xSize = 0;
            this.ySize = 0;
            this.compressionFormat = RGBAImageCompressed.COMPRESSION_UNKNOWN;
            this.compressedDataSize = 0;
            return false;
        }
        this.xSize = width;
        this.ySize = height;
        this.compressionFormat = compressionFormat;
        this.compressedDataSize = RGBAImageCompressed.calculateTopLevelDataSize(width, height, compressionFormat);
        if (compressedData.length < this.compressedDataSize) {
            this.data = null;
            this.directData = null;
            this.xSize = 0;
            this.ySize = 0;
            this.compressionFormat = RGBAImageCompressed.COMPRESSION_UNKNOWN;
            this.compressedDataSize = 0;
            return false;
        }
        // Arrays.copyOf, then the direct buffer copy
        this.data = Int8Array.from(compressedData, (v: number): number => (v << 24) >> 24);
        this.directData = new Uint8Array(this.data.buffer.slice(0));
        return true;
    }

    public setRawImage(
        width: number,
        height: number,
        compressionFormat: number,
        compressedData: Int8Array | Uint8Array | null,
    ): void {
        this.initCompressed(width, height, compressionFormat, compressedData);
    }

    public getCompressionFormat(): number {
        return this.compressionFormat;
    }

    public getCompressedDataSize(): number {
        return this.compressedDataSize;
    }

    public getRawImage(): Int8Array | null {
        return this.data;
    }

    public getRawImageDirectBuffer(): Uint8Array | null {
        return this.directData;
    }

    public getXSize(): number {
        return this.xSize;
    }

    public getYSize(): number {
        return this.ySize;
    }

    public putPixel(x: number, y: number, r: number, g: number, b: number): void;
    public putPixel(x: number, y: number, r: number, g: number, b: number, a: number): void;
    public putPixel(x: number, y: number, p: RGBAPixel): void;
    public putPixel(_x: number, _y: number, _r: number | RGBAPixel, _g?: number, _b?: number, _a?: number): void {
        this.reportUnsupportedPixelAccess("putPixel");
    }

    public putPixelRgb(_x: number, _y: number, _p: RGBPixel): void {
        this.reportUnsupportedPixelAccess("putPixelRgb");
    }

    public getPixel(_x: number, _y: number): RGBAPixel {
        this.reportUnsupportedPixelAccess("getPixel");
        return new RGBAPixel();
    }

    public getPixelRgb(x: number, y: number): RGBPixel;
    public getPixelRgb(x: number, y: number, p: RGBPixel): void;
    public getPixelRgb(_x: number, _y: number, p?: RGBPixel): RGBPixel | void {
        this.reportUnsupportedPixelAccess("getPixelRgb");
        if (p === undefined) return new RGBPixel();
    }

    public getPixelRgba(_x: number, _y: number, _p: RGBAPixel): void {
        this.reportUnsupportedPixelAccess("getPixelRgba");
    }

    public override clone(): RGBAImageCompressed {
        const copy: RGBAImageCompressed = new RGBAImageCompressed();
        copy.initCompressed(this.xSize, this.ySize, this.compressionFormat, this.data);
        return copy;
    }

    public static calculateTopLevelDataSize(width: number, height: number, compressionFormat: number): number {
        let blockSize: number;

        if (compressionFormat === RGBAImageCompressed.COMPRESSION_DXT1) {
            blockSize = 8;
        } else if (
            compressionFormat === RGBAImageCompressed.COMPRESSION_DXT3 ||
            compressionFormat === RGBAImageCompressed.COMPRESSION_DXT5
        ) {
            blockSize = 16;
        } else {
            return 0;
        }

        const blockWidth: number = Math.max(1, Math.trunc(((width + 3) | 0) / 4));
        const blockHeight: number = Math.max(1, Math.trunc(((height + 3) | 0) / 4));
        return Math.imul(Math.imul(blockWidth, blockHeight), blockSize);
    }

    private reportUnsupportedPixelAccess(method: string): void {
        Logger.reportMessage(
            this,
            VSDK.FATAL_ERROR,
            method,
            "Pixel access is not implemented for RGBAImageCompressed.",
        );
    }
}
