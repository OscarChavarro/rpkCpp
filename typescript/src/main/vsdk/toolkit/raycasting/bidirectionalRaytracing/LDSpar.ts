import { Error as VsdkError } from "../../common/Error";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { Spar } from "./Spar";
import { SparConfig } from "./SparConfig";
import { SparPathGroup } from "./SparPathGroup";

export class LDSpar extends Spar {
  public override init(sparConfig: SparConfig, radianceMethod: RadianceMethod | null): void {
    super.init(sparConfig, radianceMethod);

    if (sparConfig.baseConfig === null || !(sparConfig.baseConfig.doLD !== 0 || sparConfig.baseConfig.doWeighted !== 0)) {
      return;
    }

    if (radianceMethod === null) {
      VsdkError.error("CLDSpar::mainInitApplication", "Galerkin Radiance method not active !");
    }

    if (sparConfig.baseConfig.doLD !== 0) {
      this.parseAndInit(SparPathGroup.DISJOINT_GROUP, sparConfig.baseConfig.ldRegExp);
    }

    if (sparConfig.baseConfig.doWeighted !== 0) {
      this.parseAndInit(SparPathGroup.LD_GROUP, sparConfig.baseConfig.wldRegExp);
      this.m_sparList[SparPathGroup.LD_GROUP].add(sparConfig.leSpar as Spar);
    }
  }
}
