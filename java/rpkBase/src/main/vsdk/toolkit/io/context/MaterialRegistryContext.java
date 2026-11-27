package vsdk.toolkit.io.context;

import vsdk.toolkit.common.dataStructures.LookUpBehaviors;
import vsdk.toolkit.common.dataStructures.LookUpTable;

public class MaterialRegistryContext {
    public LookUpTable<MaterialContext> materialLookUpTable;
    public MaterialContext defaultMaterialContext;
    public MaterialContext unNamedMaterialContext;
    public MaterialContext currentMaterialContext;

    public MaterialRegistryContext() {
        materialLookUpTable = new LookUpTable<>(LookUpBehaviors.OWNING);
        defaultMaterialContext = MaterialRegistryContext.createDefaultMgfMaterialContext();
        unNamedMaterialContext = MaterialRegistryContext.createDefaultMgfMaterialContext();
        unNamedMaterialContext.copy(defaultMaterialContext);
        currentMaterialContext = unNamedMaterialContext;
    }

    public void destroy() {
        if (materialLookUpTable != null) {
            materialLookUpTable.lookUpDone();
            materialLookUpTable = null;
        }
        currentMaterialContext = null;
    }

    public void reset() {
        unNamedMaterialContext.copy(defaultMaterialContext);
        currentMaterialContext = unNamedMaterialContext;
        materialLookUpTable.lookUpDone();
    }

    private static MaterialContext createDefaultMgfMaterialContext() {
        MaterialContext materialContext = new MaterialContext();
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
