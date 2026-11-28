import { MediaEntity } from "./MediaEntity.js";
import { ColorRgb } from "../common/color/ColorRgb.js";
import { javaIntCast } from "./_pixel.js";
import { IndexOutOfBoundsException } from "../../../java/lang/IndexOutOfBoundsException.js";

/**
This class represents a color palette, as an indexed set of colors.

Java's `ArrayList.get` checks its index; `colorAt` mirrors that check with
the same exception type.
*/
export class RGBColorPalette extends MediaEntity {
    protected colors: ColorRgb[];

    /**
    Builds a default color pallete: the 256 tones gray scale
    */
    public constructor() {
        super();
        this.colors = [];
        this.init(256);
    }

    /**
    Initialize this color palette's data to a gray scale color palette with a
    number of levels specified by the input parameter.
    @param size Initial size of gray scale for current color palette's
    initialization
    */
    public init(size: number): void {
        this.colors = [];
        let i: number;
        let c: ColorRgb;

        for (i = 0; i < size; i++) {
            c = new ColorRgb(0, 0, 0);
            this.colors.push(c);
        }
        this.buildGrayLevelsTable();
    }

    /**
    Returns the size of current palette
    @return The size of current palette
    */
    public size(): number {
        return this.colors.length;
    }

    /**
    Given current RGBColorPalette assigned in memory, and withoun changing its
    size, this method change the palette's values to make it correspond to
    a gray scale.  The first values in the are near black, and the last values
    in the table are near white.
    */
    public buildGrayLevelsTable(): void {
        let val = 0;
        let i: number;

        for (i = 0; i < this.colors.length; i++) {
            const c: ColorRgb = new ColorRgb(val, val, val);
            this.colors[i] = c;
            val += 1.0 / (this.colors.length - 1);
        }
    }

    /**
    Returns the exact color in the given index color table position. Note that
    this is a direct table manipulation method, as opposed to the `evalNearest`
    and `evalLinear` methods, which are the recomended ones to use for most
    applications.
    @param i The index of the color to return
    @return The color in the index parameter position
    */
    public getColorAt(i: number): ColorRgb | null {
        if (i < 0 || i >= this.colors.length) return null;
        return this.colors[i]!;
    }

    public setColorAt(i: number, c: ColorRgb): void;
    public setColorAt(i: number, r: number, g: number, b: number): void;
    public setColorAt(i: number, a: ColorRgb | number, g?: number, b?: number): void {
        if (i < 0 || i >= this.colors.length) return;
        this.colors[i] = a instanceof ColorRgb ? a : new ColorRgb(a, g!, b!);
    }

    public addColor(c: ColorRgb): void;
    public addColor(r: number, g: number, b: number): void;
    public addColor(a: ColorRgb | number, g?: number, b?: number): void {
        this.colors.push(a instanceof ColorRgb ? a : new ColorRgb(a, g!, b!));
    }

    /**
    The parameter `t` must be between 0 and 1
    */
    public evalNearest(t: number): ColorRgb {
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;

        const N: number = this.colors.length;

        let i: number = javaIntCast(t * N);
        if (i < 0) i = 0;
        if (i >= N) i = N - 1;
        return this.colorAt(i);
    }

    /**
    The parameter `t` must be between 0 and 1.
    */
    public evalLinear(t: number): ColorRgb {
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;

        const N: number = this.colors.length - 1;

        let inf: number = javaIntCast(t * N);
        let sup: number = (inf + 1) | 0;

        const delta: number = 1 / N;
        const r: number = t - inf * delta;
        const p: number = r / delta;

        if (inf < 0) inf = 0;
        if (inf > N) inf = N;
        if (sup < 0) sup = 0;
        if (sup > N) sup = N;

        const a: ColorRgb = this.colorAt(inf);
        const b: ColorRgb = this.colorAt(sup);
        const c: ColorRgb = new ColorRgb(
            a.r() + (b.r() - a.r()) * p,
            a.g() + (b.g() - a.g()) * p,
            a.b() + (b.b() - a.b()) * p,
        );

        return c;
    }

    public selectNearestIndexToRgb(c: ColorRgb): number {
        let minDistance: number = Number.MAX_VALUE;
        let i: number;
        let currentDistance: number;
        let index = 0;

        for (i = 0; i < this.colors.length; i++) {
            currentDistance = this.colors[i]!.distance(c);
            if (currentDistance < minDistance) {
                minDistance = currentDistance;
                index = i;
            }
        }
        return index;
    }

    /** Java's `ArrayList.get(int)`, with its index check. */
    private colorAt(i: number): ColorRgb {
        if (i < 0 || i >= this.colors.length) {
            throw new IndexOutOfBoundsException("Index " + i + " out of bounds for length " + this.colors.length);
        }
        return this.colors[i]!;
    }
}
