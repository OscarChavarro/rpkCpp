/**
Higher order approximations for Galerkin radiosity
*/

import { ColorRgb } from "../../common/ColorRgb";
import { Error as VsdkError } from "../../common/Error";
import { Matrix2x2 } from "../../common/linealAlgebra/Matrix2x2";
import { Vector2D } from "../../common/linealAlgebra/Vector2D";
import { CubatureRule } from "../../numericalAnalysis/CubatureRule";
import { QuadCubatureRule } from "../../numericalAnalysis/QuadCubatureRule";
import { TriangleCubatureRule } from "../../numericalAnalysis/TriangleCubatureRule";
import { GalerkinBasis } from "./GalerkinBasis";
import { StochasticRadiosityBasisState } from "./StochasticRadiosityBasisState";
import { StochasticRadiosityElementType } from "./StochasticRadiosityElementType";
import { StochasticRadiosityElementTypeInfo } from "./StochasticRadiosityElementTypeInfo";
import { StochasticRaytracingApproximation } from "./StochasticRaytracingApproximation";

export class Basismcrad {
  private constructor() {
  }

  public static oneBasis(u: number, v: number): number {
    void u;
    void v;
    return 1;
  }

  public static makeBasis(et: StochasticRadiosityElementType, at: StochasticRaytracingApproximation): GalerkinBasis {
    const basisState = StochasticRadiosityBasisState.activeState();
    let basis = basisState.quadBasis;
    let elem = "";

    switch (et) {
      case StochasticRadiosityElementType.ET_TRIANGLE:
        basis = basisState.triBasis;
        elem = "triangles";
        break;
      case StochasticRadiosityElementType.ET_QUAD:
        basis = basisState.quadBasis;
        elem = "quadrilaterals";
        break;
      default:
        VsdkError.fatal(-1, "Basismcrad::makeBasis", "Invalid element type %d", et);
        return basis;
    }

    const out = Basismcrad.cloneBasis(basis);
    out.size = basisState.approxDesc[at].basis_size;
    out.description = `${basisState.approxDesc[at].name} orthonormal basis for ${elem}`;

    return out;
  }

  /**
Computes the filter coefficients for push-pull operations between a
parent and child with given basis and nr of basis functions. 'upxfm' is
the transform to be used to find the point on the parent corresponding
to a given point on the child. 'cr' is the cubature rule to be used
for computing the coefficients. The order should be at least the highest
product of the order of a parent and a child basis function. The filter
coefficients are filled in in the table 'filter'. The filter coefficients are:

H_{\alpha\,\beta} = int _S phi_\alpha(u',v') phi_\beta(u,v) du dv

with S the domain on which the basis functions are defined (unit square or
standard triangle), and (u',v') the result of "up-transforming" (u,v).
*/
  public static computeFilterCoefficients(
    parentBasis: GalerkinBasis,
    parentSize: number,
    childBasis: GalerkinBasis,
    childSize: number,
    upxfm: Matrix2x2,
    cr: CubatureRule,
    filter: number[][]
  ): void {
    for (let a = 0; a < parentSize; a++) {
      for (let b = 0; b < childSize; b++) {
        let x = 0.0;
        for (let k = 0; k < cr.numberOfNodes; k++) {
          const up = new Vector2D(cr.u[k], cr.v[k]);
          upxfm.transformPoint2D(up, up);
          x += cr.w[k] * parentBasis.function![a](up.x, up.y) * childBasis.function![b](cr.u[k], cr.v[k]);
        }
        filter[a][b] = x;
      }
    }
  }

  /**
Computes the push-pull filter coefficients for regular subdivision for
elements with given basis and uptransform. The cubature rule 'cr' is used
to compute the coefficients. The coefficients are filled in the
basis->regular_filter table
*/
  public static basisGalerkinComputeRegularFilterCoefficients(
    basis: GalerkinBasis,
    upxfm: Matrix2x2[],
    cr: CubatureRule
  ): void {
    for (let s = 0; s < 4; s++) {
      Basismcrad.computeFilterCoefficients(
        basis,
        basis.size,
        basis,
        basis.size,
        upxfm[s],
        cr,
        basis.regularFilter![s]
      );
    }
  }

  /**
Initialises table of bases
*/
  public static monteCarloRadiosityInitBasis(): void {
    const basisState = StochasticRadiosityBasisState.activeState();
    if (basisState.inited) {
      return;
    }

    Basismcrad.basisGalerkinComputeRegularFilterCoefficients(
      basisState.triBasis,
      basisState.triangleUpTransform,
      TriangleCubatureRule.degree8Rule()
    );
    Basismcrad.basisGalerkinComputeRegularFilterCoefficients(
      basisState.quadBasis,
      basisState.quadUpTransform,
      QuadCubatureRule.degree8QuadrilateralRule()
    );

    for (let et = 0; et < StochasticRadiosityElementTypeInfo.NUMBER_OF_ELEMENT_TYPES; et++) {
      for (let at = 0; at < StochasticRadiosityBasisState.NUMBER_OF_APPROXIMATION_TYPES; at++) {
        basisState.basis[et][at] = Basismcrad.makeBasis(et as StochasticRadiosityElementType, at as StochasticRaytracingApproximation);
      }
    }
    basisState.inited = true;
  }

  /**
Returns color at a given point, with parameters (u,v)
*/
  public static colorAtUv(basis: GalerkinBasis, rad: ColorRgb[], u: number, v: number): ColorRgb {
    const res = new ColorRgb();
    res.clear();
    for (let i = 0; i < basis.size; i++) {
      const s = basis.function![i](u, v);
      res.addScaled(res, s, rad[i]);
    }
    return res;
  }

  /**
These routine filter the source coefficients down/up and add
the result to the destination coefficients
*/
  public static filterColorDown(parent: ColorRgb[], h: number[][], child: ColorRgb[], n: number): void {
    for (let i = 0; i < n; i++) {
      for (let j = 0; j < n; j++) {
        child[i].addScaled(child[i], h[j][i], parent[j]);
      }
    }
  }

  public static filterColorUp(child: ColorRgb[], h: number[][], parent: ColorRgb[], n: number, areaFactor: number): void {
    for (let i = 0; i < n; i++) {
      for (let j = 0; j < n; j++) {
        const H = h[i][j] * areaFactor;
        parent[i].addScaled(parent[i], H, child[j]);
      }
    }
  }

  public static stochasticRadiosityCreateQuadBasis(): GalerkinBasis {
    return Basismcrad.Basisquadmcrad_stochasticRadiosityCreateQuadBasis();
  }

  private static cloneBasis(input: GalerkinBasis): GalerkinBasis {
    const out = new GalerkinBasis();
    out.description = input.description;
    out.size = input.size;
    out.function = input.function;
    out.dualFunction = input.dualFunction;
    out.regularFilter = input.regularFilter;
    return out;
  }

  private static qm0(u: number, v: number): number {
    void u;
    void v;
    return 1.000000000000000;
  }

  private static qm1(u: number, v: number): number {
    void v;
    return -1.732050807568877 + 3.464101615137753 * u;
  }

  private static qm2(u: number, v: number): number {
    void u;
    return -1.732050807568877 + 3.464101615137753 * v;
  }

  private static qm3(u: number, v: number): number {
    return 3.000000000000003 + -6.000000000000006 * u + -6.000000000000009 * v + 12.000000000000021 * u * v;
  }

  private static qm4(u: number, v: number): number {
    void v;
    return 2.236067977499749 + -13.416407864998552 * u + 13.416407864998591 * u * u;
  }

  private static qm5(u: number, v: number): number {
    void u;
    return 2.236067977499781 + -13.416407864998723 * v + 13.416407864998760 * v * v;
  }

  private static qm6(u: number, v: number): number {
    void v;
    return -2.645751311064023 + 31.749015732770424 * u + -79.372539331927356 * u * u + 52.915026221285316 * u * u * u;
  }

  private static qm7(u: number, v: number): number {
    return -3.872983346207165 + 23.237900077242056 * u + 7.745966692414697 * v + -46.475800154488844 * u * v
      + -23.237900077239200 * u * u + 46.475800154488617 * u * u * v;
  }

  private static qm8(u: number, v: number): number {
    return -3.872983346207866 + 7.745966692416303 * u + 23.237900077246348 * v + -46.475800154495623 * u * v
      + -23.237900077245619 * v * v + 46.475800154491409 * u * v * v;
  }

  private static qm9(u: number, v: number): number {
    void u;
    return -2.645751311064409 + 31.749015732781054 * v + -79.372539331951486 * v * v + 52.915026221299712 * v * v * v;
  }

  private static create3DArray(a: number, b: number, c: number): number[][][] {
    const out = new Array<number[][]>(a);
    for (let i = 0; i < a; i++) {
      out[i] = new Array<number[]>(b);
      for (let j = 0; j < b; j++) {
        out[i][j] = new Array<number>(c).fill(0.0);
      }
    }
    return out;
  }

  private static Basisquadmcrad_stochasticRadiosityCreateQuadBasis(): GalerkinBasis {
    const f = [
      (u: number, v: number): number => Basismcrad.qm0(u, v),
      (u: number, v: number): number => Basismcrad.qm1(u, v),
      (u: number, v: number): number => Basismcrad.qm2(u, v),
      (u: number, v: number): number => Basismcrad.qm3(u, v),
      (u: number, v: number): number => Basismcrad.qm4(u, v),
      (u: number, v: number): number => Basismcrad.qm5(u, v),
      (u: number, v: number): number => Basismcrad.qm6(u, v),
      (u: number, v: number): number => Basismcrad.qm7(u, v),
      (u: number, v: number): number => Basismcrad.qm8(u, v),
      (u: number, v: number): number => Basismcrad.qm9(u, v),
    ];
    const h = Basismcrad.create3DArray(4, GalerkinBasis.MAX_BASIS_SIZE, GalerkinBasis.MAX_BASIS_SIZE);
    const b = new GalerkinBasis();
    b.description = "orthonormal basis on the unit square";
    b.size = GalerkinBasis.MAX_BASIS_SIZE;
    b.function = f;
    b.dualFunction = f;
    b.regularFilter = h;
    return b;
  }
}
