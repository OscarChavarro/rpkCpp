import { XxdfComponentFlagInfo } from "./XxdfComponentFlag";

export class BsdfComponentFlag {
  private constructor() {
  }

  public static bsdfIndexToComp(index: number): number {
    return 1 << index;
  }

  public static getBrdfFlags(bsflags: number): number {
    return bsflags & XxdfComponentFlagInfo.ALL_COMPONENTS;
  }

  public static getBtdfFlags(bsflags: number): number {
    return (bsflags >> XxdfComponentFlagInfo.XXDF_COMPONENTS) & XxdfComponentFlagInfo.ALL_COMPONENTS;
  }
}
