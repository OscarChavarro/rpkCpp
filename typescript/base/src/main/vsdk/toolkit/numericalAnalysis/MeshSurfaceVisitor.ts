import { MeshSurface } from "../skin/MeshSurface";
import { Patch } from "../environment/geometry/elements/Patch";

export class MeshSurfaceVisitor {
  /**
  Fills in the MeshSurface back pointer of the face belonging to the given surface
  */
  private static surfaceConnectFace(mesh: MeshSurface, face: Patch): void {
    face.material = mesh.material;
    // Face display colors start black in the scene's FaceColorTable. The color
    // defaults that used to be derived here never applied: MeshSurface.colorFlags
    // is reset to NO_COLORS before the faces are connected.
  }

  public static initializeFacesDefaults(mesh: MeshSurface | null): void {
    if (mesh === null) {
      return;
    }
    for (let i = 0; mesh.faces !== null && i < mesh.faces.length; i++) {
      const face = mesh.faces[i]!;
      if (face === null) {
        continue;
      }
      face.material = mesh.material;
    }
  }

  public static fillFacesBackPointers(mesh: MeshSurface): void {
    MeshSurfaceVisitor.initializeFacesDefaults(mesh);
    for (let i = 0; mesh.faces !== null && i < mesh.faces.length; i++) {
      const face = mesh.faces[i];
      if (face !== null) {
        MeshSurfaceVisitor.surfaceConnectFace(mesh, face!);
      }
    }
  }
}
