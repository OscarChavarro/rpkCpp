import { Error as VsdkError } from "../../common/Error";
import { Matrix2x2 } from "../../common/linealAlgebra/Matrix2x2";
import { ApproximationTypeDescription } from "./ApproximationTypeDescription";
import { Basismcrad } from "./Basismcrad";
import { Basistrimcrad } from "./Basistrimcrad";
import { GalerkinBasis } from "./GalerkinBasis";
import { StochasticRadiosityElementTypeInfo } from "./StochasticRadiosityElementTypeInfo";

export class StochasticRadiosityBasisState {
  public static readonly NUMBER_OF_APPROXIMATION_TYPES = 5;

  public approxDesc: ApproximationTypeDescription[];
  public basis: GalerkinBasis[][];
  public triBasis: GalerkinBasis;
  public quadBasis: GalerkinBasis;
  public dummyBasis: GalerkinBasis;
  public clusterBasis: GalerkinBasis;
  public quadUpTransform: Matrix2x2[];
  public triangleUpTransform: Matrix2x2[];
  public inited: boolean;

  public static readonly oneBasisTable = [
    (u: number, v: number): number => Basismcrad.oneBasis(u, v),
  ];

  public constructor() {
    this.approxDesc = new Array<ApproximationTypeDescription>(StochasticRadiosityBasisState.NUMBER_OF_APPROXIMATION_TYPES);
    for (let i = 0; i < this.approxDesc.length; i++) {
      this.approxDesc[i] = new ApproximationTypeDescription();
    }

    this.basis = new Array<GalerkinBasis[]>(StochasticRadiosityElementTypeInfo.NUMBER_OF_ELEMENT_TYPES);
    for (let i = 0; i < this.basis.length; i++) {
      this.basis[i] = new Array<GalerkinBasis>(StochasticRadiosityBasisState.NUMBER_OF_APPROXIMATION_TYPES);
    }

    this.triBasis = Basistrimcrad.createBasis();
    this.quadBasis = Basismcrad.stochasticRadiosityCreateQuadBasis();

    this.dummyBasis = new GalerkinBasis();
    this.dummyBasis.description = "dummy basis";
    this.dummyBasis.size = 0;
    this.dummyBasis.function = null;
    this.dummyBasis.dualFunction = null;
    this.dummyBasis.regularFilter = null;

    this.clusterBasis = new GalerkinBasis();
    this.clusterBasis.description = "cluster basis";
    this.clusterBasis.size = 1;
    this.clusterBasis.function = StochasticRadiosityBasisState.oneBasisTable;
    this.clusterBasis.dualFunction = StochasticRadiosityBasisState.oneBasisTable;
    this.clusterBasis.regularFilter = null;

    this.quadUpTransform = new Array<Matrix2x2>(4);
    this.triangleUpTransform = new Array<Matrix2x2>(4);

    this.approxDesc[0].name = "constant";
    this.approxDesc[0].basis_size = 1;
    this.approxDesc[1].name = "linear";
    this.approxDesc[1].basis_size = 3;
    this.approxDesc[2].name = "bilinear";
    this.approxDesc[2].basis_size = 4;
    this.approxDesc[3].name = "quadratic";
    this.approxDesc[3].basis_size = 6;
    this.approxDesc[4].name = "cubic";
    this.approxDesc[4].basis_size = 10;

    this.quadUpTransform[0] = StochasticRadiosityBasisState.createTransform(0.5, 0.0, 0.0, 0.5, 0.0, 0.0);
    this.quadUpTransform[1] = StochasticRadiosityBasisState.createTransform(0.5, 0.0, 0.0, 0.5, 0.5, 0.0);
    this.quadUpTransform[2] = StochasticRadiosityBasisState.createTransform(0.5, 0.0, 0.0, 0.5, 0.0, 0.5);
    this.quadUpTransform[3] = StochasticRadiosityBasisState.createTransform(0.5, 0.0, 0.0, 0.5, 0.5, 0.5);

    this.triangleUpTransform[0] = StochasticRadiosityBasisState.createTransform(0.5, 0.0, 0.0, 0.5, 0.0, 0.0);
    this.triangleUpTransform[1] = StochasticRadiosityBasisState.createTransform(0.5, 0.0, 0.0, 0.5, 0.5, 0.0);
    this.triangleUpTransform[2] = StochasticRadiosityBasisState.createTransform(0.5, 0.0, 0.0, 0.5, 0.0, 0.5);
    this.triangleUpTransform[3] = StochasticRadiosityBasisState.createTransform(-0.5, 0.0, 0.0, -0.5, 0.5, 0.5);

    this.inited = false;
  }

  public static setActiveState(state: StochasticRadiosityBasisState): void {
    StochasticRadiosityBasisState.activeStatePtr = state;
  }

  public static activeState(): StochasticRadiosityBasisState {
    if (StochasticRadiosityBasisState.activeStatePtr === null) {
      VsdkError.fatal(-1, "StochasticRadiosityBasisState::activeState", "Stochastic radiosity basis state was not initialized");
    }
    return StochasticRadiosityBasisState.activeStatePtr!;
  }

  public static createTransform(m00: number, m01: number, m10: number, m11: number, t0: number, t1: number): Matrix2x2 {
    const transform = new Matrix2x2();
    transform.m[0][0] = m00;
    transform.m[0][1] = m01;
    transform.m[1][0] = m10;
    transform.m[1][1] = m11;
    transform.t[0] = t0;
    transform.t[1] = t1;
    return transform;
  }

  private static activeStatePtr: StochasticRadiosityBasisState | null = null;
}
