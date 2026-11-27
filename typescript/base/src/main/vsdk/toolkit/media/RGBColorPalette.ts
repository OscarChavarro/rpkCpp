import { MediaEntity } from "./MediaEntity.js";
import { ColorRgb } from "../common/color/ColorRgb.js";
export class RGBColorPalette extends MediaEntity {
    protected colors: ColorRgb[] = [];
    public constructor() {
        super();
        this.init(256);
    }
    public init(n: number): void {
        this.colors = Array.from({ length: n }, () => new ColorRgb());
        this.buildGrayLevelsTable();
    }
    public size(): number {
        return this.colors.length;
    }
    public buildGrayLevelsTable(): void {
        const d = 1 / (this.colors.length - 1);
        this.colors = this.colors.map((_, i) => new ColorRgb(i * d, i * d, i * d));
    }
    public getColorAt(i: number): ColorRgb | null {
        return i < 0 || i >= this.colors.length ? null : this.colors[i]!;
    }
    public setColorAt(i: number, c: ColorRgb): void;
    public setColorAt(i: number, r: number, g: number, b: number): void;
    public setColorAt(i: number, a: ColorRgb | number, b?: number, c?: number): void {
        if (i < 0 || i >= this.colors.length) return;
        this.colors[i] = a instanceof ColorRgb ? a : new ColorRgb(a, b!, c!);
    }
    public addColor(c: ColorRgb): void;
    public addColor(r: number, g: number, b: number): void;
    public addColor(a: ColorRgb | number, b?: number, c?: number): void {
        this.colors.push(a instanceof ColorRgb ? a : new ColorRgb(a, b!, c!));
    }
    public evalNearest(t: number): ColorRgb {
        t = Math.max(0, Math.min(1, t));
        return this.colors[Math.min(this.colors.length - 1, Math.floor(t * this.colors.length))]!;
    }
    public evalLinear(t: number): ColorRgb {
        t = Math.max(0, Math.min(1, t));
        const n = this.colors.length - 1,
            inf = Math.max(0, Math.min(n, Math.floor(t * n))),
            sup = Math.min(n, inf + 1),
            p = (t - inf / n) * n,
            a = this.colors[inf]!,
            b = this.colors[sup]!;
        return new ColorRgb(a.r() + (b.r() - a.r()) * p, a.g() + (b.g() - a.g()) * p, a.b() + (b.b() - a.b()) * p);
    }
    public selectNearestIndexToRgb(c: ColorRgb): number {
        let best = 0,
            d = Infinity;
        this.colors.forEach((x, i) => {
            const z = x.distance(c);
            if (z < d) {
                d = z;
                best = i;
            }
        });
        return best;
    }
}
