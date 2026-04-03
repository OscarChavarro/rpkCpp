package vsdk.toolkit.render;

import java.util.ArrayList;

/**
Render hooks are called each time the scene is rendered.
Functions are provided to add and remove hooks.
Hooks should only depend on render.h, not on GLX or OpenGL
*/
public class RenderHookList {
    private static ArrayList<RenderHook> renderHookList = new ArrayList<>();

    public static void renderHooks() {
        for (int i = 0; renderHookList != null && i < renderHookList.size(); i++) {
            RenderHook h = renderHookList.get(i);
            h.function.apply(h.data);
        }
    }

    public static void removeAllRenderHooks() {
        renderHookList = null;
    }
}
