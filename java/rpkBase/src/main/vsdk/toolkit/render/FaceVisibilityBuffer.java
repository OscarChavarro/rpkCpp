package vsdk.toolkit.render;

import vsdk.toolkit.render.raster.polygonScanConverter.PolygonVisibilityBuffer;

/**
Visible faces, as FaceIndexSample samples.
Java adaptation: the C++ port's alias of PolygonVisibilityBuffer<FaceIndexSample>.
*/
public final class FaceVisibilityBuffer extends PolygonVisibilityBuffer<FaceIndexSample> {
    public FaceVisibilityBuffer(int width, int height, FaceIndexSample backgroundValue) {
        super(width, height, backgroundValue);
    }
}
