import { NegativeArraySizeException } from "../../../java/lang/NegativeArraySizeException.js";
import { NullPointerException } from "../../../java/lang/NullPointerException.js";
import { ColorRgb } from "../common/color/ColorRgb.js";
import { Entity } from "../common/Entity.js";
import { Image } from "./Image.js";
import { RGBPixel } from "./RGBPixel.js";
import { byte, checkArrayIndex } from "./_pixel.js";

/**
Current class is a specific low level implementation of an uncompressed high
dynamic range RGB image: each pixel keeps its red, green and blue components
as unbounded double precision floating point values (e.g. radiance values
computed by a global illumination renderer), ordered in a sequential array
of RGB triplets, row by row from the upper left pixel, and left to right on
each row.

As an `Image`, the RGBPixel based methods map the range [0, 1] of the
components to the byte range [0, 255]: values outside [0, 1] are clamped
when reading, so this image can be given to the 8 bit exporters (PPM, PNG,
JPEG) of `ImagePersistence`. The full range of the components is kept by
`putPixel` / `getPixel` and by the high dynamic range exporters (RGBE).

Java's `serialVersionUID` has no counterpart: Java object serialization is a
runtime boundary of this port (see Phase 14 of the TypeScript port plan).
*/
export class RGBImageFloatUncompressed extends Image {
    private static readonly COMPONENTS_PER_PIXEL = 3;

    private data: Float64Array | null;
    private xSize: number;
    private ySize: number;

    public constructor() {
        super();
        this.data = null;
        this.xSize = 0;
        this.ySize = 0;
    }

    public override getSizeInBytes(): number {
        return (
            (Math.imul(
                Math.imul(Math.imul(this.xSize, this.ySize), RGBImageFloatUncompressed.COMPONENTS_PER_PIXEL),
                8,
            ) +
                Math.imul(2, Entity.INT_SIZE_IN_BYTES) +
                Entity.POINTER_SIZE_IN_BYTES) |
            0
        );
    }

    /**
    Allocates the image with all of its components set to zero
    */
    public init(width: number, height: number): boolean {
        if (!this.initNoFill(width, height)) {
            return false;
        }
        const data = this.data!;
        for (let i = 0; i < data.length; i++) {
            data[i] = 0.0;
        }
        return true;
    }

    public initNoFill(width: number, height: number): boolean {
        if (width <= 0 || height <= 0) {
            return false;
        }
        if (this.data !== null && width === this.xSize && height === this.ySize) {
            return true;
        }

        // Java evaluates the size in int arithmetic: an overflow to a negative
        // value throws NegativeArraySizeException, which is not caught below.
        const length = Math.imul(Math.imul(width, height), RGBImageFloatUncompressed.COMPONENTS_PER_PIXEL);
        if (length < 0) {
            throw new NegativeArraySizeException(String(length));
        }
        try {
            this.data = new Float64Array(length);
        } catch {
            // The OutOfMemoryError of Java: a typed array that cannot be
            // allocated throws a RangeError
            this.data = null;
            this.xSize = 0;
            this.ySize = 0;
            return false;
        }
        this.xSize = width;
        this.ySize = height;
        return true;
    }

    private pixelBaseIndex(x: number, y: number): number {
        return Math.imul((Math.imul(y, this.xSize) + x) | 0, RGBImageFloatUncompressed.COMPONENTS_PER_PIXEL);
    }

    private static componentToByte(value: number): number {
        if (!(value > 0.0)) {
            return 0;
        }
        if (value >= 1.0) {
            return byte(255);
        }
        return byte(value * 255.0);
    }

    /** The data array, with Java's NullPointerException on an unallocated image */
    private components(): Float64Array {
        if (this.data === null) {
            throw new NullPointerException();
        }
        return this.data;
    }

    public putPixel(x: number, y: number, r: number, g: number, b: number): void;
    public putPixel(x: number, y: number, color: ColorRgb): void;
    public putPixel(x: number, y: number, r: number | ColorRgb, g?: number, b?: number): void {
        if (r instanceof ColorRgb) {
            this.putPixel(x, y, r.getR(), r.getG(), r.getB());
            return;
        }
        const index = this.pixelBaseIndex(x, y);
        const data = this.components();
        checkArrayIndex(index, data.length);
        data[index] = r;
        checkArrayIndex(index + 1, data.length);
        data[index + 1] = g!;
        checkArrayIndex(index + 2, data.length);
        data[index + 2] = b!;
    }

    public getPixel(x: number, y: number): ColorRgb {
        const index = this.pixelBaseIndex(x, y);
        const data = this.components();
        checkArrayIndex(index, data.length);
        checkArrayIndex(index + 1, data.length);
        checkArrayIndex(index + 2, data.length);
        return new ColorRgb(data[index]!, data[index + 1]!, data[index + 2]!);
    }

    public override putPixelRgb(x: number, y: number, p: RGBPixel): void {
        this.putPixel(x, y, (p.r & 0xff) / 255.0, (p.g & 0xff) / 255.0, (p.b & 0xff) / 255.0);
    }

    public override getPixelRgb(x: number, y: number): RGBPixel;
    public override getPixelRgb(x: number, y: number, p: RGBPixel): void;
    public override getPixelRgb(x: number, y: number, p?: RGBPixel): RGBPixel | void {
        if (p === undefined) {
            const pixel = new RGBPixel();
            this.getPixelRgb(x, y, pixel);
            return pixel;
        }
        const index = this.pixelBaseIndex(x, y);
        const data = this.components();
        checkArrayIndex(index, data.length);
        p.r = RGBImageFloatUncompressed.componentToByte(data[index]!);
        checkArrayIndex(index + 1, data.length);
        p.g = RGBImageFloatUncompressed.componentToByte(data[index + 1]!);
        checkArrayIndex(index + 2, data.length);
        p.b = RGBImageFloatUncompressed.componentToByte(data[index + 2]!);
    }

    public getXSize(): number {
        return this.xSize;
    }

    public getYSize(): number {
        return this.ySize;
    }
}
