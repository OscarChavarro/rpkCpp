import { Ray } from "../../common/linealAlgebra/Ray";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import type { ShadowStatistics } from "../../common/statistics/ShadowStatistics";
import type { GalerkinElement } from "../GalerkinElement";
import { CubatureRule } from "../../numericalAnalysis/CubatureRule";

/**
Scratch of the area to area form factor computation. Very often the source or
receiver element is the same as in the previous computation, so the cubature
rules, nodes and kernel values of the last element pair are kept here and
reused.

Workspace: mutable, owned by the solver (the serial one) or by a parallel
Jacobi worker, and passed explicitly to FormFactorStrategy.
It also tells where the shadow rays are counted (the global statistics, or the
counters of a worker, added to them afterwards).

TypeScript adaptation: the ray, hit record and distance cells that the C++
port keeps on the stack live here too, so the computation allocates nothing.
*/
export class FormFactorWorkspace {
  private lastReceiver: GalerkinElement | null = null;
  private lastSource: GalerkinElement | null = null;
  private receiverCubatureRule: CubatureRule | null = null;
  private sourceCubatureRule: CubatureRule | null = null;
  private readonly receiverNodes: Vector3Df[];
  private readonly sourceNodes: Vector3Df[];
  private readonly kernel: number[][];
  private maximumKernelValue = 0.0;
  private visibilityCount = 0; // Number of rays that "pass" occluders
  private readonly shadowStatistics: ShadowStatistics; // Not owned

  private readonly kernelRay = new Ray();
  private readonly kernelHitStore = new RayHit();
  private readonly kernelDistance = [0.0];
  private readonly shadowTestDistance = [0.0];

  public constructor(inShadowStatistics: ShadowStatistics) {
    this.shadowStatistics = inShadowStatistics;
    this.receiverNodes = [];
    this.sourceNodes = [];
    this.kernel = [];
    for (let i = 0; i < CubatureRule.MAXIMUM_NODES; i++) {
      this.receiverNodes.push(new Vector3Df());
      this.sourceNodes.push(new Vector3Df());
      this.kernel.push(new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0));
    }
  }

  public getLastReceiver(): GalerkinElement | null {
    return this.lastReceiver;
  }

  public getLastSource(): GalerkinElement | null {
    return this.lastSource;
  }

  public setLastPair(receiver: GalerkinElement, source: GalerkinElement): void {
    this.lastReceiver = receiver;
    this.lastSource = source;
  }

  public getReceiverCubatureRule(): CubatureRule | null {
    return this.receiverCubatureRule;
  }

  public getSourceCubatureRule(): CubatureRule | null {
    return this.sourceCubatureRule;
  }

  public setReceiverCubatureRule(rule: CubatureRule | null): void {
    this.receiverCubatureRule = rule;
  }

  public setSourceCubatureRule(rule: CubatureRule | null): void {
    this.sourceCubatureRule = rule;
  }

  public getReceiverNodes(): Vector3Df[] {
    return this.receiverNodes;
  }

  public getSourceNodes(): Vector3Df[] {
    return this.sourceNodes;
  }

  public getKernel(): number[][] {
    return this.kernel;
  }

  public getMaximumKernelValue(): number {
    return this.maximumKernelValue;
  }

  public setMaximumKernelValue(value: number): void {
    this.maximumKernelValue = value;
  }

  public getVisibilityCount(): number {
    return this.visibilityCount;
  }

  public setVisibilityCount(value: number): void {
    this.visibilityCount = value;
  }

  public getShadowStatistics(): ShadowStatistics {
    return this.shadowStatistics;
  }

  public getKernelRay(): Ray {
    return this.kernelRay;
  }

  public getKernelHitStore(): RayHit {
    return this.kernelHitStore;
  }

  public getKernelDistance(): number[] {
    return this.kernelDistance;
  }

  public getShadowTestDistance(): number[] {
    return this.shadowTestDistance;
  }
}
