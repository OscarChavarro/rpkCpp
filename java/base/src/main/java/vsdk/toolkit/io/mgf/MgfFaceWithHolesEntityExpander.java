package vsdk.toolkit.io.mgf;

import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.ReaderContext;

public final class MgfFaceWithHolesEntityExpander {
    /**
    Replace face + holes with single contour
    */
    public static int handleEntity(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        String[] newArgumentValues = new String[ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT];
        int lastPerimeterIndex = 0;

        newArgumentValues[0] = context.entityNames[EntityTypeContext.FACE];
        int i;
        for (i = 1; i < argumentCount; i++) {
            if (argumentValues[i].charAt(0) == '-') {
                if (i < 4) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (i >= argumentCount - 1) {
                    break;
                }
                if (lastPerimeterIndex == 0) {
                    lastPerimeterIndex = i - 1;
                }
                int j;
                for (j = i + 1; j < argumentCount - 1 && argumentValues[j + 1].charAt(0) != '-'; j++) {
                }
                if (j - i < 3) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                newArgumentValues[i] = argumentValues[j]; // Connect hole loop
            }
            else {
                // Hole or perimeter vertex
                newArgumentValues[i] = argumentValues[i];
            }
        }
        if (lastPerimeterIndex != 0) {
            // Finish seam to outside
            newArgumentValues[i++] = argumentValues[lastPerimeterIndex];
        }
        return MgfEntityControl.mgfHandle(EntityTypeContext.FACE, i, newArgumentValues, context);
    }
}
