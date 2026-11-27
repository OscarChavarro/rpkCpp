package vsdk.toolkit.render.jogl;

import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.scene.Camera;

public class OpenGlRenderTraversalCallback<T> {
    public final OpenGlRenderPatchCallback callbackWithoutData;
    public final OpenGlRenderPatchCallbackWithData<T> callbackWithData;
    public final T callbackData;

    public OpenGlRenderTraversalCallback(OpenGlRenderPatchCallback callbackWithoutData,
                                         OpenGlRenderPatchCallbackWithData<T> callbackWithData,
                                         T callbackData) {
        this.callbackWithoutData = callbackWithoutData;
        this.callbackWithData = callbackWithData;
        this.callbackData = callbackData;
    }

    public void apply(Patch patch, Camera camera, RendererConfiguration renderOptions) {
        if (callbackWithData != null) callbackWithData.apply(patch, camera, renderOptions, callbackData);
        else if (callbackWithoutData != null) callbackWithoutData.apply(patch, camera, renderOptions);
    }
}
