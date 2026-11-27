import { MediaEntity } from "./MediaEntity.js";
import { byte, unsigned } from "./_pixel.js";
export class RGBAPixel extends MediaEntity {
    public r = 0;
    public g = 0;
    public b = 0;
    public a = 0;
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
    public setA(v: number): void {
        this.a = byte(v);
    }
    public getA(): number {
        return this.a;
    }
    public override toString(): string {
        return `<${unsigned(this.r)}, ${unsigned(this.g)}, ${unsigned(this.b)}, ${unsigned(this.a)}>`;
    }
}
