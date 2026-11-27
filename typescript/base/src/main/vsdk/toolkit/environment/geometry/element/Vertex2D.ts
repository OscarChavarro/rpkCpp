import { FundamentalEntity } from "../../../common/FundamentalEntity.js";
import { VSDK } from "../../../common/VSDK.js";
import { ColorRgb } from "../../../common/color/ColorRgb.js";
export class Vertex2D extends FundamentalEntity {
    public color: ColorRgb;
    public constructor(
        public x: number,
        public y: number,
        r?: number,
        g?: number,
        b?: number,
    ) {
        super();
        this.color = r === undefined ? new ColorRgb() : new ColorRgb(r, g!, b!);
    }
    public override toString() {
        return `<${VSDK.formatDouble(this.x)}, ${VSDK.formatDouble(this.y)}>`;
    }
}
