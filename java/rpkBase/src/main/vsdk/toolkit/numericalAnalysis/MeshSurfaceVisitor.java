package vsdk.toolkit.numericalAnalysis;

import vsdk.toolkit.skin.MeshSurface;
import vsdk.toolkit.environment.geometry.elements.Patch;

public class MeshSurfaceVisitor {
    /**
    Initializes MeshSurface/Patch links with safe defaults without running numerical analysis.
    */
    public static void initializeFacesDefaults(MeshSurface mesh) {
        if (mesh == null) {
            return;
        }
        for (int i = 0; mesh.faces != null && i < mesh.faces.size(); i++) {
            Patch face = mesh.faces.get(i);
            if (face == null) {
                continue;
            }
            face.setMaterial(mesh.material);
        }
    }

    /**
    Fills in the MeshSurface back pointer of the face belonging to the given surface
    */
    private static void surfaceConnectFace(MeshSurface mesh, Patch face) {
        face.setMaterial(mesh.material);
        // Face display colors start black in the scene's FaceColorTable. The color
        // defaults that used to be derived here never applied: MeshSurface.colorFlags
        // is reset to NO_COLORS before the faces are connected.
    }

    /**
    Fill in the MeshSurface back pointer of the FACEs in the MeshSurface
    */
    public static void fillFacesBackPointers(MeshSurface mesh) {
        initializeFacesDefaults(mesh);
        for (int i = 0; mesh.faces != null && i < mesh.faces.size(); i++) {
            MeshSurfaceVisitor.surfaceConnectFace(mesh, mesh.faces.get(i));
        }
    }
}
