import { JavaArray } from "../../../../../../java/lang/JavaArray.js";
/**
Immutable polygon vertex: homogeneous screen space position (sx, sy, sz, sw),
world space position (x, y, z), texture position (u, v) and color (r, g, b).
Each attribute also has an index, in that order, to select it by number in
the interpolation masks of ScreenPolygon and in getCoord.
*/
export class PolygonVertex {
    public static readonly SX = 0;
    public static readonly SY = 1;
    public static readonly SZ = 2;
    public static readonly SW = 3;
    public static readonly X = 4;
    public static readonly Y = 5;
    public static readonly Z = 6;
    public static readonly U = 7;
    public static readonly V = 8;
    public static readonly R = 9;
    public static readonly G = 10;
    public static readonly B = 11;
    public static readonly NUMBER_OF_ATTRIBUTES = 12;

    private readonly attributes: number[];

    public constructor();
    public constructor(
        inSx: number,
        inSy: number,
        inSz: number,
        inSw: number,
        inX: number,
        inY: number,
        inZ: number,
        inU: number,
        inV: number,
        inR: number,
        inG: number,
        inB: number,
    );
    public constructor(
        inSx = 0.0,
        inSy = 0.0,
        inSz = 0.0,
        inSw = 0.0,
        inX = 0.0,
        inY = 0.0,
        inZ = 0.0,
        inU = 0.0,
        inV = 0.0,
        inR = 0.0,
        inG = 0.0,
        inB = 0.0,
    ) {
        this.attributes = [inSx, inSy, inSz, inSw, inX, inY, inZ, inU, inV, inR, inG, inB];
    }

    /** Vertex with the NUMBER_OF_ATTRIBUTES attributes given in index order */
    public static fromAttributes(values: readonly number[]): PolygonVertex {
        return new PolygonVertex(
            JavaArray.get(values, PolygonVertex.SX),
            JavaArray.get(values, PolygonVertex.SY),
            JavaArray.get(values, PolygonVertex.SZ),
            JavaArray.get(values, PolygonVertex.SW),
            JavaArray.get(values, PolygonVertex.X),
            JavaArray.get(values, PolygonVertex.Y),
            JavaArray.get(values, PolygonVertex.Z),
            JavaArray.get(values, PolygonVertex.U),
            JavaArray.get(values, PolygonVertex.V),
            JavaArray.get(values, PolygonVertex.R),
            JavaArray.get(values, PolygonVertex.G),
            JavaArray.get(values, PolygonVertex.B),
        );
    }

    public getSx(): number {
        return this.attributes[PolygonVertex.SX]!;
    }

    public getSy(): number {
        return this.attributes[PolygonVertex.SY]!;
    }

    public getSz(): number {
        return this.attributes[PolygonVertex.SZ]!;
    }

    public getSw(): number {
        return this.attributes[PolygonVertex.SW]!;
    }

    public getX(): number {
        return this.attributes[PolygonVertex.X]!;
    }

    public getY(): number {
        return this.attributes[PolygonVertex.Y]!;
    }

    public getZ(): number {
        return this.attributes[PolygonVertex.Z]!;
    }

    public getU(): number {
        return this.attributes[PolygonVertex.U]!;
    }

    public getV(): number {
        return this.attributes[PolygonVertex.V]!;
    }

    public getR(): number {
        return this.attributes[PolygonVertex.R]!;
    }

    public getG(): number {
        return this.attributes[PolygonVertex.G]!;
    }

    public getB(): number {
        return this.attributes[PolygonVertex.B]!;
    }

    /** The attribute of the given index, or 0.0 outside [0, NUMBER_OF_ATTRIBUTES) */
    public getCoord(index: number): number {
        return index >= 0 && index < PolygonVertex.NUMBER_OF_ATTRIBUTES ? this.attributes[index]! : 0.0;
    }
}
