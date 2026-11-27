import { MediaEntity } from "./MediaEntity.js";
import type { RGBPixel } from "./RGBPixel.js";
import { ColorRgb } from "../common/color/ColorRgb.js";
import { unsigned, byte } from "./_pixel.js";
export abstract class Image extends MediaEntity {
    public abstract init(w: number, h: number): boolean;
    public abstract initNoFill(w: number, h: number): boolean;
    public abstract getXSize(): number;
    public abstract getYSize(): number;
    public abstract putPixelRgb(x: number, y: number, p: RGBPixel): void;
    public abstract getPixelRgb(x: number, y: number): RGBPixel;
    public abstract getPixelRgb(x: number, y: number, p: RGBPixel): void;
    public getPixel8bitGrayScale(x: number, y: number): number {
        const p = this.getPixelRgb(x, y);
        return byte((unsigned(p.r) + unsigned(p.g) + unsigned(p.b)) / 3);
    }
    public getColorRgbNearest(x: number, y: number): ColorRgb {
        const i = Math.floor((x - Math.floor(x)) * (this.getXSize() - 1));
        const j = Math.floor((y - Math.floor(y)) * (this.getYSize() - 1));
        const p = this.getPixelRgb(i, j);
        return new ColorRgb(unsigned(p.r) / 255, unsigned(p.g) / 255, unsigned(p.b) / 255);
    }
    public getColorRgbBiLinear(x: number, y: number): ColorRgb {
        const w = this.getXSize(),
            h = this.getYSize(),
            u = x - Math.floor(x),
            v = y - Math.floor(y),
            U = u * w,
            V = v * h,
            i0 = ((Math.floor(U) % w) + w) % w,
            j0 = ((Math.floor(V) % h) + h) % h,
            i1 = (i0 + 1) % w,
            j1 = (j0 + 1) % h,
            du = U - i0,
            dv = V - j0;
        const c = (i: number, j: number) => {
            const p = this.getPixelRgb(i, j);
            return new ColorRgb(unsigned(p.r) / 255, unsigned(p.g) / 255, unsigned(p.b) / 255);
        };
        const a = c(i0, j0),
            b = c(i1, j0),
            d = c(i0, j1),
            e = c(i1, j1),
            q = (m: number, n: number, o: number, z: number) =>
                m + du * (n - m) + dv * (o + du * (z - o) - m - du * (n - m));
        return new ColorRgb(
            q(a.r(), b.r(), d.r(), e.r()),
            q(a.g(), b.g(), d.g(), e.g()),
            q(a.b(), b.b(), d.b(), e.b()),
        );
    }
    public createTestPattern(): void {
        const p = this.getPixelRgb(0, 0);
        for (let i = 0; i < this.getXSize(); i++)
            for (let j = 0; j < this.getYSize(); j++) {
                p.r = p.g = p.b = (i % 2 !== 0 && j % 2 === 0) || (j % 2 !== 0 && i % 2 === 0) ? -1 : 0;
                if (j === Math.trunc(this.getYSize() / 2)) {
                    p.r = -1;
                    p.g = p.b = 0;
                }
                if (i === Math.trunc(this.getXSize() / 2)) {
                    p.r = p.b = 0;
                    p.g = -1;
                }
                this.putPixelRgb(i, j, p);
            }
    }
}
