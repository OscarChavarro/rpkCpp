import { PrintStream } from "../../../java/io/PrintStream";
import { Numeric } from "./linealAlgebra/Numeric";
import { Cie } from "./Cie";

export class ColorRgb {
  public r: number;
  public g: number;
  public b: number;

  public constructor(inR = 0.0, inG = 0.0, inB = 0.0) {
    this.r = inR;
    this.g = inG;
    this.b = inB;
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
    return (this.r > -Numeric.EPSILON && this.r < Numeric.EPSILON &&
      this.g > -Numeric.EPSILON && this.g < Numeric.EPSILON &&
      this.b > -Numeric.EPSILON && this.b < Numeric.EPSILON);
  }

  public scaledCopy(a: number, c: ColorRgb): void {
    this.r = a * c.r;
    this.g = a * c.g;
    this.b = a * c.b;
  }

  public scale(a: number): void {
    this.r *= a;
    this.g *= a;
    this.b *= a;
  }

  public scalarProduct(s: ColorRgb, t: ColorRgb): void {
    this.r = s.r * t.r;
    this.g = s.g * t.g;
    this.b = s.b * t.b;
  }

  public selfScalarProduct(s: ColorRgb): void {
    this.r *= s.r;
    this.g *= s.g;
    this.b *= s.b;
  }

  public scalarProductScaled(s: ColorRgb, a: number, t: ColorRgb): void {
    this.r = s.r * a * t.r;
    this.g = s.g * a * t.g;
    this.b = s.b * a * t.b;
  }

  public add(s: ColorRgb, t: ColorRgb): void {
    this.r = s.r + t.r;
    this.g = s.g + t.g;
    this.b = s.b + t.b;
  }

  public addScaled(s: ColorRgb, a: number, t: ColorRgb): void {
    this.r = s.r + a * t.r;
    this.g = s.g + a * t.g;
    this.b = s.b + a * t.b;
  }

  public addConstant(s: ColorRgb, a: number): void {
    this.r = s.r + a;
    this.g = s.g + a;
    this.b = s.b + a;
  }

  public subtract(s: ColorRgb, t: ColorRgb): void {
    this.r = s.r - t.r;
    this.g = s.g - t.g;
    this.b = s.b - t.b;
  }

  public divide(s: ColorRgb, t: ColorRgb): void {
    this.r = (t.r !== 0.0) ? s.r / t.r : s.r;
    this.g = (t.g !== 0.0) ? s.g / t.g : s.g;
    this.b = (t.b !== 0.0) ? s.b / t.b : s.b;
  }

  public scaleInverse(scale: number, s: ColorRgb): void {
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

  public maximum(s: ColorRgb, t: ColorRgb): void {
    this.r = (s.r > t.r) ? s.r : t.r;
    this.g = (s.g > t.g) ? s.g : t.g;
    this.b = (s.b > t.b) ? s.b : t.b;
  }

  public minimum(s: ColorRgb, t: ColorRgb): void {
    this.r = (s.r < t.r) ? s.r : t.r;
    this.g = (s.g < t.g) ? s.g : t.g;
    this.b = (s.b < t.b) ? s.b : t.b;
  }

  public average(): number {
    return (this.r + this.g + this.b) / 3.0;
  }

  public gray(): number {
    return Cie.spectrumGray(this.r, this.g, this.b);
  }

  public luminance(): number {
    return Cie.spectrumLuminance(this.r, this.g, this.b);
  }

  public interpolateBarycentric(c0: ColorRgb, c1: ColorRgb, c2: ColorRgb, u: number, v: number): void {
    this.r = c0.r + u * (c1.r - c0.r) + v * (c2.r - c0.r);
    this.g = c0.g + u * (c1.g - c0.g) + v * (c2.g - c0.g);
    this.b = c0.b + u * (c1.b - c0.b) + v * (c2.b - c0.b);
  }

  public interpolateBiLinear(c0: ColorRgb, c1: ColorRgb, c2: ColorRgb, c3: ColorRgb, u: number, v: number): void {
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

  public static arrayCopy(result: ColorRgb[], source: ColorRgb[], n: number): void {
    for (let i = 0; i < n; i++) {
      if (result[i] === undefined || result[i] === null) {
        result[i] = new ColorRgb();
      }
      if (source[i] === undefined || source[i] === null) {
        result[i].clear();
      }
      else {
        result[i].set(source[i].r, source[i].g, source[i].b);
      }
    }
  }

  public static arrayAdd(result: ColorRgb[], source: ColorRgb[], n: number): void {
    for (let i = 0; i < n; i++) {
      result[i].add(result[i], source[i]);
    }
  }

  public static arrayClear(color: ColorRgb[], n: number): void {
    for (let i = 0; i < n; i++) {
      color[i].clear();
    }
  }

  public toString(): string {
    return `ColorRgb{r=${this.r}, g=${this.g}, b=${this.b}}`;
  }
}
