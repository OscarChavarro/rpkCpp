package vsdk.toolkit.render.raster.polygonScanConverter;

import vsdk.toolkit.environment.geometry.geometricProcessing.convexPolygonClipper.ScreenPolygon;

/** Immutable result of a box clip: its classification and the clipped polygon */
public final class PolygonClipOutcome {
    private final PolygonClipResult result;
    private final ScreenPolygon polygon;

    public PolygonClipOutcome(PolygonClipResult inResult, ScreenPolygon inPolygon) {
        result = inResult;
        polygon = inPolygon;
    }

    public PolygonClipResult getResult() { return result; }
    public ScreenPolygon getPolygon() { return polygon; }
}
