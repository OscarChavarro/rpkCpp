import { ColorRgb } from "../../common/color/ColorRgb";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Photon } from "./Photon";

// IrrPhoton: photon with extra irradiance info
export class IrrPhoton extends Photon {
  public m_normal: Vector3D;
  public m_irradiance: ColorRgb;

  public constructor() {
    super();
    this.m_normal = new Vector3D();
    this.m_irradiance = new ColorRgb();
  }

  public Normal(): Vector3D {
    return this.m_normal;
  }

  public setNormal(normal: Vector3D): void {
    this.m_normal = new Vector3D(normal.x, normal.y, normal.z);
  }

  public SetIrradiance(irr: ColorRgb): void {
    this.m_irradiance = new ColorRgb(irr.r, irr.g, irr.b);
  }

  public copy(photon: Photon): void {
    this.m_pos = new Vector3D(photon.pos().x, photon.pos().y, photon.pos().z);
    this.m_power = new ColorRgb(photon.power().r, photon.power().g, photon.power().b);
    this.m_dir = new Vector3D(photon.dir().x, photon.dir().y, photon.dir().z);
  }
}

