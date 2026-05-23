import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { CoordinateSystem } from "../../common/linealAlgebra/CoordinateSystem";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { BsdfComponent } from "../../material/BsdfComponent";

// Non-compact photon representation
export class Photon {
  protected m_pos: Vector3D;
  protected m_power: ColorRgb;
  protected m_dir: Vector3D;

  public constructor();
  public constructor(pos: Vector3D, power: ColorRgb, dir: Vector3D);
  public constructor(pos?: Vector3D, power?: ColorRgb, dir?: Vector3D) {
    this.m_pos = pos === undefined ? new Vector3D() : new Vector3D(pos.x, pos.y, pos.z);
    this.m_power = power === undefined ? new ColorRgb() : new ColorRgb(power.r, power.g, power.b);
    this.m_dir = dir === undefined ? new Vector3D() : new Vector3D(dir.x, dir.y, dir.z);
  }

  public pos(): Vector3D {
    return this.m_pos;
  }

  public power(): ColorRgb {
    return this.m_power;
  }

  public addPower(col: ColorRgb): void {
    this.m_power.add(this.m_power, col);
  }

  public dir(): Vector3D {
    return this.m_dir;
  }

  public findRS(r: number[], s: number[], coord: CoordinateSystem, flag: number, n: number): void {
    const phi = [0.0];
    const theta = [0.0];
    coord.rectangularToSphericalCoord(this.m_dir, phi, theta);

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
      VsdkLogger.error("Photon::findRS", "Component %d not implemented yet", flag);
    }
  }
}
