import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Camera } from "../../scene/Camera";
import { BoundingBox } from "../../skin/BoundingBox";
import { Geometry } from "../../skin/Geometry";

export class RenderOpenGL {
  private constructor() {
  }

  public static renderGetNearFar(camera: Camera | null, sceneGeometries: Geometry[] | null): void {
    if (camera === null) {
      return;
    }

    if (sceneGeometries === null || sceneGeometries.length === 0) {
      camera.far = 10.0;
      camera.near = 0.1;
      return;
    }

    const bounds = new BoundingBox();
    Geometry.listBounds(sceneGeometries, bounds);

    const minimum = new Vector3D(bounds.minX(), bounds.minY(), bounds.minZ());
    const maximum = new Vector3D(bounds.maxX(), bounds.maxY(), bounds.maxZ());
    const d = new Vector3D();

    camera.far = -Numeric.HUGE_FLOAT_VALUE;
    camera.near = Numeric.HUGE_FLOAT_VALUE;

    for (let i = 0; i <= 1; i++) {
      for (let j = 0; j <= 1; j++) {
        for (let k = 0; k <= 1; k++) {
          d.set(
            i !== 0 ? maximum.x : minimum.x,
            j !== 0 ? maximum.y : minimum.y,
            k !== 0 ? maximum.z : minimum.z
          );

          d.subtraction(d, camera.eyePosition);
          const z = d.dotProduct(camera.Z);

          if (z > camera.far) {
            camera.far = z;
          }
          if (z < camera.near) {
            camera.near = z;
          }
        }
      }
    }

    camera.far += 0.02 * camera.far;
    camera.near -= 0.02 * camera.near;
    if (camera.far < Numeric.EPSILON_FLOAT) {
      camera.far = camera.viewDistance;
    }
    if (camera.near < Numeric.EPSILON_FLOAT) {
      camera.near = camera.viewDistance / 100.0;
    }
  }
}
