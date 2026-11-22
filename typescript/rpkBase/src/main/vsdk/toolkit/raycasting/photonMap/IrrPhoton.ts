import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { Photon } from "./Photon";

// IrrPhoton: photon with extra irradiance info
export class IrrPhoton extends Photon {
  public m_normal: Vector3Df;
  public m_irradiance: ColorRgbMutable;

  public constructor() {
    super();
    this.m_normal = new Vector3Df();
    this.m_irradiance = new ColorRgbMutable();
  }

  public Normal(): Vector3Df {
    return this.m_normal;
  }

  public setNormal(normal: Vector3Df): void {
    this.m_normal = new Vector3Df(normal.x(), normal.y(), normal.z());
  }

  public SetIrradiance(irr: ColorRgbMutable): void {
    this.m_irradiance = new ColorRgbMutable(irr.r, irr.g, irr.b);
  }

  public copy(photon: Photon): void {
    this.m_pos = new Vector3Df(photon.pos().x(), photon.pos().y(), photon.pos().z());
    this.m_power = new ColorRgbMutable(photon.power().r, photon.power().g, photon.power().b);
    this.m_dir = new Vector3Df(photon.dir().x(), photon.dir().y(), photon.dir().z());
  }
}

