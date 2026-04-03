package vsdk.toolkit.io.mgf;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.TokenValidationContext;
import vsdk.toolkit.io.context.VertexContext;

public final class MgfTorusEntityExpander {
    /**
    Expand a torus into cones
    */
    public static int handleEntity(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        StringBuilder p2x = new StringBuilder(24);
        StringBuilder p2y = new StringBuilder(24);
        StringBuilder p2z = new StringBuilder(24);
        StringBuilder radius1 = new StringBuilder(24);
        StringBuilder radius2 = new StringBuilder(24);
        String[] v1Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_tv1",
            "=",
            "_tv2"
        };
        String[] v2Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_tv2",
            "="
        };
        String[] p2Entity = new String[] {
            context.entityNames[EntityTypeContext.MGF_POINT],
            null,
            null,
            null
        };
        String[] coneEntity = new String[] {
            context.entityNames[EntityTypeContext.CONE],
            "_tv1",
            null,
            "_tv2",
            null
        };
        VertexContext vertexContext;
        double averageRadius;
        double theta;

        if (argumentCount != 4) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if ((vertexContext = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1], context)) == null) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        if (vertexContext.n.isNull(Numeric.EPSILON)) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        if (!TokenValidationContext.isFloat(argumentValues[2]) || !TokenValidationContext.isFloat(argumentValues[3])) {
            return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        double minRadius = Double.parseDouble(argumentValues[2]);
        if (minRadius <= Numeric.EPSILON && minRadius >= -Numeric.EPSILON) {
            minRadius = 0.0;
        }
        double maxRadius = Double.parseDouble(argumentValues[3]);

        // Check orientation
        int sign;
        if (minRadius > 0.0) {
            sign = 1;
        } else if (minRadius < 0.0) {
            sign = -1;
        } else {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        if (sign * (maxRadius - minRadius) <= 0.0) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        // Initialize
        context.warpConeEnds = true;
        v2Entity[3] = argumentValues[1];
        MgfTessellationMath.formatFloat(p2x, 24, vertexContext.p.x + 0.5 * sign * (maxRadius - minRadius) * vertexContext.n.x);
        MgfTessellationMath.formatFloat(p2y, 24, vertexContext.p.y + 0.5 * sign * (maxRadius - minRadius) * vertexContext.n.y);
        MgfTessellationMath.formatFloat(p2z, 24, vertexContext.p.z + 0.5 * sign * (maxRadius - minRadius) * vertexContext.n.z);
        p2Entity[1] = p2x.toString();
        p2Entity[2] = p2y.toString();
        p2Entity[3] = p2z.toString();
        int errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v2Entity, context);
        if (errorCode != ParseErrorContext.MGF_OK) {
            return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p2Entity, context);
        if (errorCode != ParseErrorContext.MGF_OK) {
            return errorCode;
        }
        MgfTessellationMath.formatFloat(radius2, 24, averageRadius = 0.5 * (minRadius + maxRadius));

        // Run outer section
        int i;
        for (i = 1; i <= 2 * context.numberOfQuarterCircleDivisions; i++) {
            theta = i * (Math.PI / 2) / context.numberOfQuarterCircleDivisions;
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            MgfTessellationMath.formatFloat(p2x, 24, vertexContext.p.x + 0.5 * sign * (maxRadius - minRadius) * Math.cos(theta) * vertexContext.n.x);
            MgfTessellationMath.formatFloat(p2y, 24, vertexContext.p.y + 0.5 * sign * (maxRadius - minRadius) * Math.cos(theta) * vertexContext.n.y);
            MgfTessellationMath.formatFloat(p2z, 24, vertexContext.p.z + 0.5 * sign * (maxRadius - minRadius) * Math.cos(theta) * vertexContext.n.z);
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v2Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            p2Entity[1] = p2x.toString();
            p2Entity[2] = p2y.toString();
            p2Entity[3] = p2z.toString();
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p2Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            radius1.setLength(0);
            radius1.append(radius2);
            MgfTessellationMath.formatFloat(radius2, 24, averageRadius + 0.5 * (maxRadius - minRadius) * Math.sin(theta));
            coneEntity[2] = radius1.toString();
            coneEntity[4] = radius2.toString();
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.CONE, 5, coneEntity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
        }

        // Run inner section
        MgfTessellationMath.formatFloat(radius2, 24, -0.5 * (minRadius + maxRadius));
        for (; i <= 4 * context.numberOfQuarterCircleDivisions; i++) {
            theta = i * (Math.PI / 2) / context.numberOfQuarterCircleDivisions;
            MgfTessellationMath.formatFloat(p2x, 24, vertexContext.p.x + 0.5 * sign * (maxRadius - minRadius) * Math.cos(theta) * vertexContext.n.x);
            MgfTessellationMath.formatFloat(p2y, 24, vertexContext.p.y + 0.5 * sign * (maxRadius - minRadius) * Math.cos(theta) * vertexContext.n.y);
            MgfTessellationMath.formatFloat(p2z, 24, vertexContext.p.z + 0.5 * sign * (maxRadius - minRadius) * Math.cos(theta) * vertexContext.n.z);
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v2Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            p2Entity[1] = p2x.toString();
            p2Entity[2] = p2y.toString();
            p2Entity[3] = p2z.toString();
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p2Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            radius1.setLength(0);
            radius1.append(radius2);
            MgfTessellationMath.formatFloat(radius2, 24, -averageRadius - .5 * (maxRadius - minRadius) * Math.sin(theta));
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
