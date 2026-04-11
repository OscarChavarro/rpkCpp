import { ColorRgb } from "../common/ColorRgb";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { Patch } from "../skin/Patch";

export class Background {
  public bkgPatch: Patch | null;

  public constructor() {
    this.bkgPatch = null;
  }

  public radiance(
    position: Vector3D,
    direction: Vector3D,
    probabilityDensityFunction: number[] | null
  ): ColorRgb {
    if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
      probabilityDensityFunction[0] = 0.0;
    }
    const black = new ColorRgb();
    black.setMonochrome(0.0);
    void position;
    void direction;
    return black;
  }

  public sample(
    position: Vector3D,
    xi1: number,
    xi2: number,
    radianceValue: ColorRgb | null,
    probabilityDensityFunction: number[] | null
  ): Vector3D {
    if (radianceValue !== null) {
      radianceValue.setMonochrome(0.0);
    }
    if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
      probabilityDensityFunction[0] = 0.0;
    }
    void position;
    void xi1;
    void xi2;
    return new Vector3D();
  }

  public power(position: Vector3D): ColorRgb {
    const black = new ColorRgb();
    black.setMonochrome(0.0);
    void position;
    return black;
  }

  public static backgroundRadiance(
    bkg: Background | null,
    position: Vector3D,
    direction: Vector3D,
    probabilityDensityFunction: number[] | null
  ): ColorRgb {
    if (bkg === null) {
      const black = new ColorRgb();
      black.setMonochrome(0.0);
      return black;
    }
    return bkg.radiance(position, direction, probabilityDensityFunction);
  }

  public static backgroundPower(bkg: Background | null, position: Vector3D): ColorRgb {
    if (bkg === null) {
      const black = new ColorRgb();
      black.setMonochrome(0.0);
      return black;
    }
    return bkg.power(position);
  }
}
