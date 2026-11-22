import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { RefractionIndex } from "./RefractionIndex";

export class Xxdf {
  public static readonly PHONG_LOWEST_SPECULAR_EXP = 250.0;

  public static idealReflectedDirection(inDirection: Vector3Df, normal: Vector3Df): Vector3Df {
    const tmp = 2.0 * normal.dotProduct(inDirection);
    let result = new Vector3Df();

    result = normal.multiply(tmp);
    result = inDirection.subtract(result);
    result = result.normalized(Numeric.EPSILON_FLOAT);

    return result;
  }

  public static idealRefractedDirection(
    inDirection: Vector3Df,
    normal: Vector3Df,
    inIndex: RefractionIndex,
    outIndex: RefractionIndex,
    totalInternalReflection: boolean[] | null
  ): Vector3Df {
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

    let result = new Vector3Df();
    result = inDirection.multiply(refractionIndex);
    result = result.add(normal.multiply(normalScale));
    result = result.normalized(Numeric.EPSILON_FLOAT);

    return result;
  }
}
