import { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd";

export class MgfTessellationMath {
  public static readonly MGF_PV_SIZE = 24;

  private constructor() {
  }

  public static formatFloat(target: string[], targetLength: number, value: number): void {
    if (target.length <= 0) {
      target.push("");
    }

    let text = `${value}`;
    if (globalThis.Number.isFinite(value)) {
      text = value.toPrecision(12);
      text = text.replace(/(\.\d*?[1-9])0+(e[+-]?\d+)?$/i, "$1$2");
      text = text.replace(/\.0+(e[+-]?\d+)?$/i, "$1");
      text = text.replace(/e\+?/i, "e");
    }

    const maxLength = targetLength - 1;
    if (maxLength >= 0 && text.length > maxLength) {
      text = text.substring(0, maxLength);
    }
    target[0] = text;
  }

  /**
  Compute u and v given w (normalized). u and v are out-parameters: their
  first element receives the computed axis.
  */
  public static mgfMakeAxes(u: Vector3Dd[], v: Vector3Dd[], w: Vector3Dd, epsilon: number): void {
    const vArr = [0.0, 0.0, 0.0];
    const wArr = [w.x(), w.y(), w.z()];

    let i = 0;
    for (; i < 3; i++) {
      if (wArr[i]! > -0.6 && wArr[i]! < 0.6) {
        break;
      }
    }

    if (i < 3) {
      vArr[i] = 1.0;
    }

    v[0] = new Vector3Dd(vArr[0]!, vArr[1]!, vArr[2]!);

    u[0] = v[0].crossProduct(w);
    MgfTessellationMath.normalizeAndGivePreviousNorm(u, epsilon);
    v[0] = w.crossProduct(u[0]);
  }

  /**
  Normalize a vector, return old magnitude. The vector is an in/out-parameter:
  its first element is replaced by the normalized vector.
  */
  public static normalizeAndGivePreviousNorm(vector: Vector3Dd[], epsilon: number): number {
    const v = vector[0]!;
    let len = v.dotProduct(v); // Note: this starts being length ^ 2
    if (len <= 0.0) {
      return 0.0;
    }

    if (len <= 1.0 + epsilon && len >= 1.0 - epsilon) {
      // First order approximation
      len = 0.5 + 0.5 * len;
    }
    else {
      len = globalThis.Math.sqrt(len);
    }

    vector[0] = new Vector3Dd(v.x() / len, v.y() / len, v.z() / len);

    return len;
  }

  public static isNull(vector: Vector3Dd, epsilon: number): boolean {
    return vector.dotProduct(vector) <= epsilon * epsilon;
  }

  /**
  Returns squared distance between the two vectors
  */
  public static distanceSquared(a: Vector3Dd, b: Vector3Dd): number {
    const dx = b.x() - a.x();
    const dy = b.y() - a.y();
    const dz = b.z() - a.z();
    return dx * dx + dy * dy + dz * dz;
  }
}
