//= References:                                                             =
//= [SUTH1974b] Sutherland, Ivan E.; Hodgman, Gary W. "Reentrant Polygon  =
//=          Clipping", Communications of the ACM 17(1), 32-42, 1974.     =
//=          DOI: 10.1145/360767.360802.                                  =
//= [HECK1990] Heckbert, Paul S. "Generic Convex Polygon Scan Conversion   =
//=          and Clipping", Graphics Gems, ed. Andrew S. Glassner,         =
//=          Academic Press, 1990, section II.5, pp. 84-86.                 =

/**
Clipping against a convex window of [SUTH1974b]: the window is an intersection
of homogeneous half-spaces, and the single-plane routine is the one of the
homogeneous 3-D convex polygon clipper of [HECK1990] (Paul Heckbert 1985,
Dec 1989), generalized from the box planes to arbitrary planes.

Note that original algorithm is available at
https://github.com/erich666/GraphicsGems/tree/master/gems/PolyScan
*/

import { VSDKFatalException } from "../../../../common/VSDKFatalException.js";
import { HomogeneousHalfSpace } from "./HomogeneousHalfSpace.js";
import { PolygonVertex } from "./PolygonVertex.js";
import { ScreenPolygon } from "./ScreenPolygon.js";

/**
Clips a convex polygon in homogeneous coordinates against a convex window,
given as an intersection of half-spaces, after [SUTH1974b]. This layer knows nothing of clip boxes, trivial acceptance or
rasterization: those belong to the render/raster/polygonScanConverter package,
which builds on it.
It keeps no state: every call returns a new polygon.
*/
export class ConvexWindowClipper {
    private constructor() {}

    /**
    Signed plane measure of the vertex for the half-space ([SUTH1974b], p. 36):
    negative or zero inside, positive outside.
    */
    private static signedMeasure(vertex: PolygonVertex, halfSpace: HomogeneousHalfSpace): number {
        return (
            halfSpace.getXCoefficient() * vertex.getSx() +
            halfSpace.getYCoefficient() * vertex.getSy() +
            halfSpace.getZCoefficient() * vertex.getSz() +
            halfSpace.getWCoefficient() * vertex.getSw()
        );
    }

    /**
    Stops the program when q, of ScreenPolygon::MAXIMUM_SIDES_PER_POLYGON
    vertices, has no room for one more. Clipping a convex polygon against a plane
    adds at most one vertex, but an input that leaves no room for it, or one that
    is not convex, would otherwise write past the end of q.
    */
    private static checkRoomForVertex(numberOfVertices: number): void {
        if (numberOfVertices >= ScreenPolygon.MAXIMUM_SIDES_PER_POLYGON) {
            // The cpp port prints this message and exits; the TypeScript port
            // prints it on the console and throws a VSDKFatalException.
            const message =
                "ConvexWindowClipper: too many vertices: " +
                (numberOfVertices + 1) +
                " (max=" +
                ScreenPolygon.MAXIMUM_SIDES_PER_POLYGON +
                ")";
            console.error(message);
            throw new VSDKFatalException(message);
        }
    }

    /**
    Clip convex polygon p against a half-space, returning the portion satisfying
    a * sx + b * sy + c * sz + d * sw <= 0 (see HomogeneousHalfSpace). The
    attributes selected by the mask of p are interpolated at the edge crossings;
    the others are zero in the vertices created there.
    */
    public static clipToHalfSpace(p: ScreenPolygon, halfSpace: HomogeneousHalfSpace): ScreenPolygon {
        const n = p.getNumberOfVertices();
        const mask = p.getMask();
        const q: PolygonVertex[] = [];
        let qn = 0;

        if (n === 0) {
            // A previous clipping stage may have emptied the polygon; without
            // this guard, vertex n - 1 below is undefined behavior
            return new ScreenPolygon(0, mask, q);
        }

        // Close the polygon by processing the last-to-first edge first
        // ([SUTH1974b], pp. 34-35), without repeating the first input vertex.
        let previousVertexIndex = n - 1;
        let tu = ConvexWindowClipper.signedMeasure(p.getVertex(previousVertexIndex), halfSpace);
        for (let currentVertexIndex = 0; currentVertexIndex < n; currentVertexIndex++) {
            // On old polygon (p), u is previous vertex, v is current vertex
            // tv is negative if vertex v is in
            const u = p.getVertex(previousVertexIndex);
            const v = p.getVertex(currentVertexIndex);
            const tv = ConvexWindowClipper.signedMeasure(v, halfSpace);
            // The four inside/outside edge cases are those of [SUTH1974b],
            // p. 34, Fig. 2: emit an intersection on a crossing, followed by
            // the terminal vertex if it is inside. This version treats on-plane
            // vertices as inside and may emit a duplicate at an on-plane crossing.
            if ((tu <= 0.0 && tv > 0.0) || (tu > 0.0 && tv <= 0.0)) {
                // Edge crosses plane; add intersection point to q
                // Signed plane measures give the edge parameter and linear
                // intersection ([SUTH1974b], p. 36, eqs. 4-5).
                const t = tu / (tu - tv);
                const w: number[] = new Array<number>(PolygonVertex.NUMBER_OF_ATTRIBUTES).fill(0.0);
                let maskBits = mask;
                for (
                    let attributeIndex = 0;
                    maskBits !== 0 && attributeIndex < PolygonVertex.NUMBER_OF_ATTRIBUTES;
                    attributeIndex++, maskBits >>>= 1
                ) {
                    if (maskBits & 1) {
                        const uCoord = u.getCoord(attributeIndex);
                        const vCoord = v.getCoord(attributeIndex);
                        w[attributeIndex] = uCoord + t * (vCoord - uCoord);
                    }
                }
                ConvexWindowClipper.checkRoomForVertex(qn);
                q[qn++] = PolygonVertex.fromAttributes(w);
            }
            if (tv <= 0.0) {
                // Vertex v is in, copy it to q
                ConvexWindowClipper.checkRoomForVertex(qn);
                q[qn++] = v;
            }
            previousVertexIndex = currentVertexIndex;
            tu = tv;
        }
        return new ScreenPolygon(qn, mask, q);
    }

    /**
    Clip the polygon successively against the given half-spaces, in their order
    ([SUTH1974b], p. 36). Unlike the streaming reentrant pipeline of the paper,
    this version stores each complete intermediate polygon and reuses the same
    single-plane routine. The polygon must leave room for one more vertex per
    half-space.
    */
    public static clipToHalfSpaces(
        polygon: ScreenPolygon,
        halfSpaces: readonly HomogeneousHalfSpace[],
        numberOfHalfSpaces: number,
    ): ScreenPolygon {
        let clipped = polygon;
        for (let i = 0; i < numberOfHalfSpaces; i++) {
            clipped = ConvexWindowClipper.clipToHalfSpace(clipped, halfSpaces[i]!);
        }
        return clipped;
    }
}
