import { Numeric } from "../../common/linealAlgebra/Numeric";
import { GalerkinBasis } from "../GalerkinBasis";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinRole } from "../GalerkinRole";
import { GalerkinState } from "../GalerkinState";
import { Interaction } from "../Interaction";

export class LinkingClusteredStrategy {
  private static readonly scalarPool: number[][] = [];
  private static readonly matrixPool: number[][] = [];

  private static borrowScalar(): number[] {
    const v = LinkingClusteredStrategy.scalarPool.pop();
    return v !== undefined ? v : [0.0];
  }

  private static returnScalar(v: number[] | null): void {
    if (v !== null && v.length === 1) {
      LinkingClusteredStrategy.scalarPool.push(v);
    }
  }

  private static borrowMatrix(): number[] {
    const v = LinkingClusteredStrategy.matrixPool.pop();
    if (v !== undefined) {
      return v;
    }
    return new Array<number>(receiverMatrixSize()).fill(0.0);
  }

  private static returnMatrix(v: number[] | null): void {
    if (v !== null && v.length === receiverMatrixSize()) {
      LinkingClusteredStrategy.matrixPool.push(v);
    }
  }

  public static createInitialLinks(
    element: GalerkinElement,
    role: GalerkinRole,
    galerkinState: GalerkinState,
  ): void {
    let receiverElement: GalerkinElement | null;
    let sourceElement: GalerkinElement | null;

    switch (role) {
      case GalerkinRole.RECEIVER:
        receiverElement = element;
        sourceElement = galerkinState.topCluster;
        break;
      case GalerkinRole.SOURCE:
        sourceElement = element;
        receiverElement = galerkinState.topCluster;
        break;
      default:
        return;
    }

    if (receiverElement === null || sourceElement === null) {
      return;
    }

    let n = receiverElement.basisSize * sourceElement.basisSize;
    let K: number[];
    if (n === 1) {
      K = LinkingClusteredStrategy.borrowScalar();
      K[0] = 0.0;
    }
    else {
      K = LinkingClusteredStrategy.borrowMatrix();
      if (n <= 0) {
        n = 1;
      }
      for (let i = 0; i < n; i++) {
        K[i] = 0.0;
      }
    }
    const deltaK = LinkingClusteredStrategy.borrowScalar();
    deltaK[0] = Numeric.HUGE_FLOAT_VALUE;

    const newLink = new Interaction(
      receiverElement,
      sourceElement,
      K,
      deltaK,
      receiverElement.basisSize,
      sourceElement.basisSize,
      1,
      128,
    );

    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
      if (sourceElement.interactions === null) {
        sourceElement.interactions = [];
      }
      sourceElement.interactions.push(newLink);
    }
    else {
      if (receiverElement.interactions === null) {
        receiverElement.interactions = [];
      }
      receiverElement.interactions.push(newLink);
    }

    if (receiverElement.basisSize * sourceElement.basisSize === 1) {
      LinkingClusteredStrategy.returnScalar(K);
    }
    else {
      LinkingClusteredStrategy.returnMatrix(K);
    }
    LinkingClusteredStrategy.returnScalar(deltaK);
  }
}

function receiverMatrixSize(): number {
  return GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE;
}
