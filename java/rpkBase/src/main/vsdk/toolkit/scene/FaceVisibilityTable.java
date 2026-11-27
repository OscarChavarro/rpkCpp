package vsdk.toolkit.scene;

/**
Whether each face of a scene is directly visible from the camera, indexed by
the dense face index (Patch::getFaceIndex()). Every face starts invisible;
Potential::updateDirectVisibility rewrites the whole table from an ID render
of the current view, and the importance-driven stochastic radiosity reads it
as source importance. Owned by the scene.
*/
public class FaceVisibilityTable {
    private final boolean[] visible;
    private final int numberOfFaces;

    public FaceVisibilityTable(int inNumberOfFaces) {
        numberOfFaces = inNumberOfFaces;
        visible = new boolean[numberOfFaces > 0 ? numberOfFaces : 1];
    }

    public int getNumberOfFaces() {
        return numberOfFaces;
    }

    public boolean isVisible(int faceIndex) {
        return visible[faceIndex];
    }

    public void setVisible(int faceIndex, boolean isFaceVisible) {
        visible[faceIndex] = isFaceVisible;
    }
}
