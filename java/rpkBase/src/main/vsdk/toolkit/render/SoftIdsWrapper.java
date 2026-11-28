package vsdk.toolkit.render;

import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Patch;

public class SoftIdsWrapper {
    private final Scene scene;
    private final FaceVisibilityBuffer target;

    public SoftIdsWrapper(Scene inScene, RendererConfiguration renderOptions) {
        scene = inScene;
        target = new FaceVisibilityBuffer(scene.camera.xSize, scene.camera.ySize, FaceIndexSample.background());
        SoftIds.renderVisibleFaces(scene, renderOptions, target);
    }

    public void getSize(long[] width, long[] height) {
        width[0] = target.getWidth();
        height[0] = target.getHeight();
    }

    public Patch getPatchAtPixel(int x, int y) {
        final FaceIndexSample face = target.getSample(x, target.getHeight() - 1 - y);
        return face.isBackground() ? null : scene.getFace(face.getIndex());
    }
}
