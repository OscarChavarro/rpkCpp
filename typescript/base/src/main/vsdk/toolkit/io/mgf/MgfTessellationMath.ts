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
  Compute u and v given w (normalized)
  */
  public static mgfMakeAxes(u: Vector3Dd, v: Vector3Dd, w: Vector3Dd, epsilon: number): void {
    v.x = 0.0;
    v.y = 0.0;
    v.z = 0.0;
    const vArr = [v.x, v.y, v.z];
    const wArr = [w.x, w.y, w.z];

    let i = 0;
    for (; i < 3; i++) {
      if (wArr[i]! > -0.6 && wArr[i]! < 0.6) {
        break;
      }
    }

    if (i < 3) {
      vArr[i] = 1.0;
    }

    v.x = vArr[0]!;
    v.y = vArr[1]!;
    v.z = vArr[2]!;

    u.crossProduct(v, w);
    u.normalizeAndGivePreviousNorm(epsilon);
    v.crossProduct(w, u);
  }
}
