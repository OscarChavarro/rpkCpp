import { Image } from "./Image.js";
import { RGBPixel } from "./RGBPixel.js";
import { byte } from "./_pixel.js";
import { RGBAImageUncompressed } from "./RGBAImageUncompressed.js";
import { RGBAPixel } from "./RGBAPixel.js";
export class RGBImageUncompressed extends Image {
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
            d[i] = r.r;
            d[i + 1] = r.g;
            d[i + 2] = r.b;
        } else {
            // Java only declares the `byte` overload here, which stores the
            // signed byte as given; the unsigned backing array wraps it back.
            d[i] = r;
            d[i + 1] = g!;
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
        p.r = byte(d[i]!);
        p.g = byte(d[i + 1]!);
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
    public getRawImage(): Int8Array {
        return new Int8Array(this.data!.buffer, this.data!.byteOffset, this.data!.length);
    }
    public getRawImageDirectBuffer(): Uint8Array {
        return this.data!;
    }
    public setRawImage(w: number, h: number, data: Int8Array | Uint8Array): void {
        this.xSize = w;
        this.ySize = h;
        this.rowStride = w * 3;
        this.data = new Uint8Array(data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength));
    }
    public override clone(): RGBImageUncompressed {
        const c = new RGBImageUncompressed();
        c.init(this.xSize, this.ySize);
        c.data!.set(this.data!);
        return c;
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
