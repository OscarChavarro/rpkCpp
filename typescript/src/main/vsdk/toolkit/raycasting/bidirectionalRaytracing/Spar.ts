import { ColorRgb } from "../../common/color/ColorRgb";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { BiPath } from "./BiPath";
import { ContribHandler } from "./ContribHandler";
import { SparConfig } from "./SparConfig";
import { SparList } from "./SparList";
import { SparPathGroupInfo } from "./SparPathGroupInfo";

export class Spar {
  public m_contrib: ContribHandler[];
  public m_sparList: SparList[];

  public constructor() {
    this.m_contrib = new Array<ContribHandler>(SparPathGroupInfo.MAX_PATH_GROUPS);
    this.m_sparList = new Array<SparList>(SparPathGroupInfo.MAX_PATH_GROUPS);

    for (let i = 0; i < SparPathGroupInfo.MAX_PATH_GROUPS; i++) {
      this.m_contrib[i] = new ContribHandler();
      this.m_sparList[i] = new SparList();
    }
  }

  public init(config: SparConfig, radianceMethod: RadianceMethod | null): void {
    void radianceMethod;
    for (let i = 0; i < SparPathGroupInfo.MAX_PATH_GROUPS; i++) {
      this.m_contrib[i].init(config.baseConfig!.maximumPathDepth);
      this.m_sparList[i].removeAll();
    }
  }

  public parseAndInit(group: number, regExp: string | null): void {
    if (regExp === null) {
      return;
    }

    let beginPos = 0;
    let endPos = 0;

    while (endPos < regExp.length) {
      if (regExp.charAt(endPos) === ",") {
        this.m_contrib[group].addRegExp(regExp.substring(beginPos, endPos));
        beginPos = endPos + 1;
      }

      endPos++;
    }

    if (beginPos !== endPos) {
      this.m_contrib[group].addRegExp(regExp.substring(beginPos, endPos));
    }
  }

  public handlePath(config: SparConfig, path: BiPath): ColorRgb {
    void config;
    void path;
    const result = new ColorRgb();
    result.clear();
    return result;
  }
}
