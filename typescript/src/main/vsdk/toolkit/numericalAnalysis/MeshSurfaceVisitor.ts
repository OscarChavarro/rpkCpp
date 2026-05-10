import { ColorRgb } from "../common/color/ColorRgb";
import { BsdfComponent } from "../material/BsdfComponent";
import { MaterialColorFlags } from "../skin/MaterialColorFlags";
import { MeshSurface } from "../skin/MeshSurface";
import { Patch } from "../skin/Patch";
import { PatchVisitor } from "./PatchVisitor";

export class MeshSurfaceVisitor {
  private static surfaceConnectFace(mesh: MeshSurface, face: Patch): void {
    face.material = mesh.material;

    switch (MeshSurface.colorFlags) {
      case MaterialColorFlags.FACE_COLORS:
        break;
      case MaterialColorFlags.VERTEX_COLORS: {
        face.color.set(0, 0, 0);
        let i = 0;
        for (; i < face.numberOfVertices; i++) {
          const vertex = face.vertex[i];
          if (vertex === null) {
            continue;
          }
          face.color.r += vertex.color.r;
          face.color.g += vertex.color.g;
          face.color.b += vertex.color.b;
        }
        if (i > 0) {
          face.color.r /= i;
          face.color.g /= i;
          face.color.b /= i;
        }
        break;
      }
      default: {
        const rho: ColorRgb = PatchVisitor.averageNormalAlbedo(
          face,
          BsdfComponent.BRDF_DIFFUSE_COMPONENT | BsdfComponent.BRDF_GLOSSY_COMPONENT
        );
        rho.set(face.color.r, face.color.g, face.color.b);
      }
    }
  }

  public static initializeFacesDefaults(mesh: MeshSurface | null): void {
    if (mesh === null) {
      return;
    }
    for (let i = 0; mesh.faces !== null && i < mesh.faces.length; i++) {
      const face = mesh.faces[i];
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
        MeshSurfaceVisitor.surfaceConnectFace(mesh, face);
      }
    }
  }
}
