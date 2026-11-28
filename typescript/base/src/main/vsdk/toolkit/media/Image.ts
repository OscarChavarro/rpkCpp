import { MediaEntity } from "./MediaEntity.js";
import type { RGBPixel } from "./RGBPixel.js";
import { ColorRgb } from "../common/color/ColorRgb.js";
import { unsigned, byte, javaIntCast, positiveMod } from "./_pixel.js";
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
            i0 = positiveMod(javaIntCast(Math.floor(U)), w),
            j0 = positiveMod(javaIntCast(Math.floor(V)), h),
            i1 = positiveMod(i0 + 1, w),
            j1 = positiveMod(j0 + 1, h),
            du = U - i0,
            dv = V - j0;
        const c = (i: number, j: number) => {
            const p = this.getPixelRgb(i, j);
            return new ColorRgb(unsigned(p.r) / 255, unsigned(p.g) / 255, unsigned(p.b) / 255);
        };
        const F00 = c(i0, j0),
            F10 = c(i1, j0),
            F01 = c(i0, j1),
            F11 = c(i1, j1);

        // Java's two horizontal interpolations, then the vertical one; the
        // grouping is kept so that every rounding step matches
        const FU0 = new ColorRgb(
            F00.r() + du * (F10.r() - F00.r()),
            F00.g() + du * (F10.g() - F00.g()),
            F00.b() + du * (F10.b() - F00.b()),
        );

        const FU1 = new ColorRgb(
            F01.r() + du * (F11.r() - F01.r()),
            F01.g() + du * (F11.g() - F01.g()),
            F01.b() + du * (F11.b() - F01.b()),
        );

        return new ColorRgb(
            FU0.r() + dv * (FU1.r() - FU0.r()),
            FU0.g() + dv * (FU1.g() - FU0.g()),
            FU0.b() + dv * (FU1.b() - FU0.b()),
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
