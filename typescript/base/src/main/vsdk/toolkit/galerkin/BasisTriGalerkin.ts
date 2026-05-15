import { GalerkinBasis } from "./GalerkinBasis";

export class BasisTriGalerkin {
  private static instanceValue: GalerkinBasis | null = null;

  public static instance(): GalerkinBasis {
    if (BasisTriGalerkin.instanceValue !== null) {
      return BasisTriGalerkin.instanceValue;
    }

    const triBasis = new GalerkinBasis();
    triBasis.description = "orthonormal basis for the standard triangle";
    triBasis.size = GalerkinBasis.MAX_BASIS_SIZE;
    triBasis.function[0] = BasisTriGalerkin.tg0;
    triBasis.function[1] = BasisTriGalerkin.tg1;
    triBasis.function[2] = BasisTriGalerkin.tg2;
    triBasis.function[3] = BasisTriGalerkin.tg3;
    triBasis.function[4] = BasisTriGalerkin.tg4;
    triBasis.function[5] = BasisTriGalerkin.tg5;
    triBasis.function[6] = BasisTriGalerkin.tg6;
    triBasis.function[7] = BasisTriGalerkin.tg7;
    triBasis.function[8] = BasisTriGalerkin.tg8;
    triBasis.function[9] = BasisTriGalerkin.tg9;
    BasisTriGalerkin.instanceValue = triBasis;
    return BasisTriGalerkin.instanceValue;
  }

  private constructor() {
  }

  private static tg0(_u: number, _v: number): number {
    return 1.000000000000000;
  }

  private static tg1(u: number, _v: number): number {
    return -1.414213562373095 * 1 + 4.242640687119287 * u;
  }

  private static tg2(u: number, v: number): number {
    return -2.449489742783179 * 1 + 2.449489742783180 * u + 4.898979485566360 * v;
  }

  private static tg3(u: number, v: number): number {
    return 1.133893419027696 * 1 + -4.535573676110755 * u + -4.535573676110757 * v + 22.677868380553690 * u * v;
  }

  private static tg4(u: number, v: number): number {
    return 3.273268353539930 * 1 + -22.258224804071368 * u + -3.927922024247956 * v + 19.639610121239613 * u * v
      + 22.912878474779255 * u * u;
  }

  private static tg5(u: number, v: number): number {
    return 3.872983346207630 * 1 + -7.745966692415757 * u + -23.237900077245097 * v + 23.237900077245847 * u * v
      + 3.872983346208171 * u * u + 23.237900077244831 * v * v;
  }

  private static tg6(u: number, v: number): number {
    return -1.999999999999970 * 1 + 30.000000000000028 * u + 0.000000000000188 * v + 0.000000000000721 * u * v
      + -90.000000000002487 * u * u + -0.000000000000351 * v * v + 70.000000000003141 * u * u * u;
  }

  private static tg7(u: number, v: number): number {
    return -3.464101615137785 * 1 + 45.033320996788269 * u + 6.928203230276860 * v + -83.138438763305757 * u * v
      + -114.315353299539055 * u * u + -0.000000000001427 * v * v + 72.746133917888613 * u * u * u
      + 145.492267835783281 * u * u * v;
  }

  private static tg8(u: number, v: number): number {
    return -4.472135955000763 * 1 + 40.249223595002263 * u + 26.832815730003585 * v + -214.662525840012592 * u * v
      + -67.082039324999926 * u * u + -26.832815730002483 * v * v + 31.304951684997551 * u * u * u
      + 187.829710110013110 * u * u * v + 187.829710110001940 * u * v * v;
  }

  private static tg9(u: number, v: number): number {
    return -5.291502622131427 * 1 + 15.874507866401922 * u + 63.498031465565624 * v + -126.996062931158960 * u * v
      + -15.874507866410964 * u * u + -158.745078663906781 * v * v + 5.291502622139829 * u * u * u
      + 63.498031465601095 * u * u * v + 158.745078663922413 * u * v * v + 105.830052442603559 * v * v * v;
  }
}
