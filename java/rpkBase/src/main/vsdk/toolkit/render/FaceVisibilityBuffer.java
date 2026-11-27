package vsdk.toolkit.render;

import vsdk.toolkit.render.sgl.SglVisibilityBuffer;

/**
Visible faces, as SglFaceIndex samples.
Java adaptation: the C++ port's alias of SglVisibilityBuffer<SglFaceIndex>.
*/
public final class FaceVisibilityBuffer extends SglVisibilityBuffer<SglFaceIndex> {
    public FaceVisibilityBuffer(int width, int height, SglFaceIndex backgroundValue) {
        super(width, height, backgroundValue);
    }
}
