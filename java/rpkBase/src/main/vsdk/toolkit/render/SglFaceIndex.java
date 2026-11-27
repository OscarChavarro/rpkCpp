package vsdk.toolkit.render;

/** A visible face in the scene's current face table. The default is background. */
public final class SglFaceIndex {
    private static final SglFaceIndex BACKGROUND = new SglFaceIndex(-1);
    private final int index;

    public SglFaceIndex(int faceIndex) {
        index = faceIndex;
    }

    public static SglFaceIndex background() {
        return BACKGROUND;
    }

    public boolean isBackground() {
        return index < 0;
    }

    public int getIndex() {
        return index;
    }
}
