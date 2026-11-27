import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { StochasticRadiosityBasis } from "./StochasticRadiosityBasis";
import { McradP } from "./McradP";
import { StochasticRadiosityBasisState } from "./StochasticRadiosityBasisState";
import { StochasticRadiosityElementType } from "./StochasticRadiosityElementType";
import { StochasticRelaxation } from "./StochasticRelaxation";

export class Coefficientsmcrad {
  private constructor() {
  }

  public static stochasticRadiosityClearCoefficients(c: ColorRgbMutable[] | null, galerkinBasis: StochasticRadiosityBasis | null): void {
    if (c === null || galerkinBasis === null) {
      return;
    }
    for (let i = 0; i < galerkinBasis.size; i++) {
      if (c[i] === undefined || c[i] === null) {
        c[i] = new ColorRgbMutable();
      }
      c[i]!.clear();
    }
  }

  public static stochasticRadiosityCopyCoefficients(dst: ColorRgbMutable[] | null, src: ColorRgbMutable[] | null, galerkinBasis: StochasticRadiosityBasis | null): void {
    if (dst === null || src === null || galerkinBasis === null) {
      return;
    }
    for (let i = 0; i < galerkinBasis.size; i++) {
      if (dst[i] === undefined || dst[i] === null) {
        dst[i] = new ColorRgbMutable();
      }
      if (src[i] === undefined || src[i] === null) {
        src[i] = new ColorRgbMutable();
      }
      dst[i]!.set(src[i]!.r, src[i]!.g, src[i]!.b);
    }
  }

  public static stochasticRadiosityAddCoefficients(dst: ColorRgbMutable[] | null, extra: ColorRgbMutable[] | null, galerkinBasis: StochasticRadiosityBasis | null): void {
    if (dst === null || extra === null || galerkinBasis === null) {
      return;
    }
    for (let i = 0; i < galerkinBasis.size; i++) {
      if (dst[i] === undefined || dst[i] === null) {
        dst[i] = new ColorRgbMutable();
      }
      if (extra[i] === undefined || extra[i] === null) {
        extra[i] = new ColorRgbMutable();
      }
      dst[i]!.add(dst[i]!, extra[i]!);
    }
  }

  public static stochasticRadiosityScaleCoefficients(scale: number, color: ColorRgbMutable[] | null, galerkinBasis: StochasticRadiosityBasis | null): void {
    if (color === null || galerkinBasis === null) {
      return;
    }
    for (let i = 0; i < galerkinBasis.size; i++) {
      if (color[i] === undefined || color[i] === null) {
        color[i] = new ColorRgbMutable();
      }
      color[i]!.scale(scale);
    }
  }

  public static stochasticRadiosityMultiplyCoefficients(color: ColorRgbMutable | null, coefficients: ColorRgbMutable[] | null, galerkinBasis: StochasticRadiosityBasis | null): void {
    if (coefficients === null || galerkinBasis === null || color === null) {
      return;
    }
    const c = new ColorRgbMutable(color.r, color.g, color.b);

    for (let i = 0; i < galerkinBasis.size; i++) {
      if (coefficients[i] === undefined || coefficients[i] === null) {
        coefficients[i] = new ColorRgbMutable();
      }
      coefficients[i]!.selfScalarProduct(c);
    }
  }

  /**
Disposes previously allocated coefficients
*/
  public static disposeCoefficients(elem: any): void {
    if (
      elem.basis !== null
      && elem.basis !== StochasticRadiosityBasisState.activeState().dummyBasis
      && elem.radiance !== null
    ) {
      elem.radiance = null;
      elem.unShotRadiance = null;
      elem.receivedRadiance = null;
    }
    Coefficientsmcrad.initCoefficients(elem);
  }

  /**
Determines basis based on element type and currently desired approximation
*/
  private static actualBasis(elem: any): StochasticRadiosityBasis {
    if (elem.isCluster()) {
      return StochasticRadiosityBasisState.activeState().clusterBasis;
    }

    const et = McradP.numberOfVertices(elem) === 3
      ? StochasticRadiosityElementType.ET_TRIANGLE
      : StochasticRadiosityElementType.ET_QUAD;
    const at = StochasticRelaxation.activeState().approximationOrderType!;
    return StochasticRadiosityBasisState.activeState().basis[et]![at]!;
  }

  /**
Allocates memory for radiance coefficients
*/
  public static allocCoefficients(elem: any): void {
    Coefficientsmcrad.disposeCoefficients(elem);
    elem.basis = Coefficientsmcrad.actualBasis(elem);
    elem.radiance = Coefficientsmcrad.createColors(elem.basis.size);
    elem.unShotRadiance = Coefficientsmcrad.createColors(elem.basis.size);
    elem.receivedRadiance = Coefficientsmcrad.createColors(elem.basis.size);
  }

  /**
Re-allocates memory for radiance coefficients if
the currently desired approximation order is not the same
as the approximation order for which the element has
been initialised before
*/
  public static reAllocCoefficients(elem: any): void {
    if (elem !== null && elem.basis !== Coefficientsmcrad.actualBasis(elem)) {
      Coefficientsmcrad.allocCoefficients(elem);
    }
  }

  /**
Basically sets rad to nullptr
*/
  public static initCoefficients(elem: any): void {
    if (!elem.constructor.coefficientPoolsAreInitialized()) {
      elem.constructor.markCoefficientPoolsInitialized();
    }

    elem.radiance = null;
    elem.unShotRadiance = null;
    elem.receivedRadiance = null;
    elem.basis = StochasticRadiosityBasisState.activeState().dummyBasis;
  }

  private static createColors(n: number): ColorRgbMutable[] {
    const data = new Array<ColorRgbMutable>(n);
    for (let i = 0; i < n; i++) {
      data[i] = new ColorRgbMutable();
    }
    return data;
  }
}
