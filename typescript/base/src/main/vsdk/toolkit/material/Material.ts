import { PhongBidirectionalScatteringDistributionFunction } from "./PhongBidirectionalScatteringDistributionFunction";
import { PhongEmittanceDistributionFunction } from "./PhongEmittanceDistributionFunction";

export class Material {
  private readonly edf: PhongEmittanceDistributionFunction;
  private readonly bsdf: PhongBidirectionalScatteringDistributionFunction;
  private readonly sided: boolean;
  private readonly name: string;

  public constructor(
    inName: string | null,
    inEdf: PhongEmittanceDistributionFunction,
    inBsdf: PhongBidirectionalScatteringDistributionFunction,
    inSided: boolean
  ) {
    this.name = inName ?? "";
    this.sided = inSided;
    this.edf = inEdf;
    this.bsdf = inBsdf;
  }

  public getEdf(): PhongEmittanceDistributionFunction {
    return this.edf;
  }

  public getBsdf(): PhongBidirectionalScatteringDistributionFunction {
    return this.bsdf;
  }

  public isSided(): boolean {
    return this.sided;
  }

  public getName(): string {
    return this.name;
  }
}
