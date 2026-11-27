package vsdk.toolkit.render.jogl.visualDebugTools;

public class GlutDebugState {
    public int primarySelectedPatch;
    public int selectedSelectedPatch;
    public boolean showSelectedPathOnly;
    public double angleAroundViewportU;
    public double angleAroundViewportV;

    public GlutDebugState() {
        primarySelectedPatch = -1;
        selectedSelectedPatch = -1;
        showSelectedPathOnly = true;
        angleAroundViewportU = 0.0;
        angleAroundViewportV = 0.0;
    }
}
