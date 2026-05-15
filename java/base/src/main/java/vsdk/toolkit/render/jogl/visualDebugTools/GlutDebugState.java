package vsdk.toolkit.render.jogl.visualDebugTools;

public class GlutDebugState {
    public int primarySelectedPatch;
    public int selectedSelectedPatch;
    public boolean showSelectedPathOnly;
    public float angleAroundViewportU;
    public float angleAroundViewportV;

    public GlutDebugState() {
        primarySelectedPatch = -1;
        selectedSelectedPatch = -1;
        showSelectedPathOnly = true;
        angleAroundViewportU = 0.0f;
        angleAroundViewportV = 0.0f;
    }
}
