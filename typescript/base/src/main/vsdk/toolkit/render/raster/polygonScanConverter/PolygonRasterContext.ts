//= References:                                                             =
//= [HECK1990] Heckbert, Paul S. "Generic Convex Polygon Scan Conversion   =
//=          and Clipping", Graphics Gems, ed. Andrew S. Glassner,         =
//=          Academic Press, 1990, section II.5, pp. 84-86.                 =
//= [BEKA1997] Bekaert, Philippe. "SGL: Simple (Small) Graphics Library", =
//=          SGL_README, June 20, 1997; RenderPark source, as preserved in =
//=          this repository at commit c98a00f1, doc/txt/SGL_README.       =
//= [SUTH1974b] Sutherland, Ivan E.; Hodgman, Gary W. "Reentrant Polygon    =
//=          Clipping", Communications of the ACM 17(1), 32-42, 1974.     =
//=          DOI: 10.1145/360767.360802.                                   =

/*
The polygon rasterizer integrates the clipping and scan conversion routines of
[HECK1990].II.5, with flat and depth-buffered scan conversion optimized by
[BEKA1997] (the SGL library of RenderPark). Heckbert credits the scan converter
to an algorithm by Frank Crow at NYIT and his clipper to an implementation of
[SUTH1974b] by Ed Catmull and Alvy Ray Smith. These two implementations are
identified in [HECK1990].85; no separate publication is specified there. The
half-space clipping is in convexPolygonClipper/ConvexWindowClipper, the box
clipping in PolygonBoxClipper and the scan conversion in PolygonScanConverter,
while this context prepares their projected input.
*/

import { VSDK } from "../../../common/VSDK.js";
import { Logger } from "../../../common/logging/Logger.js";
import { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { Vector4Dd } from "../../../common/linealAlgebra/Vector4Dd.js";
import { PolygonVertex } from "../../../environment/geometry/geometricProcessing/convexPolygonClipper/PolygonVertex.js";
import { ScreenPolygon } from "../../../environment/geometry/geometricProcessing/convexPolygonClipper/ScreenPolygon.js";
import { PolygonBox } from "./PolygonBox.js";
import { PolygonBoxClipper } from "./PolygonBoxClipper.js";
import { PolygonClipResult } from "./PolygonClipResult.js";
import { PolygonScanConverter } from "./PolygonScanConverter.js";
import { RasterRectangle2D } from "./RasterRectangle2D.js";

/** Projection, viewport and optional depth plane shared by typed raster targets. */
export class PolygonRasterContext {
    private readonly width: number;
    private readonly height: number;
    private viewport: RasterRectangle2D;
    private transform: Matrix4x4d;
    private readonly depthBuffer: Float64Array | null;

    public constructor(inWidth: number, inHeight: number, depthTesting: boolean) {
        this.width = inWidth;
        this.height = inHeight;
        this.viewport = new RasterRectangle2D(0, 0, inWidth - 1, inHeight - 1);
        this.transform = new Matrix4x4d();
        this.depthBuffer = depthTesting ? new Float64Array(inWidth * inHeight) : null;
    }

    public getWidth(): number {
        return this.width;
    }

    public getHeight(): number {
        return this.height;
    }

    public getViewportWidth(): number {
        return this.viewport.getMaxX() - this.viewport.getMinX() + 1;
    }

    public getViewportHeight(): number {
        return this.viewport.getMaxY() - this.viewport.getMinY() + 1;
    }

    public getViewport(): RasterRectangle2D {
        return this.viewport;
    }

    public getDepthBuffer(): Float64Array | null {
        return this.depthBuffer;
    }

    public setViewport(x: number, y: number, viewportWidth: number, viewportHeight: number): void {
        if (
            x < 0 ||
            y < 0 ||
            viewportWidth <= 0 ||
            viewportHeight <= 0 ||
            viewportWidth > this.width - x ||
            viewportHeight > this.height - y
        ) {
            Logger.reportMessage(this, Logger.ERROR, "setViewport", "Viewport lies outside the raster target");
            return;
        }

        this.viewport = new RasterRectangle2D(x, y, x + viewportWidth - 1, y + viewportHeight - 1);
    }

    public setTransform(matrix: Matrix4x4d): void {
        this.transform = matrix;
    }

    /**
    Transforms, clips and projects the polygon to the viewport and depth range.
    The result is empty when nothing of the polygon is to be drawn.
    */
    public projectPolygon(numberOfVertices: number, vertices: readonly Vector3Dd[]): ScreenPolygon {
        const clipBox = new PolygonBox(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

        if (numberOfVertices < 3 || numberOfVertices > ScreenPolygon.MAXIMUM_SIDES_PER_POLYGON - 6) {
            Logger.reportMessage(
                this,
                Logger.ERROR,
                "projectPolygon",
                "Invalid vertex count for polygon rasterization",
            );
            return new ScreenPolygon();
        }

        // Transform the vertices and fill in a ScreenPolygon
        const transformed: PolygonVertex[] = [];
        for (let i = 0; i < numberOfVertices; i++) {
            const vertex = vertices[i]!;
            const v = this.transform.multiply(new Vector4Dd(vertex.x(), vertex.y(), vertex.z(), 1.0));
            transformed[i] = new PolygonVertex(v.x(), v.y(), v.z(), v.w(), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        }
        const mask =
            ScreenPolygon.attributeBit(PolygonVertex.SX) |
            ScreenPolygon.attributeBit(PolygonVertex.SY) |
            ScreenPolygon.attributeBit(PolygonVertex.SZ) |
            ScreenPolygon.attributeBit(PolygonVertex.SW);
        // Clip homogeneous coordinates before perspective division, as in
        // [SUTH1974b], p. 33; the plane tests are in PolygonBoxClipper and the
        // edge cases in ConvexWindowClipper.
        const outcome = PolygonBoxClipper.clipToBox(new ScreenPolygon(numberOfVertices, mask, transformed), clipBox);
        if (outcome.getResult() === PolygonClipResult.POLY_CLIP_OUT) {
            return new ScreenPolygon();
        }

        // Perspective divide and transformation to viewport and depth range
        const clipped = outcome.getPolygon();
        const projected: PolygonVertex[] = [];
        for (let i = 0; i < clipped.getNumberOfVertices(); i++) {
            const vertex = clipped.getVertex(i);
            if (vertex.getSw() < VSDK.EPSILON) {
                // Inside the clip box -w <= sx, sy, sz <= w, so w >= 0; a w near zero
                // leaves the vertex at the eye, where the perspective divide is
                // undefined. The test is done after clipping, so that a polygon that
                // only crosses the plane w = 0 keeps its visible part.
                return new ScreenPolygon();
            }
            projected[i] = new PolygonVertex(
                this.viewport.getMinX() + (vertex.getSx() / vertex.getSw() + 1.0) * this.getViewportWidth() * 0.5,
                this.viewport.getMinY() + (vertex.getSy() / vertex.getSw() + 1.0) * this.getViewportHeight() * 0.5,
                (vertex.getSz() / vertex.getSw() + 1.0) * 0.5 * PolygonScanConverter.MAXIMUM_DEPTH,
                vertex.getSw(),
                vertex.getX(),
                vertex.getY(),
                vertex.getZ(),
                vertex.getU(),
                vertex.getV(),
                vertex.getR(),
                vertex.getG(),
                vertex.getB(),
            );
        }

        return new ScreenPolygon(clipped.getNumberOfVertices(), clipped.getMask(), projected);
    }
}
