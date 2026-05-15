package vsdk.toolkit.render.jogl.visualDebugTools;

import java.util.function.Consumer;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.tonemap.ToneMappingContext;

public class GlutDebugToolsModel {
    public GlutDebugMode mode;
    public boolean fullScreen;
    public boolean fullScreenApplied;
    public int selectedHierarchyLevel;
    public int width;
    public int height;
    public int windowedWidth;
    public int windowedHeight;
    public Scene scene;
    public RadianceMethod radianceMethod;
    public RenderOptions renderOptions;
    public ToneMappingContext toneMapOptions;
    public GlutDebugState debugState;
    public Consumer<ParseRuntimeContext> memoryFreeCallBack;
    public ParseRuntimeContext mgfContext;

    public GlutDebugToolsModel() {
        mode = GlutDebugMode.RADIANCE_SCENE;
        fullScreen = false;
        fullScreenApplied = false;
        selectedHierarchyLevel = 0;
        width = 1920;
        height = 1200;
        windowedWidth = 1920;
        windowedHeight = 1200;
        scene = null;
        radianceMethod = null;
        renderOptions = null;
        toneMapOptions = null;
        debugState = null;
        memoryFreeCallBack = null;
        mgfContext = null;
    }
}
