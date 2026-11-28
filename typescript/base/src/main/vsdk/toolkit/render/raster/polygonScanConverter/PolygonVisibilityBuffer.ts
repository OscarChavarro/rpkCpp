import { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { PolygonRasterContext } from "./PolygonRasterContext.js";
import { PolygonScanConverter } from "./PolygonScanConverter.js";
import { RasterRectangle2D } from "./RasterRectangle2D.js";

/**
A single payload per sample. Depth, when enabled, belongs to this pass.

Implementation of PolygonVisibilityBuffer, generic on its Sample payload. It does
not know which payloads exist: each client module that defines one
instantiates it with its own payload type (see FaceVisibilityBuffer and
GalerkinElementVisibilityBuffer). That keeps the dependency pointing from the
client to this package.
*/
export class PolygonVisibilityBuffer<Sample> {
    private readonly context: PolygonRasterContext;
    private readonly background: Sample;
    private readonly samples: Sample[];

    public constructor(width: number, height: number, backgroundValue: Sample, depthTesting = true) {
        this.context = new PolygonRasterContext(width, height, depthTesting);
        this.background = backgroundValue;
        this.samples = new Array<Sample>(width * height);
        this.clear();
    }

    public getWidth(): number {
        return this.context.getWidth();
    }

    public getHeight(): number {
        return this.context.getHeight();
    }

    public getViewport(): RasterRectangle2D {
        return this.context.getViewport();
    }

    public getViewportWidth(): number {
        return this.context.getViewportWidth();
    }

    public getViewportHeight(): number {
        return this.context.getViewportHeight();
    }

    public getSample(x: number, y: number): Sample {
        return this.samples[y * this.getWidth() + x] as Sample;
    }

    public setViewport(x: number, y: number, width: number, height: number): void {
        this.context.setViewport(x, y, width, height);
    }

    public setTransform(matrix: Matrix4x4d): void {
        this.context.setTransform(matrix);
    }

    public clear(): void {
        const viewport = this.getViewport();
        const depth = this.context.getDepthBuffer();
        for (let y = viewport.getMinY(); y <= viewport.getMaxY(); y++) {
            const firstSample = y * this.getWidth() + viewport.getMinX();
            const endSample = firstSample + this.getViewportWidth();
            for (let i = firstSample; i < endSample; i++) {
                this.samples[i] = this.background;
            }
            if (depth !== null) {
                for (let i = firstSample; i < endSample; i++) {
                    depth[i] = PolygonScanConverter.MAXIMUM_DEPTH;
                }
            }
        }
    }

    public drawPolygon(numberOfVertices: number, vertices: readonly Vector3Dd[], value: Sample): void {
        const polygon = this.context.projectPolygon(numberOfVertices, vertices);
        if (polygon.getNumberOfVertices() === 0) {
            return;
        }

        const depth = this.context.getDepthBuffer();
        if (depth !== null) {
            PolygonScanConverter.scanZ(this.samples, depth, this.getWidth(), value, polygon, this.getViewport());
        } else {
            PolygonScanConverter.scanFlat(this.samples, this.getWidth(), value, polygon, this.getViewport());
        }
    }
}
