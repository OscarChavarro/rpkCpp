import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Background } from "./Background";

export class ConstantColorBackground extends Background {
  private static readonly FOUR_PI = 12.56637061435917295385;
  private static readonly INV_FOUR_PI = 0.07957747154594766788;

  private color: ColorRgbMutable;

  public constructor();
  public constructor(backgroundColor: ColorRgbMutable);
  public constructor(backgroundColor?: ColorRgbMutable) {
    super();
    this.color = new ColorRgbMutable();
    if (backgroundColor !== undefined) {
      this.color = new ColorRgbMutable(backgroundColor.r, backgroundColor.g, backgroundColor.b);
    }
    else {
      this.color.clear();
    }
  }

  public override radiance(
    position: Vector3Dd,
    direction: Vector3Dd,
    probabilityDensityFunction: number[] | null
  ): ColorRgbMutable {
    if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
      probabilityDensityFunction[0] = ConstantColorBackground.INV_FOUR_PI;
    }
    void position;
    void direction;
    return this.color;
  }

  public override sample(
    position: Vector3Dd,
    xi1: number,
    xi2: number,
    radianceValue: ColorRgbMutable | null,
    probabilityDensityFunction: number[] | null
  ): Vector3Dd {
    const phi = 2.0 * globalThis.Math.PI * xi1;
    const z = 1.0 - 2.0 * xi2;
    const radialSquared = globalThis.Math.max(0.0, 1.0 - z * z);
    const radius = globalThis.Math.sqrt(radialSquared);

    if (radianceValue !== null) {
      radianceValue.set(this.color.r, this.color.g, this.color.b);
    }
    if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
      probabilityDensityFunction[0] = ConstantColorBackground.INV_FOUR_PI;
    }

    let direction = new Vector3Dd();
    direction = new Vector3Dd(radius * globalThis.Math.cos(phi), radius * globalThis.Math.sin(phi), z);
    void position;
    return direction;
  }

  public override power(position: Vector3Dd): ColorRgbMutable {
    const emittedPower = new ColorRgbMutable();
    emittedPower.scaledCopy(ConstantColorBackground.FOUR_PI, this.color);
    void position;
    return emittedPower;
  }
}
