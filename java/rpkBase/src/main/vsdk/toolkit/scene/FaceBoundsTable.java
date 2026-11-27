package vsdk.toolkit.scene;

import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;

/**
Conservative bounds of every face of a scene, indexed by the dense face index
(Patch.getFaceIndex()). Built once by the scene after the face indices are
assigned, from the patch bounding boxes, and read-only afterwards, so any
number of threads can share it.
*/
public final class FaceBoundsTable {
    private final RpkAxisAlignedBoundingBox[] bounds;
    private final int numberOfFaces;

    public FaceBoundsTable(Patch[] faces, int inNumberOfFaces) {
        numberOfFaces = inNumberOfFaces;
        bounds = new RpkAxisAlignedBoundingBox[numberOfFaces > 0 ? numberOfFaces : 1];
        for (int i = 0; i < numberOfFaces; i++) {
            bounds[i] = new RpkAxisAlignedBoundingBox();
            faces[i].computeBounds(bounds[i]);
        }
    }

    public int getNumberOfFaces() {
        return numberOfFaces;
    }

    public RpkAxisAlignedBoundingBox get(int faceIndex) {
        return bounds[faceIndex];
    }
}
