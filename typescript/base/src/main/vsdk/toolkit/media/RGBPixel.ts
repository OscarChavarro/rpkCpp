import { MediaEntity } from "./MediaEntity.js";
import { ColorRgb } from "../common/color/ColorRgb.js";
import { byte, unsigned } from "./_pixel.js";
export class RGBPixel extends MediaEntity {
    public r = 0;
    public g = 0;
    public b = 0;
    public setR(v: number): void {
        this.r = byte(v);
    }
    public getR(): number {
        return this.r;
    }
    public setG(v: number): void {
        this.g = byte(v);
    }
    public getG(): number {
        return this.g;
    }
    public setB(v: number): void {
        this.b = byte(v);
    }
    public getB(): number {
        return this.b;
    }
    public importFromColorRgb(c: ColorRgb): void {
        this.r = byte(Math.max(0, Math.min(1, c.r())) * 255);
        this.g = byte(Math.max(0, Math.min(1, c.g())) * 255);
        this.b = byte(Math.max(0, Math.min(1, c.b())) * 255);
    }
    public isBlack(): boolean {
        return this.r === 0 && this.g === 0 && this.b === 0;
    }
    public override toString(): string {
        return `<${unsigned(this.r)}, ${unsigned(this.g)}, ${unsigned(this.b)}>`;
    }
}
