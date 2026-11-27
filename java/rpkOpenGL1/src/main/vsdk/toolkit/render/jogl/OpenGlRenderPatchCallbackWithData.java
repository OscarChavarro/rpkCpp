package vsdk.toolkit.render.jogl;

import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.environment.geometry.elements.Patch;

@FunctionalInterface
public interface OpenGlRenderPatchCallbackWithData<T> {
    void apply(Patch patch, Camera camera, RendererConfiguration renderOptions, T callbackData);
}
