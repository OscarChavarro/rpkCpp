package vsdk.toolkit.io.context;

import vsdk.toolkit.common.dataStructures.LookUpBehaviors;
import vsdk.toolkit.common.dataStructures.LookUpTable;

public class ColorRegistryContext {
    public LookUpTable<ColorContext> colorLookUpTable;
    public ColorContext unNamedColorContext;
    public ColorContext currentColor;

    public ColorRegistryContext() {
        colorLookUpTable = new LookUpTable<>(LookUpBehaviors.OWNING);
        unNamedColorContext = new ColorContext();
        currentColor = unNamedColorContext;
        unNamedColorContext.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
    }

    public void destroy() {
        unNamedColorContext = null;
        if (colorLookUpTable != null) {
            colorLookUpTable.lookUpDone();
            colorLookUpTable = null;
        }
        currentColor = null;
    }

    public void reset() {
        unNamedColorContext.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
        currentColor = unNamedColorContext;
        colorLookUpTable.lookUpDone();
    }
}
