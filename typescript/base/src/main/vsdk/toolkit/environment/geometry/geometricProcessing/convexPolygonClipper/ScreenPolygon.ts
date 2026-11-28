import { VSDKFatalException } from "../../../../common/VSDKFatalException.js";
import { PolygonVertex } from "./PolygonVertex.js";

/**
Immutable convex polygon with homogeneous vertices, as clipped by
ConvexWindowClipper. The mask is an interpolation mask whose kth bit
indicates whether the kth attribute of a PolygonVertex is relevant. For
example, if the valid attributes are sx, sy, and sz, then set
mask = ScreenPolygon.attributeBit(PolygonVertex.SX) |
       ScreenPolygon.attributeBit(PolygonVertex.SY) |
       ScreenPolygon.attributeBit(PolygonVertex.SZ);
*/
export class ScreenPolygon {
    // Clipping an n-gon against k planes can give an (n + k)-gon
    public static readonly MAXIMUM_SIDES_PER_POLYGON = 10;

    private readonly numberOfVertices: number;
    private readonly mask: number; // Interpolation mask for vertex elems
    private readonly vertices: PolygonVertex[];

    /** The empty polygon */
    public constructor();
    /** Copies the first inNumberOfVertices vertices of inVertices */
    public constructor(inNumberOfVertices: number, inMask: number, inVertices: readonly PolygonVertex[]);
    public constructor(inNumberOfVertices = 0, inMask = 0, inVertices: readonly PolygonVertex[] = []) {
        this.numberOfVertices = inNumberOfVertices;
        this.mask = inMask;
        this.vertices = [];
        for (let i = 0; i < ScreenPolygon.MAXIMUM_SIDES_PER_POLYGON; i++) {
            this.vertices.push(new PolygonVertex());
        }
        if (inNumberOfVertices < 0 || inNumberOfVertices > ScreenPolygon.MAXIMUM_SIDES_PER_POLYGON) {
            // Runtime boundary: C++ prints to System.err and calls
            // System.exit(1); the platform-neutral base package reports the
            // message on the console and throws a VSDKFatalException.
            const message =
                "ScreenPolygon: invalid number of vertices: " +
                inNumberOfVertices +
                " (max=" +
                ScreenPolygon.MAXIMUM_SIDES_PER_POLYGON +
                ")";
            console.error(message);
            throw new VSDKFatalException(message);
        }
        for (let i = 0; i < inNumberOfVertices; i++) {
            this.vertices[i] = inVertices[i]!;
        }
    }

    public static attributeBit(attributeIndex: number): number {
        return 1 << attributeIndex;
    }

    public getNumberOfVertices(): number {
        return this.numberOfVertices;
    }

    public getMask(): number {
        return this.mask;
    }

    public getVertex(index: number): PolygonVertex {
        return this.vertices[index]!;
    }
}
