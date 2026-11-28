//= References:                                                             =
//= [HECK1990] Heckbert, Paul S. "Generic Convex Polygon Scan Conversion   =
//=          and Clipping", Graphics Gems, ed. Andrew S. Glassner,         =
//=          Academic Press, 1990, section II.5, pp. 84-86.                 =
//= [SUTH1974b] Sutherland, Ivan E.; Hodgman, Gary W. "Reentrant Polygon  =
//=          Clipping", Communications of the ACM 17(1), 32-42, 1974.     =
//=          DOI: 10.1145/360767.360802.                                  =

/**
Generic Convex Polygon Scan Conversion and Clipping by Paul Heckbert
from "Graphics Gems", Academic Press, 1990

Homogeneous 3-D convex polygon clipper Paul Heckbert 1985, Dec 1989

Note that original algorithm is available at
https://github.com/erich666/GraphicsGems/tree/master/gems/PolyScan
*/

import { VSDKFatalException } from "../../../common/VSDKFatalException.js";
import { ConvexWindowClipper } from "../../../environment/geometry/geometricProcessing/convexPolygonClipper/ConvexWindowClipper.js";
import { HomogeneousHalfSpace } from "../../../environment/geometry/geometricProcessing/convexPolygonClipper/HomogeneousHalfSpace.js";
import { ScreenPolygon } from "../../../environment/geometry/geometricProcessing/convexPolygonClipper/ScreenPolygon.js";
import { PolygonBox } from "./PolygonBox.js";
import { PolygonClipOutcome } from "./PolygonClipOutcome.js";
import { PolygonClipResult } from "./PolygonClipResult.js";

/**
Clips a convex polygon to a screen space box with trivial acceptance and
rejection, after [HECK1990]. The plane by plane clipping is delegated to
ConvexWindowClipper. It keeps no state.
*/
export class PolygonBoxClipper {
    private constructor() {}

    /**
    Clip the convex polygon p1 to the screen space box
    using the homogeneous screen coordinates (sx, sy, sz, sw) of each vertex,
    testing if v->sx/v->sw > box.getMinX() and v->sx/v->sw < box.getMaxX(),
    and similar tests for y and z, for each vertex v of the polygon.
    If polygon is entirely inside box, then POLY_CLIP_IN is returned.
    If polygon is entirely outside box, then POLY_CLIP_OUT is returned with the
    empty polygon. Otherwise, if the polygon is cut by the box, POLY_CLIP_PARTIAL
    is returned with the clipped polygon.

    Given an n-gon as input, clipping against 6 planes could generate an
    (n+6)gon, so ScreenPolygon.MAXIMUM_SIDES_PER_POLYGON must be big enough to
    allow that.
    */
    public static clipToBox(polygon: ScreenPolygon, box: PolygonBox): PolygonClipOutcome {
        const n = polygon.getNumberOfVertices();
        let x0out = 0;
        let x1out = 0;
        let y0out = 0;
        let y1out = 0;
        let z0out = 0;
        let z1out = 0;

        if (n + 6 > ScreenPolygon.MAXIMUM_SIDES_PER_POLYGON) {
            // As in Java, the message goes to the error stream and a
            // VSDKFatalException is thrown for the caller to handle (the C++
            // port still exits).
            const message =
                "polyClipToBox: too many vertices: " + n + " (max=" + ScreenPolygon.MAXIMUM_SIDES_PER_POLYGON + "-6)";
            console.error(message);
            throw new VSDKFatalException(message);
        }

        // Homogeneous plane tests precede division by w ([SUTH1974b], p. 33).
        // This clip box uses [-w, w] on z as well as x and y; the paper uses
        // [0, w] for z. Counts also permit trivial acceptance and rejection.
        for (let i = 0; i < n; i++) {
            const vertex = polygon.getVertex(i);
            if (vertex.getSx() < box.getMinX() * vertex.getSw()) {
                // Out on left
                x0out++;
            }
            if (vertex.getSx() > box.getMaxX() * vertex.getSw()) {
                // Out on right
                x1out++;
            }
            if (vertex.getSy() < box.getMinY() * vertex.getSw()) {
                // Out on top
                y0out++;
            }
            if (vertex.getSy() > box.getMaxY() * vertex.getSw()) {
                // Out on bottom
                y1out++;
            }
            if (vertex.getSz() < box.getMinZ() * vertex.getSw()) {
                // Out on near
                z0out++;
            }
            if (vertex.getSz() > box.getMaxZ() * vertex.getSw()) {
                // Out on far
                z1out++;
            }
        }

        // Check if all vertices inside
        if (x0out + x1out + y0out + y1out + z0out + z1out === 0) {
            return new PolygonClipOutcome(PolygonClipResult.POLY_CLIP_IN, polygon);
        }

        // Check if all vertices are "outside" any of the six planes
        if (x0out === n || x1out === n || y0out === n || y1out === n || z0out === n || z1out === n) {
            return new PolygonClipOutcome(PolygonClipResult.POLY_CLIP_OUT, new ScreenPolygon());
        }

        // Only the planes that some vertex lies outside of cut the polygon:
        // pass those, in a fixed order, to the convex window clipper
        const halfSpaces: HomogeneousHalfSpace[] = [];
        let numberOfHalfSpaces = 0;
        if (x0out !== 0) {
            halfSpaces[numberOfHalfSpaces++] = new HomogeneousHalfSpace(-1.0, 0.0, 0.0, box.getMinX());
        }
        if (x1out !== 0) {
            halfSpaces[numberOfHalfSpaces++] = new HomogeneousHalfSpace(1.0, 0.0, 0.0, -box.getMaxX());
        }
        if (y0out !== 0) {
            halfSpaces[numberOfHalfSpaces++] = new HomogeneousHalfSpace(0.0, -1.0, 0.0, box.getMinY());
        }
        if (y1out !== 0) {
            halfSpaces[numberOfHalfSpaces++] = new HomogeneousHalfSpace(0.0, 1.0, 0.0, -box.getMaxY());
        }
        if (z0out !== 0) {
            halfSpaces[numberOfHalfSpaces++] = new HomogeneousHalfSpace(0.0, 0.0, -1.0, box.getMinZ());
        }
        if (z1out !== 0) {
            halfSpaces[numberOfHalfSpaces++] = new HomogeneousHalfSpace(0.0, 0.0, 1.0, -box.getMaxZ());
        }
        return new PolygonClipOutcome(
            PolygonClipResult.POLY_CLIP_PARTIAL,
            ConvexWindowClipper.clipToHalfSpaces(polygon, halfSpaces, numberOfHalfSpaces),
        );
    }
}
