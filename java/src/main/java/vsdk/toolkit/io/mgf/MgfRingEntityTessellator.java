package vsdk.toolkit.io.mgf;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.TokenValidationContext;
import vsdk.toolkit.io.context.VertexContext;

public final class MgfRingEntityTessellator {
    /**
    Turn a ring into polygons
    */
    public static int handleEntity(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        StringBuilder p3x = new StringBuilder(24);
        StringBuilder p3y = new StringBuilder(24);
        StringBuilder p3z = new StringBuilder(24);
        StringBuilder p4x = new StringBuilder(24);
        StringBuilder p4y = new StringBuilder(24);
        StringBuilder p4z = new StringBuilder(24);
        String[] namesEntity = new String[] {
            context.entityNames[EntityTypeContext.MGF_NORMAL],
            "0",
            "0",
            "0"
        };
        String[] v1Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_rv1",
            "=",
            null
        };
        String[] v2Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_rv2",
            "=",
            "_rv3"
        };
        String[] v3Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_rv3",
            "="
        };
        String[] p3Entity = new String[] {
            context.entityNames[EntityTypeContext.MGF_POINT],
            null,
            null,
            null
        };
        String[] v4Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_rv4",
            "="
        };
        String[] p4Entity = new String[] {
            context.entityNames[EntityTypeContext.MGF_POINT],
            null,
            null,
            null
        };
        String[] faceEntity = new String[] {
            context.entityNames[EntityTypeContext.FACE],
            "_rv1",
            "_rv2",
            "_rv3",
            "_rv4"
        };
        double theta;

        if (argumentCount != 4) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }

        VertexContext vertexContext = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1], context);
        if (vertexContext == null) {
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
        if (minRadius < 0.0 || maxRadius <= minRadius) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        // Initialize
        Vector3Dd u = new Vector3Dd();
        Vector3Dd v = new Vector3Dd();

        MgfTessellationMath.mgfMakeAxes(u, v, vertexContext.n, Numeric.EPSILON);
        MgfTessellationMath.formatFloat(p3x, 24, vertexContext.p.x + maxRadius * u.x);
        MgfTessellationMath.formatFloat(p3y, 24, vertexContext.p.y + maxRadius * u.y);
        MgfTessellationMath.formatFloat(p3z, 24, vertexContext.p.z + maxRadius * u.z);
        p3Entity[1] = p3x.toString();
        p3Entity[2] = p3y.toString();
        p3Entity[3] = p3z.toString();
        int errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, v3Entity, context);
        if (errorCode != ParseErrorContext.MGF_OK) {
            return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity, context);
        if (errorCode != ParseErrorContext.MGF_OK) {
            return errorCode;
        }

        if (Numeric.doubleEqual(minRadius, 0.0, Numeric.EPSILON)) {
            // Closed
            v1Entity[3] = argumentValues[1];
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, namesEntity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            for (int i = 1; i <= 4 * context.numberOfQuarterCircleDivisions; i++) {
                theta = i * (Math.PI / 2) / context.numberOfQuarterCircleDivisions;
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v2Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }

                MgfTessellationMath.formatFloat(
                    p3x, 24,
                    vertexContext.p.x + maxRadius * u.x * Math.cos(theta) + maxRadius * v.x * Math.sin(theta));
                MgfTessellationMath.formatFloat(
                    p3y, 24,
                    vertexContext.p.y + maxRadius * u.y * Math.cos(theta) + maxRadius * v.y * Math.sin(theta));
                MgfTessellationMath.formatFloat(
                    p3z, 24,
                    vertexContext.p.z + maxRadius * u.z * Math.cos(theta) + maxRadius * v.z * Math.sin(theta));

                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v3Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                p3Entity[1] = p3x.toString();
                p3Entity[2] = p3y.toString();
                p3Entity[3] = p3z.toString();
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, 4, faceEntity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
            }
        }
        else {
            // Open
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, v4Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }

            MgfTessellationMath.formatFloat(p4x, 24, vertexContext.p.x + minRadius * u.x);
            MgfTessellationMath.formatFloat(p4y, 24, vertexContext.p.y + minRadius * u.y);
            MgfTessellationMath.formatFloat(p4z, 24, vertexContext.p.z + minRadius * u.z);
            p4Entity[1] = p4x.toString();
            p4Entity[2] = p4y.toString();
            p4Entity[3] = p4z.toString();

            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p4Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            v1Entity[3] = "_rv4";
            for (int i = 1; i <= 4 * context.numberOfQuarterCircleDivisions; i++) {
                theta = i * (Math.PI / 2) / context.numberOfQuarterCircleDivisions;
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v2Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }

                double delta = u.x * Math.cos(theta) + v.x * Math.sin(theta);
                MgfTessellationMath.formatFloat(p3x, 24, vertexContext.p.x + maxRadius * delta);
                MgfTessellationMath.formatFloat(p4x, 24, vertexContext.p.x + minRadius * delta);

                delta = u.y * Math.cos(theta) + v.y * Math.sin(theta);
                MgfTessellationMath.formatFloat(p3y, 24, vertexContext.p.y + maxRadius * delta);
                MgfTessellationMath.formatFloat(p4y, 24, vertexContext.p.y + minRadius * delta);

                delta = u.z * Math.cos(theta) + v.z * Math.sin(theta);
                MgfTessellationMath.formatFloat(p3z, 24, vertexContext.p.z + maxRadius * delta);
                MgfTessellationMath.formatFloat(p4z, 24, vertexContext.p.z + minRadius * delta);

                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v3Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                p3Entity[1] = p3x.toString();
                p3Entity[2] = p3y.toString();
                p3Entity[3] = p3z.toString();
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v4Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                p4Entity[1] = p4x.toString();
                p4Entity[2] = p4y.toString();
                p4Entity[3] = p4z.toString();
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p4Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, 5, faceEntity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
            }
        }
        return ParseErrorContext.MGF_OK;
    }
}
