import { ColorRgb } from "../../common/color/ColorRgb";
import { Error as VsdkError } from "../../common/Error";
import { BsdfComponent } from "../../material/BsdfComponent";
import { BiPath } from "./BiPath";
import { FlagChain } from "./FlagChain";
import { FlagChainList } from "./FlagChainList";

export class ContribHandler {
  private static readonly BSDF_ALL_COMPONENTS =
    BsdfComponent.BRDF_DIFFUSE_COMPONENT
    | BsdfComponent.BRDF_GLOSSY_COMPONENT
    | BsdfComponent.BRDF_SPECULAR_COMPONENT
    | BsdfComponent.BTDF_DIFFUSE_COMPONENT
    | BsdfComponent.BTDF_GLOSSY_COMPONENT
    | BsdfComponent.BTDF_SPECULAR_COMPONENT;

  public array: FlagChainList[] | null;
  public maxLength: number;

  public constructor() {
    this.array = null;
    this.maxLength = 0;
  }

  public init(paramMaxLength: number): void {
    this.maxLength = paramMaxLength;

    this.array = new Array<FlagChainList>(paramMaxLength + 1);
    for (let i = 0; i <= paramMaxLength; i++) {
      this.array[i] = new FlagChainList();
    }
  }

  public compute(path: BiPath): ColorRgb {
    const result = new ColorRgb();
    result.clear();

    const length = path.m_eyeSize + path.m_lightSize;

    if (length > this.maxLength) {
      VsdkError.error("CContribHandler::Compute", "Path too long !!");
      return result;
    }

    return (this.array as FlagChainList[])[length].compute(path);
  }

  public addRegExp(regExp: string | null): void {
    if (regExp === null || regExp.length === 0) {
      return;
    }

    if (regExp.charAt(0) === "-") {
      this.doRegExp(regExp.substring(1), true);
    }
    else {
      this.doRegExp(regExp, false);
    }
  }

  protected doRegExp(regExp: string, subtract: boolean): void {
    this.doRegExpGeneral(regExp, subtract);
  }

  protected doSyntaxError(errString: string): void {
    VsdkError.error("Flag chain Syntax Error", errString);
    this.init(this.maxLength);
  }

  protected getFlags(regExp: string, pos: number[], flags: number[]): boolean {
    let p = pos[0];

    flags[0] = 0;

    if (p >= regExp.length || regExp.charAt(p++) !== "(") {
      this.doSyntaxError("getFlags: '(' expected");
      return false;
    }

    while (true) {
      if (p >= regExp.length) {
        this.doSyntaxError("getFlags: ')' expected");
        return false;
      }

      const c = regExp.charAt(p++);
      if (c === ")") {
        break;
      }

      switch (c) {
        case "S":
          if (p < regExp.length && regExp.charAt(p) === "T") {
            p++;
            flags[0] |= BsdfComponent.BTDF_SPECULAR_COMPONENT;
          }
          else if (p < regExp.length && regExp.charAt(p) === "R") {
            p++;
            flags[0] |= BsdfComponent.BRDF_SPECULAR_COMPONENT;
          }
          else {
            flags[0] |= BsdfComponent.BTDF_SPECULAR_COMPONENT | BsdfComponent.BRDF_SPECULAR_COMPONENT;
          }
          break;
        case "G":
          if (p < regExp.length && regExp.charAt(p) === "T") {
            p++;
            flags[0] |= BsdfComponent.BTDF_GLOSSY_COMPONENT;
          }
          else if (p < regExp.length && regExp.charAt(p) === "R") {
            p++;
            flags[0] |= BsdfComponent.BRDF_GLOSSY_COMPONENT;
          }
          else {
            flags[0] |= BsdfComponent.BTDF_GLOSSY_COMPONENT | BsdfComponent.BRDF_GLOSSY_COMPONENT;
          }
          break;
        case "D":
          if (p < regExp.length && regExp.charAt(p) === "T") {
            p++;
            flags[0] |= BsdfComponent.BTDF_DIFFUSE_COMPONENT;
          }
          else if (p < regExp.length && regExp.charAt(p) === "R") {
            p++;
            flags[0] |= BsdfComponent.BRDF_DIFFUSE_COMPONENT;
          }
          else {
            flags[0] |= BsdfComponent.BTDF_DIFFUSE_COMPONENT | BsdfComponent.BRDF_DIFFUSE_COMPONENT;
          }
          break;
        case "X":
          if (p < regExp.length && regExp.charAt(p) === "T") {
            p++;
            flags[0] |= BsdfComponent.BTDF_DIFFUSE_COMPONENT | BsdfComponent.BTDF_GLOSSY_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT;
          }
          else if (p < regExp.length && regExp.charAt(p) === "R") {
            p++;
            flags[0] |= BsdfComponent.BRDF_DIFFUSE_COMPONENT | BsdfComponent.BRDF_GLOSSY_COMPONENT | BsdfComponent.BRDF_SPECULAR_COMPONENT;
          }
          else {
            flags[0] |= ContribHandler.BSDF_ALL_COMPONENTS;
          }
          break;
        case "L":
          if (p >= regExp.length || regExp.charAt(p) !== "X") {
            this.doSyntaxError("getFlags: No 'X' after 'L'. Only LX supported");
            return false;
          }
          p++;
          flags[0] = ContribHandler.BSDF_ALL_COMPONENTS;
          break;
        case "E":
          if (p >= regExp.length || regExp.charAt(p) !== "X") {
            this.doSyntaxError("getFlags: No 'X' after 'E'. Only EX supported");
            return false;
          }
          p++;
          flags[0] = ContribHandler.BSDF_ALL_COMPONENTS;
          break;
        case "|":
          break;
        default:
          this.doSyntaxError("getFlags: Unexpected character in token");
          return false;
      }
    }

    pos[0] = p;
    return true;
  }

  protected getToken(regExp: string, pos: number[], token: string[], flags: number[]): boolean {
    if (pos[0] >= regExp.length) {
      return false;
    }

    switch (regExp.charAt(pos[0])) {
      case "\0":
        return false;
      case "+":
        token[0] = "+";
        pos[0]++;
        break;
      case "*":
        token[0] = "*";
        pos[0]++;
        break;
      case "(":
        token[0] = "F";
        return this.getFlags(regExp, pos, flags);
      default:
        this.doSyntaxError("Unknown token");
        return false;
    }

    return true;
  }

  protected doRegExpGeneral(regExp: string, subtract: boolean): void {
    const c = new FlagChain();

    const MAX_REGEXP_ITEMS = 15;

    const flagArray = new Array<number>(MAX_REGEXP_ITEMS).fill(0);
    const typeArray = new Array<string>(MAX_REGEXP_ITEMS).fill(" ");
    const countArray = new Array<number>(MAX_REGEXP_ITEMS).fill(0);
    const pos = [0];
    let tokenCount = -1;
    let iteratorCount = 0;
    const token = [""];
    const data = [0];

    while (this.getToken(regExp, pos, token, data)) {
      if (token[0] === "F") {
        if (tokenCount === MAX_REGEXP_ITEMS - 1) {
          this.doSyntaxError("Too many tokens in regexp");
          return;
        }

        tokenCount++;
        flagArray[tokenCount] = data[0];
        typeArray[tokenCount] = " ";
        countArray[tokenCount] = 0;
      }
      else {
        if (tokenCount === -1) {
          this.doSyntaxError("Initial iteration token");
          return;
        }

        if (token[0] === "+") {
          if (tokenCount === MAX_REGEXP_ITEMS - 1) {
            this.doSyntaxError("Too many tokens in regexp");
            return;
          }

          flagArray[tokenCount + 1] = flagArray[tokenCount];
          tokenCount++;
          token[0] = "*";
        }

        typeArray[tokenCount] = token[0];
        countArray[tokenCount] = 0;

        if (token[0] === "*" || token[0] === "+") {
          iteratorCount++;
        }
      }
    }

    if (tokenCount === -1) {
      this.doSyntaxError("No tokens in regexp");
      return;
    }

    tokenCount++;
    typeArray[tokenCount] = "\0";

    const beginLength = tokenCount - iteratorCount;
    let endLength = this.maxLength;

    if (iteratorCount === 0) {
      endLength = beginLength;
    }

    for (let length = beginLength; length <= endLength; length++) {
      const tmpList = new FlagChainList();
      c.init(length, subtract);

      const maxIteration = length - tokenCount + iteratorCount;

      let done = false;

      let iterationsDone = 0;
      let nextIterationsDone = 0;

      while (!done) {
        let iteratorsFound = 0;
        let remember = 0;
        pos[0] = 0;

        for (let i = 0; i < tokenCount; i++) {
          if (typeArray[i] === " ") {
            if (c.chain !== null) {
              c.chain[pos[0]] = flagArray[i];
            }
            pos[0]++;
          }
          else {
            iteratorsFound++;
            let num: number;
            if (iteratorsFound === iteratorCount) {
              num = maxIteration - iterationsDone;
              if (iteratorCount === 1 || remember !== 0) {
                done = true;
              }
            }
            else {
              num = countArray[i];
              if (iteratorsFound === 1) {
                countArray[i]++;
                nextIterationsDone++;
                if (nextIterationsDone > maxIteration) {
                  nextIterationsDone -= countArray[i];
                  countArray[i] = 0;
                  remember = 1;
                }
                else {
                  remember = 0;
                }
              }
              else if (remember !== 0) {
                countArray[i]++;
                nextIterationsDone++;
                if (nextIterationsDone > maxIteration) {
                  nextIterationsDone -= countArray[i];
                  countArray[i] = 0;
                  remember = 1;
                }
                else {
                  remember = 0;
                }
              }
            }

            for (let j = 0; j < num; j++) {
              if (c.chain !== null) {
                c.chain[pos[0]] = flagArray[i];
              }
              pos[0]++;
            }
          }
        }

        iterationsDone = nextIterationsDone;
        tmpList.addDisjoint(c);

        if (iteratorCount === 0) {
          done = true;
        }
      }

      (this.array as FlagChainList[])[length].add(tmpList.simplify());
    }
  }
}
