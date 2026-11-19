package vsdk.toolkit.io.mgf;

import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;

public final class MgfCylinderEntityExpander {
    /**
    Replace a cylinder with equivalent cone
    */
    public static int handleEntity(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        String[] newArgumentValues = new String[6];
        newArgumentValues[0] = context.entityNames[EntityTypeContext.CONE];

        if (argumentCount != 4) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        newArgumentValues[1] = argumentValues[1];
        newArgumentValues[2] = argumentValues[2];
        newArgumentValues[3] = argumentValues[3];
        newArgumentValues[4] = argumentValues[2];
        return MgfEntityControl.mgfHandle(EntityTypeContext.CONE, 5, newArgumentValues, context);
    }
}
