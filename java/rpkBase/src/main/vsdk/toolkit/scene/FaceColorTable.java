package vsdk.toolkit.scene;

import vsdk.toolkit.common.color.ColorRgb;

/**
Display color of every face of a scene, indexed by the dense face index
(Patch::getFaceIndex()). Every face starts black; the radiance methods
rewrite a face's color from its radiance after each step, and the vertex
colors (Gouraud display, VRML export) and the flat OpenGL display read it.
Owned by the scene.
*/
public class FaceColorTable {
    private final ColorRgb[] colors;
    private final int numberOfFaces;

    public FaceColorTable(int inNumberOfFaces) {
        numberOfFaces = inNumberOfFaces;
        colors = new ColorRgb[numberOfFaces > 0 ? numberOfFaces : 1];
        for ( int i = 0; i < numberOfFaces; i++ ) {
            colors[i] = new ColorRgb(0.0, 0.0, 0.0);
        }
    }

    public int getNumberOfFaces() {
        return numberOfFaces;
    }

    public ColorRgb get(int faceIndex) {
        return colors[faceIndex];
    }

    public void set(int faceIndex, ColorRgb color) {
        colors[faceIndex] = color;
    }
}
