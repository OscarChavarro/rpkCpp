import { ColorRgb } from "../../common/color/ColorRgb";
import { Error as VsdkError } from "../../common/Error";
import { BiPath } from "./BiPath";

export class FlagChain {
  public chain: number[] | null;
  public length: number;
  public subtract: boolean;

  public constructor();
  public constructor(paramLength: number, paramSubtract: boolean);
  public constructor(paramLength: number);
  public constructor(copy: FlagChain);
  public constructor(paramOrCopy?: number | FlagChain, paramSubtract?: boolean) {
    this.chain = null;
    this.length = 0;
    this.subtract = false;

    if (paramOrCopy instanceof FlagChain) {
      const c = paramOrCopy;
      this.init(c.length, c.subtract);
      if (c.chain !== null && this.chain !== null) {
        const dst = this.chain as number[];
        const src = c.chain as number[];
        for (let i = 0; i < this.length; i++) {
          dst[i] = src[i];
        }
      }
    }
    else if (typeof paramOrCopy === "number") {
      this.init(paramOrCopy, paramSubtract ?? false);
    }
    else {
      this.init(0, false);
    }
  }

  public init(inLength: number, inSubtract = false): void {
    this.length = inLength;
    this.subtract = inSubtract;

    if (inLength > 0) {
      this.chain = new Array<number>(inLength);
      for (let i = 0; i < inLength; i++) {
        this.chain[i] = 0;
      }
    }
    else {
      this.chain = null;
    }
  }

  public static compare(c1: FlagChain | null, c2: FlagChain | null): boolean {
    let nrDifferent = 0;

    if (c1 === null || c2 === null) {
      return false;
    }

    if (c1.length !== c2.length || c1.subtract !== c2.subtract) {
      return false;
    }

    for (let i = 0; i < c1.length && nrDifferent === 0; i++) {
      if ((c1.chain as number[])[i] !== (c2.chain as number[])[i]) {
        nrDifferent++;
      }
    }

    if (nrDifferent === 0) {
      return true;
    }

    return false;
  }

  public static combine(c1: FlagChain | null, c2: FlagChain | null): FlagChain | null {
    let nrDifferent = 0;
    let diffIndex = 0;

    if (c1 === null || c2 === null) {
      return null;
    }

    if (c1.length !== c2.length || c1.subtract !== c2.subtract) {
      return null;
    }

    for (let i = 0; i < c1.length && nrDifferent <= 1; i++) {
      if ((c1.chain as number[])[i] !== (c2.chain as number[])[i]) {
        nrDifferent++;
        diffIndex = i;
      }
    }

    if (nrDifferent === 0) {
      return new FlagChain(c1);
    }

    if (nrDifferent === 1) {
      const newFlagChain = new FlagChain(c1);
      (newFlagChain.chain as number[])[diffIndex] = (c1.chain as number[])[diffIndex] | (c2.chain as number[])[diffIndex];
      return newFlagChain;
    }

    return null;
  }

  public compute(path: BiPath): ColorRgb {
    const result = new ColorRgb();
    result.setMonochrome(1.0);
    const eyeSize = path.m_eyeSize;
    const lightSize = path.m_lightSize;

    if (lightSize + eyeSize !== this.length) {
      VsdkError.error("FlagChain::Compute", "Wrong path length");
      return result;
    }

    let node = path.m_lightPath;

    for (let i = 0; i < lightSize; i++) {
      const tmpCol = node!.m_bsdfComp.Sum((this.chain as number[])[i]);
      result.selfScalarProduct(tmpCol);
      node = node!.next();
    }

    node = path.m_eyePath;

    for (let i = 0; i < eyeSize; i++) {
      const tmpCol = node!.m_bsdfComp.Sum((this.chain as number[])[this.length - 1 - i]);
      result.selfScalarProduct(tmpCol);
      node = node!.next();
    }

    if (this.subtract) {
      result.scale(-1.0);
    }

    return result;
  }
}
