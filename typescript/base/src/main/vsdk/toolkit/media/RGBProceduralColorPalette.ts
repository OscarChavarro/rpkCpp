import { RGBColorPalette } from "./RGBColorPalette.js";
import { ColorRgb } from "../common/color/ColorRgb.js";
export abstract class RGBProceduralColorPalette extends RGBColorPalette {
    protected pure = true;
    public override selectNearestIndexToRgb(color: ColorRgb): number {
        return super.selectNearestIndexToRgb(color);
    }
    public override setColorAt(i: number, c: ColorRgb): void;
    public override setColorAt(i: number, r: number, g: number, b: number): void;
    public override setColorAt(i: number, a: ColorRgb | number, b?: number, c?: number): void {
        this.pure = false;
        if (a instanceof ColorRgb) super.setColorAt(i, a);
        else super.setColorAt(i, a, b!, c!);
    }
    public override addColor(c: ColorRgb): void;
    public override addColor(r: number, g: number, b: number): void;
    public override addColor(a: ColorRgb | number, b?: number, c?: number): void {
        this.pure = false;
        if (a instanceof ColorRgb) super.addColor(a);
        else super.addColor(a, b!, c!);
    }
}
