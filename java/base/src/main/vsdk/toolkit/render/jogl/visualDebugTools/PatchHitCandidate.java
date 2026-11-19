package vsdk.toolkit.render.jogl.visualDebugTools;

public class PatchHitCandidate {
    public int patchIndex;
    public float distance;
    public boolean frontFacing;

    public PatchHitCandidate() {
        this(-1, 0.0f, false);
    }

    public PatchHitCandidate(int patchIndex, float distance, boolean frontFacing) {
        this.patchIndex = patchIndex;
        this.distance = distance;
        this.frontFacing = frontFacing;
    }
}
