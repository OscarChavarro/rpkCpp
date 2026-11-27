import { FundamentalEntity } from "../../../../common/FundamentalEntity.js";
import { Vertex2D } from "../../element/Vertex2D.js";
export class _Polygon2DContour extends FundamentalEntity {
    public vertices: Vertex2D[] = [];
    private exteriorContour: _Polygon2DContour | null = null;
    public fleetingFlag = false;
    public addVertex(x: number, y: number, r?: number, g?: number, b?: number) {
        this.vertices.push(r === undefined ? new Vertex2D(x, y) : new Vertex2D(x, y, r, g!, b!));
    }
    public pushVertex(x: number, y: number) {
        this.vertices.unshift(new Vertex2D(x, y));
    }
    public getMinMax(): Float64Array {
        if (this.vertices.length === 0) return new Float64Array([Infinity, Infinity, 0, -Infinity, -Infinity, 0]);
        let minX = this.vertices[0]!.x,
            minY = this.vertices[0]!.y,
            maxX = minX,
            maxY = minY;
        for (const v of this.vertices.slice(1)) {
            minX = Math.min(minX, v.x);
            minY = Math.min(minY, v.y);
            maxX = Math.max(maxX, v.x);
            maxY = Math.max(maxY, v.y);
        }
        return new Float64Array([minX, minY, 0, maxX, maxY, 0]);
    }
    public calcMinMaxArea(_modify: boolean) {
        const m = this.getMinMax();
        return (m[3]! - m[0]!) * (m[4]! - m[1]!);
    }
    public getExteriorContour() {
        return this.exteriorContour;
    }
    public setExteriorContour(x: _Polygon2DContour | null) {
        this.exteriorContour = x;
    }
    public compareTo(x: _Polygon2DContour) {
        return Math.sign(this.calcMinMaxArea(false) - x.calcMinMaxArea(false));
    }
}
