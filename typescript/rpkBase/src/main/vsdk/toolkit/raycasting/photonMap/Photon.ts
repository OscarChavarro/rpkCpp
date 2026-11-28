import { String as JavaString } from "vitral/dist/java/lang/String";
import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { CoordinateSystem } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/CoordinateSystem";
import { Numeric } from "../../skin/Numeric";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { BsdfComponent } from "../../material/BsdfComponent";

// Non-compact photon representation
export class Photon {
  protected m_pos: Vector3Dd;
  protected m_power: ColorRgbMutable;
  protected m_dir: Vector3Dd;

  public constructor();
  public constructor(pos: Vector3Dd, power: ColorRgbMutable, dir: Vector3Dd);
  public constructor(pos?: Vector3Dd, power?: ColorRgbMutable, dir?: Vector3Dd) {
    this.m_pos = pos === undefined ? new Vector3Dd() : new Vector3Dd(pos.x(), pos.y(), pos.z());
    this.m_power = power === undefined ? new ColorRgbMutable() : new ColorRgbMutable(power.r, power.g, power.b);
    this.m_dir = dir === undefined ? new Vector3Dd() : new Vector3Dd(dir.x(), dir.y(), dir.z());
  }

  public pos(): Vector3Dd {
    return this.m_pos;
  }

  public power(): ColorRgbMutable {
    return this.m_power;
  }

  public addPower(col: ColorRgbMutable): void {
    this.m_power.add(this.m_power, col);
  }

  public dir(): Vector3Dd {
    return this.m_dir;
  }

  public findRS(r: number[], s: number[], coord: CoordinateSystem, flag: number, n: number): void {
    const phi = [0.0];
    const theta = [0.0];
    coord.rectangularToSphericalCoord(this.m_dir, phi, theta, Numeric.EPSILON);

    if (flag === BsdfComponent.BRDF_DIFFUSE_COMPONENT) {
      s[0] = phi[0]! / (2.0 * globalThis.Math.PI);
      const tmp = globalThis.Math.cos(theta[0]!);
      r[0] = -tmp * tmp + 1.0;
    }
    else if (flag === BsdfComponent.BRDF_GLOSSY_COMPONENT) {
      s[0] = phi[0]! / (2.0 * globalThis.Math.PI);
      r[0] = globalThis.Math.pow(globalThis.Math.cos(theta[0]!), n + 1.0);
    }
    else {
      VsdkLogger.reportMessage(this, VsdkLogger.ERROR, "Photon::findRS", JavaString.vformat("Component %d not implemented yet", [flag]));
    }
  }
}
