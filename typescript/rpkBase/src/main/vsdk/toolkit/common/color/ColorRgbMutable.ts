import { PrintStream } from "vitral/dist/java/io/PrintStream";
import { Cie } from "./Cie";
import { ColorRgb } from "vitral/dist/vsdk/toolkit/common/color/ColorRgb";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class ColorRgbMutable {
  public r: number;
  public g: number;
  public b: number;

  public constructor();
  public constructor(inR?: number, inG?: number, inB?: number);
  public constructor(c: ColorRgbMutable | ColorRgb);
  public constructor(inR: number | ColorRgbMutable | ColorRgb = 0.0, inG = 0.0, inB = 0.0) {
    if (typeof inR === "number") {
      this.r = inR;
      this.g = inG;
      this.b = inB;
    }
    else if (inR instanceof ColorRgbMutable) {
      this.r = inR.r;
      this.g = inR.g;
      this.b = inR.b;
    }
    else {
      this.r = inR.getR();
      this.g = inR.getG();
      this.b = inR.getB();
    }
  }

  public getR(): number {
    return this.r;
  }

  public getG(): number {
    return this.g;
  }

  public getB(): number {
    return this.b;
  }

  public clear(): void {
    this.r = 0.0;
    this.g = 0.0;
    this.b = 0.0;
  }

  public set(v1: number, v2: number, v3: number): void {
    this.r = v1;
    this.g = v2;
    this.b = v3;
  }

  public setMonochrome(v: number): void {
    this.r = v;
    this.g = v;
    this.b = v;
  }

  public isBlack(): boolean {
    return (this.r > -VSDK.EPSILON && this.r < VSDK.EPSILON &&
      this.g > -VSDK.EPSILON && this.g < VSDK.EPSILON &&
      this.b > -VSDK.EPSILON && this.b < VSDK.EPSILON);
  }

  public scaledCopy(a: number, c: ColorRgbMutable): void {
    this.r = a * c.r;
    this.g = a * c.g;
    this.b = a * c.b;
  }

  public scale(a: number): void {
    this.r *= a;
    this.g *= a;
    this.b *= a;
  }

  public scalarProduct(s: ColorRgbMutable, t: ColorRgbMutable): void {
    this.r = s.r * t.r;
    this.g = s.g * t.g;
    this.b = s.b * t.b;
  }

  public selfScalarProduct(s: ColorRgbMutable): void {
    this.r *= s.r;
    this.g *= s.g;
    this.b *= s.b;
  }

  public scalarProductScaled(s: ColorRgbMutable, a: number, t: ColorRgbMutable): void {
    this.r = s.r * a * t.r;
    this.g = s.g * a * t.g;
    this.b = s.b * a * t.b;
  }

  public add(s: ColorRgbMutable, t: ColorRgbMutable): void {
    this.r = s.r + t.r;
    this.g = s.g + t.g;
    this.b = s.b + t.b;
  }

  public addScaled(s: ColorRgbMutable, a: number, t: ColorRgbMutable): void {
    this.r = s.r + a * t.r;
    this.g = s.g + a * t.g;
    this.b = s.b + a * t.b;
  }

  public addConstant(s: ColorRgbMutable, a: number): void {
    this.r = s.r + a;
    this.g = s.g + a;
    this.b = s.b + a;
  }

  public subtract(s: ColorRgbMutable, t: ColorRgbMutable): void {
    this.r = s.r - t.r;
    this.g = s.g - t.g;
    this.b = s.b - t.b;
  }

  public divide(s: ColorRgbMutable, t: ColorRgbMutable): void {
    this.r = (t.r !== 0.0) ? s.r / t.r : s.r;
    this.g = (t.g !== 0.0) ? s.g / t.g : s.g;
    this.b = (t.b !== 0.0) ? s.b / t.b : s.b;
  }

  public scaleInverse(scale: number, s: ColorRgbMutable): void {
    const a = (scale !== 0.0) ? 1.0 / scale : 1.0;
    this.r = a * s.r;
    this.g = a * s.g;
    this.b = a * s.b;
  }

  public maximumComponent(): number {
    if (this.r > this.g) {
      return (this.r > this.b) ? this.r : this.b;
    }
    return (this.g > this.b) ? this.g : this.b;
  }

  public sumAbsComponents(): number {
    return globalThis.Math.abs(this.r) + globalThis.Math.abs(this.g) + globalThis.Math.abs(this.b);
  }

  public abs(): void {
    this.r = globalThis.Math.abs(this.r);
    this.g = globalThis.Math.abs(this.g);
    this.b = globalThis.Math.abs(this.b);
  }

  public maximum(s: ColorRgbMutable, t: ColorRgbMutable): void {
    this.r = (s.r > t.r) ? s.r : t.r;
    this.g = (s.g > t.g) ? s.g : t.g;
    this.b = (s.b > t.b) ? s.b : t.b;
  }

  public minimum(s: ColorRgbMutable, t: ColorRgbMutable): void {
    this.r = (s.r < t.r) ? s.r : t.r;
    this.g = (s.g < t.g) ? s.g : t.g;
    this.b = (s.b < t.b) ? s.b : t.b;
  }

  public average(): number {
    return (this.r + this.g + this.b) / 3.0;
  }

  public interpolateBarycentric(c0: ColorRgbMutable, c1: ColorRgbMutable, c2: ColorRgbMutable, u: number, v: number): void {
    this.r = c0.r + u * (c1.r - c0.r) + v * (c2.r - c0.r);
    this.g = c0.g + u * (c1.g - c0.g) + v * (c2.g - c0.g);
    this.b = c0.b + u * (c1.b - c0.b) + v * (c2.b - c0.b);
  }

  public interpolateBiLinear(c0: ColorRgbMutable, c1: ColorRgbMutable, c2: ColorRgbMutable, c3: ColorRgbMutable, u: number, v: number): void {
    const c = u * v;
    const bb = u - c;
    const d = v - c;

    this.r = c0.r + bb * (c1.r - c0.r) + c * (c2.r - c0.r) + d * (c3.r - c0.r);
    this.g = c0.g + bb * (c1.g - c0.g) + c * (c2.g - c0.g) + d * (c3.g - c0.g);
    this.b = c0.b + bb * (c1.b - c0.b) + c * (c2.b - c0.b) + d * (c3.b - c0.b);
  }

  public clip(): void {
    if (this.r < 0.0) {
      this.r = 0.0;
    }
    else if (this.r > 1.0) {
      this.r = 1.0;
    }

    if (this.g < 0.0) {
      this.g = 0.0;
    }
    else if (this.g > 1.0) {
      this.g = 1.0;
    }

    if (this.b < 0.0) {
      this.b = 0.0;
    }
    else if (this.b > 1.0) {
      this.b = 1.0;
    }
  }

  public print(stream: PrintStream | null): void {
    if (stream === null) {
      return;
    }
    stream.printf("%g %g %g", this.r, this.g, this.b);
  }

  public static arrayCopy(result: ColorRgbMutable[], source: ColorRgbMutable[], n: number): void {
    for (let i = 0; i < n; i++) {
      let dst = result[i];
      if (dst === undefined || dst === null) {
        result[i] = new ColorRgbMutable();
        dst = result[i];
      }
      const src = source[i];
      if (src === undefined || src === null || dst === undefined) {
        dst?.clear();
      }
      else {
        dst.set(src.r, src.g, src.b);
      }
    }
  }

  public static arrayAdd(result: ColorRgbMutable[], source: ColorRgbMutable[], n: number): void {
    for (let i = 0; i < n; i++) {
      const dst = result[i];
      const src = source[i];
      if (dst !== undefined && src !== undefined) {
        dst.add(dst, src);
      }
    }
  }

  public static arrayClear(color: ColorRgbMutable[], n: number): void {
    for (let i = 0; i < n; i++) {
      color[i]?.clear();
    }
  }

  public toString(): string {
    return `ColorRgbMutable{r=${this.r}, g=${this.g}, b=${this.b}}`;
  }

  public toImmutable(): ColorRgb {
    return new ColorRgb(this.r, this.g, this.b);
  }
}
