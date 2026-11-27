import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
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
    pos: Vector3Dd,
    importance: number,
    potential: number,
    footprint: number,
    dir: Vector3Dd
  );
  public constructor(
    pos?: Vector3Dd,
    importance?: number,
    potential?: number,
    footprint?: number,
    dir?: Vector3Dd
  ) {
    super();
    if (pos !== undefined && importance !== undefined && potential !== undefined && footprint !== undefined && dir !== undefined) {
      this.m_pos = new Vector3Dd(pos.x(), pos.y(), pos.z());
      this.m_dir = new Vector3Dd(dir.x(), dir.y(), dir.z());
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

