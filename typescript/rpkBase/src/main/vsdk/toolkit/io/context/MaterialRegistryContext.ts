import { LookUpBehaviors } from "../../common/dataStructures/LookUpBehaviors";
import { LookUpTable } from "../../common/dataStructures/LookUpTable";
import { ColorContext } from "./ColorContext";
import { MaterialContext } from "./MaterialContext";

export class MaterialRegistryContext {
  public materialLookUpTable: LookUpTable<MaterialContext> | null;
  public defaultMaterialContext: MaterialContext;
  public unNamedMaterialContext: MaterialContext;
  public currentMaterialContext: MaterialContext | null;

  public constructor() {
    this.materialLookUpTable = new LookUpTable<MaterialContext>(LookUpBehaviors.OWNING);
    this.defaultMaterialContext = MaterialRegistryContext.createDefaultMgfMaterialContext();
    this.unNamedMaterialContext = MaterialRegistryContext.createDefaultMgfMaterialContext();
    this.unNamedMaterialContext.copy(this.defaultMaterialContext);
    this.currentMaterialContext = this.unNamedMaterialContext;
  }

  public destroy(): void {
    if (this.materialLookUpTable !== null) {
      this.materialLookUpTable.lookUpDone();
      this.materialLookUpTable = null;
    }
    this.currentMaterialContext = null;
  }

  public reset(): void {
    this.unNamedMaterialContext.copy(this.defaultMaterialContext);
    this.currentMaterialContext = this.unNamedMaterialContext;
    (this.materialLookUpTable as LookUpTable<MaterialContext>).lookUpDone();
  }

  private static createDefaultMgfMaterialContext(): MaterialContext {
    const materialContext = new MaterialContext();
    materialContext.clock = 1;
    materialContext.sided = false;
    materialContext.nr = 1.0;
    materialContext.ni = 0.0;
    materialContext.rd = 0.0;
    materialContext.rd_c.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
    materialContext.td = 0.0;
    materialContext.td_c.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
    materialContext.ed = 0.0;
    materialContext.ed_c.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
    materialContext.rs = 0.0;
    materialContext.rs_c.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
    materialContext.rs_a = 0.0;
    materialContext.ts = 0.0;
    materialContext.ts_c.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
    materialContext.ts_a = 0.0;
    return materialContext;
  }
}
