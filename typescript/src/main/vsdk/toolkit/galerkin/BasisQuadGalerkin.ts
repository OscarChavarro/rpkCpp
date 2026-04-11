import { GalerkinBasis } from "./GalerkinBasis";

export class BasisQuadGalerkin {
  private static instanceValue: GalerkinBasis | null = null;

  public static instance(): GalerkinBasis {
    if (BasisQuadGalerkin.instanceValue !== null) {
      return BasisQuadGalerkin.instanceValue;
    }

    const quadBasis = new GalerkinBasis();
    quadBasis.description = "orthonormal basis for the unit square";
    quadBasis.size = GalerkinBasis.MAX_BASIS_SIZE;
    quadBasis.function[0] = BasisQuadGalerkin.qg0;
    quadBasis.function[1] = BasisQuadGalerkin.qg1;
    quadBasis.function[2] = BasisQuadGalerkin.qg2;
    quadBasis.function[3] = BasisQuadGalerkin.qg3;
    quadBasis.function[4] = BasisQuadGalerkin.qg4;
    quadBasis.function[5] = BasisQuadGalerkin.qg5;
    quadBasis.function[6] = BasisQuadGalerkin.qg6;
    quadBasis.function[7] = BasisQuadGalerkin.qg7;
    quadBasis.function[8] = BasisQuadGalerkin.qg8;
    quadBasis.function[9] = BasisQuadGalerkin.qg9;
    BasisQuadGalerkin.instanceValue = quadBasis;
    return BasisQuadGalerkin.instanceValue;
  }

  private constructor() {
  }

  private static qg0(_u: number, _v: number): number {
    return 1.000000000000000;
  }

  private static qg1(u: number, _v: number): number {
    return -1.732050807568877 + 3.464101615137753 * u;
  }

  private static qg2(_u: number, v: number): number {
    return -1.732050807568877 + 3.464101615137753 * v;
  }

  private static qg3(u: number, v: number): number {
    return 3.000000000000003 + -6.000000000000006 * u + -6.000000000000009 * v + 12.000000000000021 * u * v;
  }

  private static qg4(u: number, _v: number): number {
    return 2.236067977499749 + -13.416407864998552 * u + 13.416407864998591 * u * u;
  }

  private static qg5(_u: number, v: number): number {
    return 2.236067977499781 + -13.416407864998723 * v + 13.416407864998760 * v * v;
  }

  private static qg6(u: number, _v: number): number {
    return -2.645751311064023 + 31.749015732770424 * u + -79.372539331927356 * u * u + 52.915026221285316 * u * u * u;
  }

  private static qg7(u: number, v: number): number {
    return -3.872983346207165 + 23.237900077242056 * u + 7.745966692414697 * v + -46.475800154488844 * u * v
      + -23.237900077239200 * u * u + 46.475800154488617 * u * u * v;
  }

  private static qg8(u: number, v: number): number {
    return -3.872983346207866
      + 7.745966692416303 * u
      + 23.237900077246348 * v
      + -46.475800154495623 * u * v
      + -23.237900077245619 * v * v
      + 46.475800154491409 * u * v * v;
  }

  private static qg9(_u: number, v: number): number {
    return -2.645751311064409 + 31.749015732781054 * v + -79.372539331951486 * v * v + 52.915026221299712 * v * v * v;
  }
}
