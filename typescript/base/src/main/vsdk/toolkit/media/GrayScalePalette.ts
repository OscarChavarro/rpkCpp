import { RGBProceduralColorPalette } from "./RGBProceduralColorPalette.js";
import { ColorRgb } from "../common/color/ColorRgb.js";
export class GrayScalePalette extends RGBProceduralColorPalette {
    public override selectNearestIndexToRgb(c: ColorRgb): number {
        if (!this.pure) return super.selectNearestIndexToRgb(c);
        return Math.trunc(Math.max(0, Math.min(1, (c.r() + c.g() + c.b()) / 3)) * (this.size() - 1));
    }
}
