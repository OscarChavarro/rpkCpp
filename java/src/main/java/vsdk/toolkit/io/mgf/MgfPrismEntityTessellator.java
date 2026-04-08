package vsdk.toolkit.io.mgf;

import java.util.Locale;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.ReaderContext;
import vsdk.toolkit.io.context.TokenValidationContext;
import vsdk.toolkit.io.context.VertexContext;

public final class MgfPrismEntityTessellator {
    /**
    Turn a prism into polygons
    */
    public static int handleEntity(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        StringBuilder px = new StringBuilder(24);
        StringBuilder py = new StringBuilder(24);
        StringBuilder pz = new StringBuilder(24);
        String[] vertexEntity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            null,
            "=",
            null
        };
        String[] pointEntity = new String[] {
            context.entityNames[EntityTypeContext.MGF_POINT],
            null,
            null,
            null
        };
        String[] zeroNormal = new String[] {
            context.entityNames[EntityTypeContext.MGF_NORMAL],
            "0",
            "0",
            "0"
        };
        String[] newArgumentValues = new String[ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT];
        String[] newVertexNames = new String[ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT - 1];
        VertexContext vertexContext;
        int errorCode;
        int i;

        // Check arguments
        if (argumentCount < 5) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isFloat(argumentValues[argumentCount - 1])) {
            return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        double length = Double.parseDouble(argumentValues[argumentCount - 1]);
        if (length <= Numeric.EPSILON && length >= -Numeric.EPSILON) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        // Compute face normal
        VertexContext v0Context = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1], context);
        if (v0Context == null) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        int hasNormal = 0;

        Vector3Dd normal = new Vector3Dd(0.0, 0.0, 0.0);
        Vector3Dd v1 = new Vector3Dd(0.0, 0.0, 0.0);

        for (i = 2; i < argumentCount - 1; i++) {
            vertexContext = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[i], context);
            if (vertexContext == null) {
                return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
            }

            if (!vertexContext.n.isNull(Numeric.EPSILON)) {
                hasNormal++;
            }

            Vector3Dd v2 = new Vector3Dd();
            Vector3Dd v3 = new Vector3Dd();

            v2.x = vertexContext.p.x - v0Context.p.x;
            v2.y = vertexContext.p.y - v0Context.p.y;
            v2.z = vertexContext.p.z - v0Context.p.z;
            v3.crossProduct(v1, v2);
            normal.x += v3.x;
            normal.y += v3.y;
            normal.z += v3.z;
            v1.copy(v2);
        }
        if (normal.normalizeAndGivePreviousNorm(Numeric.EPSILON) == 0.0) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        // Create moved vertices
        for (i = 1; i < argumentCount - 1; i++) {
            newVertexNames[i - 1] = String.format(Locale.US, "_pv%d", i);
            vertexEntity[1] = newVertexNames[i - 1];
            vertexEntity[3] = argumentValues[i];
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, vertexEntity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            vertexContext = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[i], context); // Checked above
            MgfTessellationMath.formatFloat(px, 24, vertexContext.p.x - length * normal.x);
            MgfTessellationMath.formatFloat(py, 24, vertexContext.p.y - length * normal.y);
            MgfTessellationMath.formatFloat(pz, 24, vertexContext.p.z - length * normal.z);
            pointEntity[1] = px.toString();
            pointEntity[2] = py.toString();
            pointEntity[3] = pz.toString();
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, pointEntity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
        }

        // Make faces
        newArgumentValues[0] = context.entityNames[EntityTypeContext.FACE];
        // Do the side faces
        newArgumentValues[5] = null;
        newArgumentValues[3] = argumentValues[argumentCount - 2];
        newArgumentValues[4] = newVertexNames[argumentCount - 3];
        for (i = 1; i < argumentCount - 1; i++) {
            newArgumentValues[1] = newVertexNames[i - 1];
            newArgumentValues[2] = argumentValues[i];
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, 5, newArgumentValues, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            newArgumentValues[3] = newArgumentValues[2];
            newArgumentValues[4] = newArgumentValues[1];
        }

        // Do top face
        for (i = 1; i < argumentCount - 1; i++) {
            if (hasNormal != 0) {
                // Zero normals
                vertexEntity[1] = newVertexNames[i - 1];
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, vertexEntity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, zeroNormal, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
            }
            newArgumentValues[argumentCount - 1 - i] = newVertexNames[i - 1]; // Reverse
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, argumentCount - 1, newArgumentValues, context);
        if (errorCode != ParseErrorContext.MGF_OK) {
            return errorCode;
        }

        // Do bottom face
        if (hasNormal != 0) {
            for (i = 1; i < argumentCount - 1; i++) {
                vertexEntity[1] = newVertexNames[i - 1];
                vertexEntity[3] = argumentValues[i];
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, vertexEntity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, zeroNormal, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                newArgumentValues[i] = newVertexNames[i - 1];
            }
        }
        else {
            for (i = 1; i < argumentCount - 1; i++) {
                newArgumentValues[i] = argumentValues[i];
            }
        }
        newArgumentValues[i] = null;
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, i, newArgumentValues, context);
        if (errorCode != ParseErrorContext.MGF_OK) {
            return errorCode;
        }
        return ParseErrorContext.MGF_OK;
    }
}
