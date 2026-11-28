import { CLONEABLE, type Cloneable } from "../../../java/lang/Cloneable.js";
import { Image } from "./Image.js";
import { IllegalArgumentException } from "../../../java/lang/IllegalArgumentException.js";
import { RGBPixel } from "./RGBPixel.js";
import { RGBAPixel } from "./RGBAPixel.js";
import { RGBImageUncompressed } from "./RGBImageUncompressed.js";
import { byte, checkBufferIndex } from "./_pixel.js";
export class RGBAImageUncompressed extends Image implements Cloneable {
    // Java `implements Cloneable`, as a run-time marker
    public static readonly [CLONEABLE] = true;

    private data: Uint8Array | null = null;
    private xSize = 0;
    private ySize = 0;
    public dettach(): void {
        this.data = null;
    }
    public override getSizeInBytes(): number {
        return this.xSize * this.ySize * 4 + 16;
    }
    public init(w: number, h: number): boolean {
        return this.allocate(w, h);
    }
    public initNoFill(w: number, h: number): boolean {
        return this.data !== null && w === this.xSize && h === this.ySize ? true : this.allocate(w, h);
    }
    private allocate(w: number, h: number): boolean {
        try {
            this.data = new Uint8Array(w * h * 4);
            this.xSize = w;
            this.ySize = h;
            return true;
        } catch {
            this.data = null;
            return false;
        }
    }
    private at(x: number, y: number): number {
        return (this.xSize * (this.ySize - 1 - y) + x) * 4;
    }
    public putPixel(x: number, y: number, r: number | RGBAPixel, g?: number, b?: number, a?: number): void {
        const i = this.at(x, y),
            d = this.data!;
        if (r instanceof RGBAPixel) {
            checkBufferIndex(i, d.length);
            d[i] = r.r;
            checkBufferIndex(i + 1, d.length);
            d[i + 1] = r.g;
            checkBufferIndex(i + 2, d.length);
            d[i + 2] = r.b;
            checkBufferIndex(i + 3, d.length);
            d[i + 3] = r.a;
        } else {
            // Java only declares the `byte` overloads here, which store the
            // signed bytes as given; the three-argument one fills the alpha
            // channel with `(byte) -1`.
            checkBufferIndex(i, d.length);
            d[i] = r;
            checkBufferIndex(i + 1, d.length);
            d[i + 1] = g!;
            checkBufferIndex(i + 2, d.length);
            d[i + 2] = b!;
            checkBufferIndex(i + 3, d.length);
            d[i + 3] = a === undefined ? -1 : a;
        }
    }
    public override putPixelRgb(x: number, y: number, p: RGBPixel): void {
        const i = this.at(x, y),
            d = this.data!;
        checkBufferIndex(i, d.length);
        d[i] = p.r;
        checkBufferIndex(i + 1, d.length);
        d[i + 1] = p.g;
        checkBufferIndex(i + 2, d.length);
        d[i + 2] = p.b;
        checkBufferIndex(i + 3, d.length);
        d[i + 3] = 127;
    }
    public getPixel(x: number, y: number): RGBAPixel {
        const i = this.at(x, y),
            d = this.data!,
            p = new RGBAPixel();
        checkBufferIndex(i, d.length);
        p.r = byte(d[i]!);
        checkBufferIndex(i + 1, d.length);
        p.g = byte(d[i + 1]!);
        checkBufferIndex(i + 2, d.length);
        p.b = byte(d[i + 2]!);
        checkBufferIndex(i + 3, d.length);
        p.a = byte(d[i + 3]!);
        return p;
    }
    public override getPixelRgb(x: number, y: number): RGBPixel;
    public override getPixelRgb(x: number, y: number, p: RGBPixel): void;
    public override getPixelRgb(x: number, y: number, p?: RGBPixel): RGBPixel | void {
        const q = this.getPixel(x, y),
            o = new RGBPixel();
        o.r = q.r;
        o.g = q.g;
        o.b = q.b;
        if (p) {
            p.r = o.r;
            p.g = o.g;
            p.b = o.b;
            return;
        }
        return o;
    }
    public getPixelRgba(x: number, y: number, p: RGBAPixel): void {
        const q = this.getPixel(x, y);
        p.r = q.r;
        p.g = q.g;
        p.b = q.b;
        p.a = q.a;
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
        return Math.imul(Math.imul(this.xSize, this.ySize), 4);
    }
    public getRawImageDirectBuffer(): Uint8Array {
        return this.data!;
    }
    public setRawImage(w: number, h: number, data: Int8Array | Uint8Array): void {
        if (data.length !== Math.imul(Math.imul(w, h), 4)) {
            throw new IllegalArgumentException(
                "Expected " +
                    Math.imul(Math.imul(w, h), 4) +
                    " bytes for a " +
                    w +
                    "x" +
                    h +
                    " RGBA image, but got " +
                    data.length,
            );
        }
        this.initNoFill(w, h);
        this.data!.set(data);
    }
    public override clone(): RGBAImageUncompressed {
        super.clone();
        //Image parentCopy = (Image)super.clone();
        const xxSize = this.getXSize();
        const yySize = this.getYSize();
        let x: number;
        let y: number;

        const copy = new RGBAImageUncompressed();
        copy.init(xxSize, yySize);
        for (x = 0; x < xxSize; x++) {
            for (y = 0; y < yySize; y++) {
                copy.putPixel(x, y, this.getPixel(x, y));
            }
        }
        return copy;
    }
    public exportToRgbImage(): RGBImageUncompressed {
        const out = new RGBImageUncompressed();
        out.init(this.xSize, this.ySize);
        for (let x = 0; x < this.xSize; x++)
            for (let y = 0; y < this.ySize; y++) {
                const p = this.getPixel(x, y);
                out.putPixel(x, y, p.r, p.g, p.b);
            }
        return out;
    }
}
