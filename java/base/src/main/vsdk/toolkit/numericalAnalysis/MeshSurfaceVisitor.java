package vsdk.toolkit.numericalAnalysis;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.MaterialColorFlags;
import vsdk.toolkit.skin.MeshSurface;
import vsdk.toolkit.environment.geometry.elements.Patch;
/**
Initializes MeshSurface/Patch links with safe defaults without running numerical analysis.
*/

/**
Fills in the MeshSurface back pointer of the face belonging to the given surface
*/

/**
Fill in the MeshSurface back pointer of the FACEs in the MeshSurface
*/


public class MeshSurfaceVisitor {
    private static void surfaceConnectFace(MeshSurface mesh, Patch face) {
        int i;

        face.material = mesh.material;

        // Also fill in a nicer default color for the patch
        switch (mesh.colorFlags) {
            case MaterialColorFlags.FACE_COLORS:
                break;
            case MaterialColorFlags.VERTEX_COLORS:
                // Average color of the vertices
                face.color.set(0, 0, 0);
                for (i = 0; i < face.numberOfVertices; i++) {
                    face.color.r += face.vertex[i].color.r;
                    face.color.g += face.vertex[i].color.g;
                    face.color.b += face.vertex[i].color.b;
                }
                face.color.r /= (float)i;
                face.color.g /= (float)i;
                face.color.b /= (float)i;
                break;
            default: {
                ColorRgb rho;
                rho = PatchVisitor.averageNormalAlbedo(
                    face,
                    BsdfComponent.BRDF_DIFFUSE_COMPONENT | BsdfComponent.BRDF_GLOSSY_COMPONENT);
                face.color = new ColorRgb(rho);
            }
        }
    }

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
            face.material = mesh.material;
        }
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
