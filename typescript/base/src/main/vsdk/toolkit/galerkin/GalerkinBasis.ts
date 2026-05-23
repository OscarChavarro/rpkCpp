import { ColorRgb } from "../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../common/logging/Logger";
import { Matrix2x2 } from "../common/linealAlgebra/Matrix2x2";
import { Vector2D } from "../common/linealAlgebra/Vector2D";
import { CubatureRule } from "../numericalAnalysis/CubatureRule";
import { BasisQuadGalerkin } from "./BasisQuadGalerkin";
import { BasisTriGalerkin } from "./BasisTriGalerkin";
import { GalerkinElement } from "./GalerkinElement";
import { GalerkinIterationMethod } from "./GalerkinIterationMethod";
import { GalerkinState } from "./GalerkinState";

export class GalerkinBasis {
  public static readonly MAX_BASIS_SIZE = 10;

  public description: string;
  public size: number;
  public function: Array<(u: number, v: number) => number>;
  public regularFilter: number[][][];

  public constructor() {
    this.description = "";
    this.size = 0;
    this.function = new Array<(u: number, v: number) => number>(GalerkinBasis.MAX_BASIS_SIZE);
    for (let i = 0; i < this.function.length; i++) {
      this.function[i] = (_u: number, _v: number): number => 0.0;
    }
    this.regularFilter = new Array<number[][]>(4);
    for (let sigma = 0; sigma < 4; sigma++) {
      this.regularFilter[sigma] = new Array<number[]>(GalerkinBasis.MAX_BASIS_SIZE);
      for (let alpha = 0; alpha < GalerkinBasis.MAX_BASIS_SIZE; alpha++) {
        this.regularFilter[sigma]![alpha] = new Array<number>(GalerkinBasis.MAX_BASIS_SIZE).fill(0.0);
      }
    }
  }

  public static radianceAtPoint(
    element: GalerkinElement | null,
    coefficients: ColorRgb[] | null,
    u: number,
    v: number,
  ): ColorRgb {
    const c = new ColorRgb();
    if (element === null || coefficients === null) {
      return c;
    }

    const basis = GalerkinBasis.mutableBasisForVertexCount(element.patch !== null ? element.patch.numberOfVertices : 4);
    if (basis === null) {
      return c;
    }

    const n = Math.min(element.basisUsed, Math.min(basis.size, coefficients.length));
    for (let i = 0; i < n; i++) {
      const f = basis.function[i]!(u, v);
      c.addScaled(c, f, coefficients[i]!);
    }
    return c;
  }

  public static push(
    element: GalerkinElement | null,
    parentCoefficients: ColorRgb[] | null,
    child: GalerkinElement | null,
    childCoefficients: ColorRgb[] | null,
  ): void {
    if (element === null || child === null || parentCoefficients === null || childCoefficients === null) {
      return;
    }

    const sigma = child.childNumber;
    if (element.isCluster()) {
      ColorRgb.arrayClear(childCoefficients, child.basisSize);
      childCoefficients[0]!.set(parentCoefficients[0]!.r, parentCoefficients[0]!.g, parentCoefficients[0]!.b);
      return;
    }

    if (sigma < 0 || sigma > 3) {
      VsdkLogger.error("GalerkinBasis::push", "Not yet implemented for non-regular subdivision");
      ColorRgb.arrayClear(childCoefficients, child.basisSize);
      childCoefficients[0]!.set(parentCoefficients[0]!.r, parentCoefficients[0]!.g, parentCoefficients[0]!.b);
      return;
    }

    const basis = GalerkinBasis.basisForVertexCount(child.patch !== null ? child.patch.numberOfVertices : 4);
    if (basis === null) {
      return;
    }

    const a = Math.min(child.basisSize, childCoefficients.length);
    const b = Math.min(element.basisSize, parentCoefficients.length);
    for (let beta = 0; beta < a; beta++) {
      childCoefficients[beta]!.clear();
      for (let alpha = 0; alpha < b; alpha++) {
        const f = basis.regularFilter[sigma]![alpha]![beta]!;
        childCoefficients[beta]!.addScaled(childCoefficients[beta]!, f, parentCoefficients[alpha]!);
      }
    }
  }

  public static pushPullRadiance(top: GalerkinElement | null, galerkinState: GalerkinState | null): void {
    if (top === null || galerkinState === null) {
      return;
    }
    const bdown = new Array<ColorRgb>(GalerkinBasis.MAX_BASIS_SIZE);
    const bup = new Array<ColorRgb>(GalerkinBasis.MAX_BASIS_SIZE);
    for (let i = 0; i < GalerkinBasis.MAX_BASIS_SIZE; i++) {
      bdown[i] = new ColorRgb();
      bup[i] = new ColorRgb();
    }
    GalerkinBasis.pushPullRadianceRecursive(top, bdown, bup, galerkinState);
  }

  public static computeRegularFilterCoefficients(
    basis: GalerkinBasis | null,
    upTransform: Matrix2x2[] | null,
    cubaRule: CubatureRule | null,
  ): void {
    if (basis === null || upTransform === null || cubaRule === null) {
      return;
    }
    for (let sigma = 0; sigma < 4; sigma++) {
      GalerkinBasis.computeFilterCoefficients(
        basis,
        basis.size,
        basis,
        basis.size,
        upTransform[sigma]!,
        cubaRule,
        basis.regularFilter[sigma]!,
      );
    }
  }

  public static basisForVertexCount(numberOfVertices: number): GalerkinBasis {
    if (numberOfVertices === 3) {
      return BasisTriGalerkin.instance();
    }
    return BasisQuadGalerkin.instance();
  }

  public static mutableBasisForVertexCount(numberOfVertices: number): GalerkinBasis {
    return GalerkinBasis.basisForVertexCount(numberOfVertices);
  }

  private static pull(
    parent: GalerkinElement | null,
    parentCoefficients: ColorRgb[] | null,
    child: GalerkinElement | null,
    childCoefficients: ColorRgb[] | null,
  ): void {
    if (parent === null || parentCoefficients === null || child === null || childCoefficients === null) {
      return;
    }

    const sigma = child.childNumber;
    if (parent.isCluster()) {
      ColorRgb.arrayClear(parentCoefficients, parent.basisSize);
      parentCoefficients[0]!.scaledCopy(parent.area > 0.0 ? child.area / parent.area : 0.0, childCoefficients[0]!);
      return;
    }

    if (sigma < 0 || sigma > 3) {
      VsdkLogger.error("stochasticJacobiPull", "Not yet implemented for non-regular subdivision");
      ColorRgb.arrayClear(parentCoefficients, parent.basisSize);
      parentCoefficients[0] = childCoefficients[0]!;
      return;
    }

    const basis = GalerkinBasis.basisForVertexCount(child.patch !== null ? child.patch.numberOfVertices : 4);
    for (let alpha = 0; alpha < parent.basisSize; alpha++) {
      parentCoefficients[alpha]!.clear();
      for (let beta = 0; beta < child.basisSize; beta++) {
        const f = basis.regularFilter[sigma]![alpha]![beta]!;
        parentCoefficients[alpha]!.addScaled(parentCoefficients[alpha]!, f, childCoefficients[beta]!);
      }
      parentCoefficients[alpha]!.scale(0.25);
    }
  }

  private static pushPullRadianceRecursive(
    element: GalerkinElement,
    bdown: ColorRgb[],
    bup: ColorRgb[],
    galerkinState: GalerkinState,
  ): void {
    const receivedRadiance = element.receivedRadiance as ColorRgb[];
    const radiance = element.radiance as ColorRgb[];
    const unShotRadiance = element.unShotRadiance as ColorRgb[];
    const n = Math.min(element.basisSize, bdown.length);
    for (let i = 0; i < n; i++) {
      bdown[i]!.addScaled(bdown[i]!, element.area > 0.0 ? 1.0 / element.area : 0.0, receivedRadiance[i]!);
      receivedRadiance[i]!.clear();
      bup[i]!.clear();
    }

    if (element.regularSubElements === null && element.irregularSubElements === null && element.patch !== null) {
      const rho = element.patch.radianceData!.Rd;
      for (let i = 0; i < n; i++) {
        bup[i]!.scalarProduct(rho, bdown[i]!);
      }

      if (
        galerkinState.galerkinIterationMethod === GalerkinIterationMethod.JACOBI
        || galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
      ) {
        const Ed = element.patch.radianceData!.Ed;
        bup[0]!.add(bup[0]!, Ed);
      }
    }

    if (element.regularSubElements !== null) {
      for (let i = 0; i < 4; i++) {
        if (!(element.regularSubElements[i] instanceof GalerkinElement)) {
          continue;
        }
        const child = element.regularSubElements[i] as GalerkinElement;
        const btmp = GalerkinBasis.freshColorArray();
        const bdown2 = GalerkinBasis.freshColorArray();
        const bup2 = GalerkinBasis.freshColorArray();
        GalerkinBasis.push(element, bdown, child, bdown2);
        GalerkinBasis.pushPullRadianceRecursive(child, bdown2, btmp, galerkinState);
        GalerkinBasis.pull(element, bup2, child, btmp);
        ColorRgb.arrayAdd(bup, bup2, n);
      }
    }

    if (element.irregularSubElements !== null) {
      for (let i = 0; i < element.irregularSubElements.length; i++) {
        if (!(element.irregularSubElements[i] instanceof GalerkinElement)) {
          continue;
        }
        const subElement = element.irregularSubElements[i] as GalerkinElement;
        const btmp = GalerkinBasis.freshColorArray();
        const bdown2 = GalerkinBasis.freshColorArray();
        const bup2 = GalerkinBasis.freshColorArray();
        if (element.isCluster()) {
          GalerkinBasis.push(element, bdown, subElement, bdown2);
        }
        else {
          ColorRgb.arrayClear(bdown2, n);
        }
        GalerkinBasis.pushPullRadianceRecursive(subElement, bdown2, btmp, galerkinState);
        GalerkinBasis.pull(element, bup2, subElement, btmp);
        ColorRgb.arrayAdd(bup, bup2, n);
      }
    }

    if (
      galerkinState.galerkinIterationMethod === GalerkinIterationMethod.JACOBI
      || galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
    ) {
      ColorRgb.arrayCopy(radiance, bup, n);
    }
    else {
      ColorRgb.arrayAdd(radiance, bup, n);
      ColorRgb.arrayAdd(unShotRadiance, bup, n);
    }
  }

  private static computeFilterCoefficients(
    parentBasis: GalerkinBasis,
    parentSize: number,
    childBasis: GalerkinBasis,
    childSize: number,
    upTransform: Matrix2x2,
    cubatureRule: CubatureRule,
    filter: number[][],
  ): void {
    for (let alpha = 0; alpha < parentSize; alpha++) {
      for (let beta = 0; beta < childSize; beta++) {
        let x = 0.0;
        for (let k = 0; k < cubatureRule.numberOfNodes; k++) {
          const up = new Vector2D(cubatureRule.u[k]!, cubatureRule.v[k]!);
          upTransform.transformPoint2D(up, up);
          x += cubatureRule.w[k]!
            * parentBasis.function[alpha]!(up.x, up.y)
            * childBasis.function[beta]!(cubatureRule.u[k]!, cubatureRule.v[k]!);
        }
        filter[alpha]![beta] = x;
      }
    }
  }

  private static freshColorArray(): ColorRgb[] {
    const c = new Array<ColorRgb>(GalerkinBasis.MAX_BASIS_SIZE);
    for (let i = 0; i < GalerkinBasis.MAX_BASIS_SIZE; i++) {
      c[i] = new ColorRgb();
    }
    return c;
  }
}
