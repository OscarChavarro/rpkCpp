import { MediaEntity } from "./MediaEntity.js";
import { Vector3Dd } from "../common/linealAlgebra/Vector3Dd.js";
import { RGBImageUncompressed } from "./RGBImageUncompressed.js";
import { RGBAImageUncompressed } from "./RGBAImageUncompressed.js";
import { IndexedColorImageUncompressed } from "./IndexedColorImageUncompressed.js";
import { byte } from "./_pixel.js";
import { VSDK } from "../common/VSDK.js";
export class NormalMap extends MediaEntity {
    private xSize = 0;
    private ySize = 0;
    private data: Vector3Dd[] | null = null;
    private bumpMapScale = new Vector3Dd(1, 1, 1);
    public init(w: number, h: number): boolean {
        try {
            this.data = Array.from({ length: w * h }, () => new Vector3Dd());
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
    public getBumpMapScale(): Vector3Dd {
        return Vector3Dd.copyOf(this.bumpMapScale);
    }
    public putNormal(x: number, y: number, n: Vector3Dd): void {
        if (x < 0 || y < 0 || x >= this.xSize || y >= this.ySize) return;
        this.data![y * this.xSize + x] = Vector3Dd.copyOf(n);
    }
    public getNormal(x: number, y: number): Vector3Dd | null {
        if (!Number.isInteger(x) || !Number.isInteger(y)) return this.getNormalAt(x, y);
        if (x < 0 || y < 0 || x >= this.xSize || y >= this.ySize) return null;
        return Vector3Dd.copyOf(this.data![y * this.xSize + x]!);
    }
    public getNormalAt(x: number, y: number): Vector3Dd {
        const w = this.xSize,
            h = this.ySize,
            U = (x - Math.floor(x)) * w,
            V = (y - Math.floor(y)) * h,
            i0 = ((Math.floor(U) % w) + w) % w,
            j0 = ((Math.floor(V) % h) + h) % h,
            i1 = (i0 + 1) % w,
            j1 = (j0 + 1) % h,
            du = U - i0,
            dv = V - j0,
            a = this.getNormal(i0, j0)!,
            b = this.getNormal(i1, j0)!,
            c = this.getNormal(i0, j1)!,
            d = this.getNormal(i1, j1)!;
        return a.add(b.subtract(a).multiply(du)).add(
            c
                .add(d.subtract(c).multiply(du))
                .subtract(a.add(b.subtract(a).multiply(du)))
                .multiply(dv),
        );
    }
    private map(n: Vector3Dd): [number, number, number] {
        const q = n.normalized();
        return [byte((q.x() + 1) * 127.5), byte((q.y() + 1) * 127.5), byte((q.z() + 1) * 127.5)];
    }
    public exportToRgbImage(): RGBImageUncompressed {
        const o = new RGBImageUncompressed();
        o.init(this.xSize, this.ySize);
        for (let y = 0; y < this.ySize; y++)
            for (let x = 0; x < this.xSize; x++) o.putPixel(x, y, ...this.map(this.getNormal(x, y)!));
        return o;
    }
    public exportToRgbaImage(): RGBAImageUncompressed {
        const o = new RGBAImageUncompressed();
        o.init(this.xSize, this.ySize);
        for (let y = 0; y < this.ySize; y++)
            for (let x = 0; x < this.xSize; x++) o.putPixel(x, y, ...this.map(this.getNormal(x, y)!), -1);
        return o;
    }
    public exportToRgbImageGradient(): RGBImageUncompressed {
        const o = new RGBImageUncompressed();
        o.init(this.xSize, this.ySize);
        for (let y = 0; y < this.ySize; y++)
            for (let x = 0; x < this.xSize; x++) {
                const c = byte((1 - this.getNormal(x, y)!.normalized().z()) * 255);
                o.putPixel(x, y, c, c, c);
            }
        return o;
    }
    public exportToRgbaImageGradient(): RGBAImageUncompressed {
        const o = new RGBAImageUncompressed();
        o.init(this.xSize, this.ySize);
        for (let y = 0; y < this.ySize; y++)
            for (let x = 0; x < this.xSize; x++) {
                const c = byte((1 - this.getNormal(x, y)!.normalized().z()) * 255);
                o.putPixel(x, y, c, c, c, -128);
            }
        return o;
    }
    public importBumpMap(input: IndexedColorImageUncompressed, scale: Vector3Dd): Vector3Dd {
        const w = input.getXSize(),
            h = input.getYSize();
        if (scale.x() < VSDK.EPSILON || scale.y() < VSDK.EPSILON || scale.z() < VSDK.EPSILON) {
            const r = w / h;
            scale = r < 1 ? new Vector3Dd(1, 1 / r, 1) : new Vector3Dd(r, 1, 1);
        }
        this.bumpMapScale = Vector3Dd.copyOf(scale);
        this.init(w, h);
        for (let u = 1; u < w - 1; u++)
            for (let v = 1; v < h - 1; v++) {
                const n = new Vector3Dd(2, 0, (input.getPixel(u + 1, v) - input.getPixel(u - 1, v)) / 255).crossProduct(
                    new Vector3Dd(0, 2, (input.getPixel(u, v - 1) - input.getPixel(u, v + 1)) / 255),
                );
                this.putNormal(
                    u,
                    v,
                    new Vector3Dd(n.x() * scale.x(), n.y() * scale.y(), n.z() * scale.z()).normalized(),
                );
            }
        for (let u = 0; u < w; u++) {
            this.putNormal(u, 0, this.getNormal(u, 1)!);
            this.putNormal(u, h - 2, this.getNormal(u, h - 3)!);
            this.putNormal(u, h - 1, this.getNormal(u, h - 2)!);
        }
        for (let v = 0; v < h; v++) {
            this.putNormal(1, v, this.getNormal(2, v)!);
            this.putNormal(0, v, this.getNormal(1, v)!);
            this.putNormal(w - 1, v, this.getNormal(w - 2, v)!);
        }
        return scale;
    }
}
