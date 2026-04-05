package vsdk.toolkit.render.jogl;

import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.skin.Patch;

@FunctionalInterface
public interface OpenGlRenderPatchCallbackWithData {
    void apply(Patch patch, Camera camera, RenderOptions renderOptions, Object callbackData);
}
