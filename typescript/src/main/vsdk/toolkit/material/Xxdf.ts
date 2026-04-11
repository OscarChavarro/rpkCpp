import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { RefractionIndex } from "./RefractionIndex";

export class Xxdf {
  public static readonly PHONG_LOWEST_SPECULAR_EXP = 250.0;

  public static idealReflectedDirection(inDirection: Vector3D, normal: Vector3D): Vector3D {
    const tmp = 2.0 * normal.dotProduct(inDirection);
    const result = new Vector3D();

    result.scaledCopy(tmp, normal);
    result.subtraction(inDirection, result);
    result.normalize(Numeric.EPSILON_FLOAT);

    return result;
  }

  public static idealRefractedDirection(
    inDirection: Vector3D,
    normal: Vector3D,
    inIndex: RefractionIndex,
    outIndex: RefractionIndex,
    totalInternalReflection: boolean[] | null
  ): Vector3D {
    const refractionIndex = inIndex.getNr() / outIndex.getNr();
    const ci = -inDirection.dotProduct(normal);
    const ct2 = 1.0 + refractionIndex * refractionIndex * (ci * ci - 1.0);

    if (ct2 < 0.0) {
      if (totalInternalReflection !== null && totalInternalReflection.length > 0) {
        totalInternalReflection[0] = true;
      }
      return Xxdf.idealReflectedDirection(inDirection, normal);
    }

    if (totalInternalReflection !== null && totalInternalReflection.length > 0) {
      totalInternalReflection[0] = false;
    }

    const ct = globalThis.Math.sqrt(ct2);
    const normalScale = refractionIndex * ci - ct;

    const result = new Vector3D();
    result.scaledCopy(refractionIndex, inDirection);
    result.sumScaled(result, normalScale, normal);
    result.normalize(Numeric.EPSILON_FLOAT);

    return result;
  }
}
