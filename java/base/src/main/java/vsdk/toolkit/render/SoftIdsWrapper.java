package vsdk.toolkit.render;

import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.render.sgl.SglContext;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Patch;

public class SoftIdsWrapper {
    private SglContext sgl; // Software rendering context, includes frame buffer

    // Also performs the actual ID rendering
    private void init(Scene scene, RenderOptions renderOptions) {
        sgl = SoftIds.setupSoftFrameBuffer(scene.camera);
        SoftIds.softRenderPatches(scene, renderOptions, sgl);
    }

    public SoftIdsWrapper(Scene scene, RenderOptions renderOptions) {
        sgl = null;
        init(scene, renderOptions);
    }

    public void getSize(long[] width, long[] height) {
        if (width != null && width.length > 0) {
            width[0] = sgl.width;
        }
        if (height != null && height.length > 0) {
            height[0] = sgl.height;
        }
    }

    public Patch getPatchAtPixel(int x, int y) {
        int index = (sgl.height - 1 - y) * sgl.width + x;
        return sgl.patchBuffer[index];
    }
}
