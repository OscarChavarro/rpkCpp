import { CLONEABLE, type Cloneable } from "../../../java/lang/Cloneable.js";
import { Image } from "./Image.js";
import { IllegalArgumentException } from "../../../java/lang/IllegalArgumentException.js";
import { RGBPixel } from "./RGBPixel.js";
import { byte, checkBufferIndex } from "./_pixel.js";
import { RGBAImageUncompressed } from "./RGBAImageUncompressed.js";
import { RGBAPixel } from "./RGBAPixel.js";
export class RGBImageUncompressed extends Image implements Cloneable {
    // Java `implements Cloneable`, as a run-time marker
    public static readonly [CLONEABLE] = true;

    private data: Uint8Array | null = null;
    private xSize = 0;
    private ySize = 0;
    private rowStride = 0;
    public dettach(): void {
        this.data = null;
    }
    public override getSizeInBytes(): number {
        return this.xSize * this.ySize * 3 + 16;
    }
    public init(w: number, h: number): boolean {
        return this.allocate(w, h);
    }
    public initNoFill(w: number, h: number): boolean {
        return this.data !== null && w === this.xSize && h === this.ySize ? true : this.allocate(w, h);
    }
    private allocate(w: number, h: number): boolean {
        try {
            this.data = new Uint8Array(w * h * 3);
            this.xSize = w;
            this.ySize = h;
            this.rowStride = w * 3;
            return true;
        } catch {
            this.data = null;
            return false;
        }
    }
    private at(x: number, y: number): number {
        return (this.ySize - 1 - y) * this.rowStride + x * 3;
    }
    public putPixel(x: number, y: number, r: number | RGBPixel, g?: number, b?: number): void {
        const i = this.at(x, y),
            d = this.data!;
        if (r instanceof RGBPixel) {
            checkBufferIndex(i, d.length);
            d[i] = r.r;
            checkBufferIndex(i + 1, d.length);
            d[i + 1] = r.g;
            checkBufferIndex(i + 2, d.length);
            d[i + 2] = r.b;
        } else {
            // Java only declares the `byte` overload here, which stores the
            // signed byte as given; the unsigned backing array wraps it back.
            checkBufferIndex(i, d.length);
            d[i] = r;
            checkBufferIndex(i + 1, d.length);
            d[i + 1] = g!;
            checkBufferIndex(i + 2, d.length);
            d[i + 2] = b!;
        }
    }
    public override putPixelRgb(x: number, y: number, p: RGBPixel): void {
        this.putPixel(x, y, p);
    }
    public getPixel(x: number, y: number): RGBPixel {
        const i = this.at(x, y),
            d = this.data!,
            p = new RGBPixel();
        checkBufferIndex(i, d.length);
        p.r = byte(d[i]!);
        checkBufferIndex(i + 1, d.length);
        p.g = byte(d[i + 1]!);
        checkBufferIndex(i + 2, d.length);
        p.b = byte(d[i + 2]!);
        return p;
    }
    public override getPixelRgb(x: number, y: number): RGBPixel;
    public override getPixelRgb(x: number, y: number, p: RGBPixel): void;
    public override getPixelRgb(x: number, y: number, p?: RGBPixel): RGBPixel | void {
        const v = this.getPixel(x, y);
        if (p) {
            p.r = v.r;
            p.g = v.g;
            p.b = v.b;
            return;
        }
        return v;
    }
    public getXSize(): number {
        return this.xSize;
    }
    public getYSize(): number {
        return this.ySize;
    }
    /** An independent copy of the pixel bytes, as Java's `getRawImage()`. */
    public getRawImage(): Int8Array {
        const copy: Int8Array = new Int8Array(this.getRawImageSizeInBytes());
        copy.set(new Int8Array(this.data!.buffer, this.data!.byteOffset, copy.length));
        return copy;
    }
    public getRawImageSizeInBytes(): number {
        return Math.imul(Math.imul(this.xSize, this.ySize), 3);
    }
    public getRawImageDirectBuffer(): Uint8Array {
        return this.data!;
    }
    public setRawImage(w: number, h: number, data: Int8Array | Uint8Array): void {
        if (data.length !== Math.imul(Math.imul(w, h), 3)) {
            throw new IllegalArgumentException(
                "Expected " +
                    Math.imul(Math.imul(w, h), 3) +
                    " bytes for a " +
                    w +
                    "x" +
                    h +
                    " RGB image, but got " +
                    data.length,
            );
        }
        this.initNoFill(w, h);
        this.data!.set(data);
    }
    public override clone(): RGBImageUncompressed {
        super.clone();
        //Image parentCopy = (Image)super.clone();
        const xxSize = this.getXSize();
        const yySize = this.getYSize();
        let x: number;
        let y: number;

        const copy = new RGBImageUncompressed();
        copy.init(xxSize, yySize);
        for (x = 0; x < xxSize; x++) {
            for (y = 0; y < yySize; y++) {
                copy.putPixel(x, y, this.getPixel(x, y));
            }
        }
        return copy;
    }
    public cloneToRgba(): RGBAImageUncompressed {
        const copy = new RGBAImageUncompressed();
        copy.init(this.xSize, this.ySize);
        const target = new RGBAPixel();
        for (let x = 0; x < this.xSize; x++)
            for (let y = 0; y < this.ySize; y++) {
                const source = this.getPixel(x, y);
                target.r = source.r;
                target.g = source.g;
                target.b = source.b;
                copy.putPixel(x, y, target);
            }
        return copy;
    }
    /**
    Java's current direct-buffer disposal implementation is intentionally a
    no-op; only the `Entity` event is emitted.
    */
    public override dispose(): void {
        super.dispose();
    }
}
