package vsdk.toolkit.render.jogl;

import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.environment.geometry.elements.Patch;

@FunctionalInterface
public interface OpenGlRenderPatchCallbackWithData {
    void apply(Patch patch, Camera camera, RenderOptions renderOptions, Object callbackData);
}
