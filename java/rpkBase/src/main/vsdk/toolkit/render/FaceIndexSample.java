package vsdk.toolkit.render;

/** A visible face in the scene's current face table. The default is background. */
public final class FaceIndexSample {
    private static final FaceIndexSample BACKGROUND = new FaceIndexSample(-1);
    private final int index;

    public FaceIndexSample(int faceIndex) {
        index = faceIndex;
    }

    public static FaceIndexSample background() {
        return BACKGROUND;
    }

    public boolean isBackground() {
        return index < 0;
    }

    public int getIndex() {
        return index;
    }
}
