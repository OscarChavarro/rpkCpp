import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { IrrPhoton } from "./IrrPhoton";

// Importon: identical to IrrPhoton, but with some extra functions
export class Importon extends IrrPhoton {
  public SetAll(imp: number, pot: number, foot: number): void {
    void pot;
    void foot;
    this.m_power.r = imp;
  }

  public PSetAll(imp: number, pot: number, foot: number): void {
    void pot;
    void foot;
    this.m_irradiance.r = imp;
  }

  public constructor();
  public constructor(
    pos: Vector3D,
    importance: number,
    potential: number,
    footprint: number,
    dir: Vector3D
  );
  public constructor(
    pos?: Vector3D,
    importance?: number,
    potential?: number,
    footprint?: number,
    dir?: Vector3D
  ) {
    super();
    if (pos !== undefined && importance !== undefined && potential !== undefined && footprint !== undefined && dir !== undefined) {
      this.m_pos = new Vector3D(pos.x, pos.y, pos.z);
      this.m_dir = new Vector3D(dir.x, dir.y, dir.z);
      this.SetAll(importance, potential, footprint);
    }
  }

  public Importance(): number {
    return this.m_power.r;
  }

  public PImportance(): number {
    return this.m_irradiance.r;
  }
}

