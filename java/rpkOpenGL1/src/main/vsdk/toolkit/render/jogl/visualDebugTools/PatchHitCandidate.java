package vsdk.toolkit.render.jogl.visualDebugTools;

public class PatchHitCandidate {
    public int patchIndex;
    public double distance;
    public boolean frontFacing;

    public PatchHitCandidate() {
        this(-1, 0.0, false);
    }

    public PatchHitCandidate(int patchIndex, double distance, boolean frontFacing) {
        this.patchIndex = patchIndex;
        this.distance = distance;
        this.frontFacing = frontFacing;
    }
}
