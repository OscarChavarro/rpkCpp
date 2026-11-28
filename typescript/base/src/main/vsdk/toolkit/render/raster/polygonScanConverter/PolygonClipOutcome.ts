import { ScreenPolygon } from "../../../environment/geometry/geometricProcessing/convexPolygonClipper/ScreenPolygon.js";
import { PolygonClipResult } from "./PolygonClipResult.js";

/** Immutable result of a box clip: its classification and the clipped polygon */
export class PolygonClipOutcome {
    private readonly result: PolygonClipResult;
    private readonly polygon: ScreenPolygon;

    public constructor(inResult: PolygonClipResult, inPolygon: ScreenPolygon) {
        this.result = inResult;
        this.polygon = inPolygon;
    }

    public getResult(): PolygonClipResult {
        return this.result;
    }

    public getPolygon(): ScreenPolygon {
        return this.polygon;
    }
}
