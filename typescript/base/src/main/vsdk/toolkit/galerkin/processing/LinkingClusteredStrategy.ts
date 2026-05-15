import { Numeric } from "../../common/linealAlgebra/Numeric";
import { MemoryPool } from "../../common/memoryManagement/MemoryPool";
import { GalerkinBasis } from "../GalerkinBasis";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinRole } from "../GalerkinRole";
import { GalerkinState } from "../GalerkinState";
import { Interaction } from "../Interaction";

export class LinkingClusteredStrategy {
  private static readonly linkingClusteredPool = new MemoryPool.Generic<number[]>(
    () => new Array<number>(receiverMatrixSize()).fill(0.0)
  );
  private static linkingClusteredPoolInitialized = false;

  private static ensureLinkingClusteredPool(): void {
    if (LinkingClusteredStrategy.linkingClusteredPoolInitialized) {
      return;
    }
    LinkingClusteredStrategy.linkingClusteredPool.init(2048);
    if (LinkingClusteredStrategy.linkingClusteredPool.allocate(1) === null) {
      LinkingClusteredStrategy.linkingClusteredPool.expand(64);
    }
    LinkingClusteredStrategy.linkingClusteredPool.clear();
    LinkingClusteredStrategy.linkingClusteredPoolInitialized = true;
  }

  private static borrowBuffer(): number[] {
    LinkingClusteredStrategy.ensureLinkingClusteredPool();
    let v = LinkingClusteredStrategy.linkingClusteredPool.allocate(1);
    if (v === null) {
      const expanded = LinkingClusteredStrategy.linkingClusteredPool.expand(64);
      if (!expanded) {
        return new Array<number>(receiverMatrixSize()).fill(0.0);
      }
      v = LinkingClusteredStrategy.linkingClusteredPool.allocate(1);
      if (v === null) {
        return new Array<number>(receiverMatrixSize()).fill(0.0);
      }
    }
    return v;
  }

  private static returnBuffer(v: number[] | null): void {
    if (v !== null && LinkingClusteredStrategy.linkingClusteredPool.owns(v)) {
      LinkingClusteredStrategy.linkingClusteredPool.free(1);
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
    const K = LinkingClusteredStrategy.borrowBuffer();
    if (n <= 0) {
      n = 1;
    }
    for (let i = 0; i < n; i++) {
      K[i] = 0.0;
    }
    const deltaK = LinkingClusteredStrategy.borrowBuffer();
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

    LinkingClusteredStrategy.returnBuffer(K);
    LinkingClusteredStrategy.returnBuffer(deltaK);
  }
}

function receiverMatrixSize(): number {
  return GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE;
}
