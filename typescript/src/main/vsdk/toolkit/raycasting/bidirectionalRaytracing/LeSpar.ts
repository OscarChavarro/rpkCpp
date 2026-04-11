import { RadianceMethod } from "../../scene/RadianceMethod";
import { Spar } from "./Spar";
import { SparConfig } from "./SparConfig";
import { SparPathGroup } from "./SparPathGroup";

export class LeSpar extends Spar {
  public override init(sparConfig: SparConfig, radianceMethod: RadianceMethod | null): void {
    super.init(sparConfig, radianceMethod);

    if (sparConfig.baseConfig !== null && sparConfig.baseConfig.doLe !== 0) {
      this.parseAndInit(SparPathGroup.DISJOINT_GROUP, sparConfig.baseConfig.leRegExp);
    }

    if (sparConfig.baseConfig !== null && sparConfig.baseConfig.doWeighted !== 0) {
      this.parseAndInit(SparPathGroup.LD_GROUP, sparConfig.baseConfig.wleRegExp);
      this.m_sparList[SparPathGroup.LD_GROUP].add(sparConfig.ldSpar as Spar);
    }
  }
}
