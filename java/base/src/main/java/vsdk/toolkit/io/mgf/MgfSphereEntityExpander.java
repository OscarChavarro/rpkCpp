package vsdk.toolkit.io.mgf;

import vsdk.toolkit.io.context.ColorContext;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.TokenValidationContext;
import vsdk.toolkit.io.context.VertexContext;

public final class MgfSphereEntityExpander {
    /**
    Expand a sphere into cones
    */
    public static int handleEntity(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        StringBuilder p2x = new StringBuilder(24);
        StringBuilder p2y = new StringBuilder(24);
        StringBuilder p2z = new StringBuilder(24);
        StringBuilder radius1 = new StringBuilder(24);
        StringBuilder radius2 = new StringBuilder(24);

        String[] v1Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_sv1",
            "=",
            "_sv2"
        };
        String[] v2Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_sv2",
            "="
        };
        String[] p2Entity = new String[5];
        p2Entity[0] = context.entityNames[EntityTypeContext.MGF_POINT];
        String[] coneEntity = new String[6];
        coneEntity[0] = context.entityNames[EntityTypeContext.CONE];
        coneEntity[1] = "_sv1";
        coneEntity[3] = "_sv2";

        if (argumentCount != 3) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        VertexContext vertexContext = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1], context);
        if (vertexContext == null) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        if (!TokenValidationContext.isFloat(argumentValues[2])) {
            return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        double radius = Double.parseDouble(argumentValues[2]);

        // Initialize
        context.warpConeEnds = true;
        int errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, v2Entity, context);
        if (errorCode != ParseErrorContext.MGF_OK) {
            return errorCode;
        }
        MgfTessellationMath.formatFloat(p2x, 24, vertexContext.p.x);
        MgfTessellationMath.formatFloat(p2y, 24, vertexContext.p.y);
        MgfTessellationMath.formatFloat(p2z, 24, vertexContext.p.z + radius);
        p2Entity[1] = p2x.toString();
        p2Entity[2] = p2y.toString();
        p2Entity[3] = p2z.toString();
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p2Entity, context);
        if (errorCode != ParseErrorContext.MGF_OK) {
            return errorCode;
        }
        radius2.setLength(0);
        radius2.append('0');

        for (int i = 1; i <= 2 * context.numberOfQuarterCircleDivisions; i++) {
            double theta = i * (Math.PI / 2) / context.numberOfQuarterCircleDivisions;
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            MgfTessellationMath.formatFloat(p2z, 24, vertexContext.p.z + radius * Math.cos(theta));
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v2Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            p2Entity[3] = p2z.toString();
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p2Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            radius1.setLength(0);
            radius1.append(radius2);
            MgfTessellationMath.formatFloat(radius2, 24, radius * Math.sin(theta));
            coneEntity[2] = radius1.toString();
            coneEntity[4] = radius2.toString();
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.CONE, 5, coneEntity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
        }
        context.warpConeEnds = false;
        return ParseErrorContext.MGF_OK;
    }
}
