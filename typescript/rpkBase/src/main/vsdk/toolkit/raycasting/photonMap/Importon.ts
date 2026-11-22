import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
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
    pos: Vector3Df,
    importance: number,
    potential: number,
    footprint: number,
    dir: Vector3Df
  );
  public constructor(
    pos?: Vector3Df,
    importance?: number,
    potential?: number,
    footprint?: number,
    dir?: Vector3Df
  ) {
    super();
    if (pos !== undefined && importance !== undefined && potential !== undefined && footprint !== undefined && dir !== undefined) {
      this.m_pos = new Vector3Df(pos.x(), pos.y(), pos.z());
      this.m_dir = new Vector3Df(dir.x(), dir.y(), dir.z());
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

