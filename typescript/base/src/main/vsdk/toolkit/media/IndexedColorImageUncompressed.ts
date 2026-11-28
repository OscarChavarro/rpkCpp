import { Image } from "./Image.js";
import { RGBColorPalette } from "./RGBColorPalette.js";
import { RGBPixel } from "./RGBPixel.js";
import { RGBAImageUncompressed } from "./RGBAImageUncompressed.js";
import { RGBImageUncompressed } from "./RGBImageUncompressed.js";
import { ColorRgb } from "../common/color/ColorRgb.js";
import { byte, checkArrayIndex, javaIntCast, unsigned } from "./_pixel.js";
import { VSDK } from "../common/VSDK.js";
export class IndexedColorImageUncompressed extends Image {
    private data: Int8Array | null = null;
    private xSize = 0;
    private ySize = 0;
    private colorTable: RGBColorPalette;
    private staticColor: ColorRgb | null = null;
    public constructor(colorTable: RGBColorPalette | null = null) {
        super();
        this.colorTable = colorTable ?? new RGBColorPalette();
    }
    public init(w: number, h: number): boolean {
        return this.allocate(w, h);
    }
    public initNoFill(w: number, h: number): boolean {
        return this.allocate(w, h);
    }
    private allocate(w: number, h: number): boolean {
        try {
            this.data = new Int8Array(w * h);
            this.xSize = w;
            this.ySize = h;
            return true;
        } catch {
            this.data = null;
            return false;
        }
    }
    public getXSize(): number {
        return this.xSize;
    }
    public getYSize(): number {
        return this.ySize;
    }
    public getRawImage(): Int8Array {
        return this.data!;
    }
    /** Java overload `putPixel(int x, int y, int val)`: clamps `val` to 0..255. */
    public putPixel(x: number, y: number, v: number): void {
        const index = this.xSize * y + x;
        checkArrayIndex(index, this.data!.length);
        this.data![index] = byte(v);
    }
    /**
    Java overload `putPixel(int x, int y, byte val)`: stores the signed byte
    as is. TypeScript numbers cannot select an overload by the Java `byte`
    type, so this overload has its own name.
    */
    public putPixelByte(x: number, y: number, v: number): void {
        const index = this.xSize * y + x;
        checkArrayIndex(index, this.data!.length);
        this.data![index] = v;
    }
    public override getPixel8bitGrayScale(x: number, y: number): number {
        checkArrayIndex(this.xSize * y + x, this.data!.length);
        return this.data![this.xSize * y + x]!;
    }
    public getPixel(x: number, y: number): number {
        const index = this.xSize * y + x;
        checkArrayIndex(index, this.data!.length);
        return unsigned(this.data![index]!);
    }
    public override getPixelRgb(x: number, y: number): RGBPixel;
    public override getPixelRgb(x: number, y: number, p: RGBPixel): void;
    public override getPixelRgb(x: number, y: number, p?: RGBPixel): RGBPixel | void {
        const index = this.xSize * y + x;
        checkArrayIndex(index, this.data!.length);
        let val: number;
        val = VSDK.signedByte2unsignedInteger(this.data![index]!) / 255.0;

        this.staticColor = this.colorTable.evalLinear(val);
        const target: RGBPixel = p ?? new RGBPixel();
        target.r = VSDK.unsigned8BitInteger2signedByte(javaIntCast(this.staticColor.r() * 255.0));
        target.g = VSDK.unsigned8BitInteger2signedByte(javaIntCast(this.staticColor.g() * 255.0));
        target.b = VSDK.unsigned8BitInteger2signedByte(javaIntCast(this.staticColor.b() * 255.0));
        if (p) {
            return;
        }
        return target;
    }
    public getColorTable(): RGBColorPalette {
        return this.colorTable;
    }
    public setColorTable(p: RGBColorPalette): void {
        this.colorTable = p;
    }
    public putPixelRgb(x: number, y: number, p: RGBPixel): void {
        this.putPixel(
            x,
            y,
            this.colorTable.selectNearestIndexToRgb(
                new ColorRgb(unsigned(p.r) / 255, unsigned(p.g) / 255, unsigned(p.b) / 255),
            ),
        );
    }
    public exportToRgbImage(): RGBImageUncompressed {
        const o = new RGBImageUncompressed();
        o.init(this.xSize, this.ySize);
        for (let x = 0; x < this.xSize; x++)
            for (let y = 0; y < this.ySize; y++) o.putPixel(x, y, this.getPixelRgb(x, y));
        return o;
    }
    public exportToRgbaImage(): RGBAImageUncompressed {
        const o = new RGBAImageUncompressed();
        o.init(this.xSize, this.ySize);
        for (let x = 0; x < this.xSize; x++)
            for (let y = 0; y < this.ySize; y++) {
                const q = this.getPixel(x, y);
                o.putPixel(x, y, q, q, q, -128);
            }
        return o;
    }
}
