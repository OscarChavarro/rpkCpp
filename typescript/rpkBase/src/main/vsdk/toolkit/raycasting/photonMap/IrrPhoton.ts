import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Photon } from "./Photon";

// IrrPhoton: photon with extra irradiance info
export class IrrPhoton extends Photon {
  public m_normal: Vector3Dd;
  public m_irradiance: ColorRgbMutable;

  public constructor() {
    super();
    this.m_normal = new Vector3Dd();
    this.m_irradiance = new ColorRgbMutable();
  }

  public Normal(): Vector3Dd {
    return this.m_normal;
  }

  public setNormal(normal: Vector3Dd): void {
    this.m_normal = new Vector3Dd(normal.x(), normal.y(), normal.z());
  }

  public SetIrradiance(irr: ColorRgbMutable): void {
    this.m_irradiance = new ColorRgbMutable(irr.r, irr.g, irr.b);
  }

  public copy(photon: Photon): void {
    this.m_pos = new Vector3Dd(photon.pos().x(), photon.pos().y(), photon.pos().z());
    this.m_power = new ColorRgbMutable(photon.power().r, photon.power().g, photon.power().b);
    this.m_dir = new Vector3Dd(photon.dir().x(), photon.dir().y(), photon.dir().z());
  }
}

