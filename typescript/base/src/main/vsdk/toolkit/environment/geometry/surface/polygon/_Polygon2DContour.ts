import { FundamentalEntity } from "../../../../common/FundamentalEntity.js";
import { Vertex2D } from "../../element/Vertex2D.js";
import { Double } from "../../../../../../java/lang/Double.js";

/**
Contour of a `Polygon2D`. Java's `List<Vertex2D> vertices` (an `ArrayList`)
is an array here.
*/
export class _Polygon2DContour extends FundamentalEntity {
    public vertices: Vertex2D[];

    // If this contour is a hole, exteriorContour is the contour that contains it.
    private exteriorContour: _Polygon2DContour | null;

    public fleetingFlag: boolean = false; //Caution, not a long term flag.

    public constructor() {
        super();
        this.vertices = [];
        this.exteriorContour = null;
    }

    public addVertex(x: number, y: number): void;
    public addVertex(x: number, y: number, r: number, g: number, b: number): void;
    public addVertex(x: number, y: number, r?: number, g?: number, b?: number): void {
        if (r === undefined) {
            this.vertices.push(new Vertex2D(x, y));
            return;
        }
        this.vertices.push(new Vertex2D(x, y, r, g!, b!));
    }

    public pushVertex(x: number, y: number): void {
        this.vertices.splice(0, 0, new Vertex2D(x, y));
    }

    /**
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    public getMinMax(): number[] {
        let v: Vertex2D;
        let i: number;
        let minX: number;
        let minY: number;
        let maxX: number;
        let maxY: number;

        const minMax: number[] = [0, 0, 0, 0, 0, 0];
        const size = this.vertices.length;
        if (size > 0) {
            v = this.vertices[0]!;
            minX = v.x;
            minY = v.y;
            maxX = v.x;
            maxY = v.y;
        } else {
            // To operate in the same way that others getMinMax() functions.
            minX = Double.MAX_VALUE;
            minY = Double.MAX_VALUE;
            maxX = -Double.MAX_VALUE;
            maxY = -Double.MAX_VALUE;
        }

        for (i = 1; i < size; ++i) {
            v = this.vertices[i]!;
            if (v.x > maxX) {
                maxX = v.x;
            }
            if (v.x < minX) {
                minX = v.x;
            }
            if (v.y > maxY) {
                maxY = v.y;
            }
            if (v.y < minY) {
                minY = v.y;
            }
        }
        minMax[0] = minX;
        minMax[1] = minY;
        minMax[2] = 0;
        minMax[3] = maxX;
        minMax[4] = maxY;
        minMax[5] = 0;
        return minMax;
    }

    /**
     * The variable modifyState is kept for source compatibility.
     */
    public calcMinMaxArea(_modifyState: boolean): number {
        const minMax = this.getMinMax();
        return (minMax[3]! - minMax[0]!) * (minMax[4]! - minMax[1]!);
    }

    /**
     * @return the exteriorContour
     */
    public getExteriorContour(): _Polygon2DContour | null {
        return this.exteriorContour;
    }

    /**
     * @param exteriorContour the exteriorContour to set
     */
    public setExteriorContour(exteriorContour: _Polygon2DContour | null): void {
        this.exteriorContour = exteriorContour;
    }

    /**
     * To use it efficiently(specialy, ordering a list of objects of this class),
     * call before CalcMinMaxArea() in both objects.
     */
    public compareTo(obj: _Polygon2DContour): number {
        const other: _Polygon2DContour = obj;
        const area = this.calcMinMaxArea(false);
        const otherArea = other.calcMinMaxArea(false);
        if (area === otherArea) return 0;
        if (area < otherArea) return -1;
        else return 1;
    }
}
