package vsdk.toolkit.io.mgf;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.TokenValidationContext;
import vsdk.toolkit.io.context.VertexContext;

public final class MgfConeEntityTessellator {
    /**
    Turn a cone into polygons
    */
    public static int handleEntity(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        StringBuilder p3x = new StringBuilder(24);
        StringBuilder p3y = new StringBuilder(24);
        StringBuilder p3z = new StringBuilder(24);
        StringBuilder p4x = new StringBuilder(24);
        StringBuilder p4y = new StringBuilder(24);
        StringBuilder p4z = new StringBuilder(24);
        StringBuilder n3x = new StringBuilder(24);
        StringBuilder n3y = new StringBuilder(24);
        StringBuilder n3z = new StringBuilder(24);
        StringBuilder n4x = new StringBuilder(24);
        StringBuilder n4y = new StringBuilder(24);
        StringBuilder n4z = new StringBuilder(24);
        String[] v1Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_cv1",
            "=",
            null
        };
        String[] v2Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_cv2",
            "=",
            "_cv3"
        };
        String[] v3Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_cv3",
            "="
        };
        String[] p3Entity = new String[] {
            context.entityNames[EntityTypeContext.MGF_POINT], null, null, null};
        String[] n3Entity = new String[] {
            context.entityNames[EntityTypeContext.MGF_NORMAL], null, null, null};
        String[] v4Entity = new String[] {
            context.entityNames[EntityTypeContext.VERTEX],
            "_cv4",
            "="};
        String[] p4Entity = new String[] {
            context.entityNames[EntityTypeContext.MGF_POINT],
            null,
            null,
            null
        };
        String[] n4Entity = new String[] {
            context.entityNames[EntityTypeContext.MGF_NORMAL],
            null,
            null,
            null
        };
        String[] faceEntity = new String[] {
            context.entityNames[EntityTypeContext.FACE],
            "_cv1",
            "_cv2",
            "_cv3",
            "_cv4"
        };
        String v1Name;
        VertexContext v1Context;
        VertexContext v2Context;
        double normalOffset1;
        double normalOffset2;
        double d;
        int errorCode;
        double theta;

        if (argumentCount != 5) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        v1Context = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1], context);
        v2Context = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[3], context);
        if (v1Context == null || v2Context == null) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        v1Name = argumentValues[1];
        if (!TokenValidationContext.isFloat(argumentValues[2]) || !TokenValidationContext.isFloat(argumentValues[4])) {
            return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }

        // Set up (radius1, radius2)
        double radius1 = Double.parseDouble(argumentValues[2]);
        if (radius1 <= Numeric.EPSILON && radius1 >= -Numeric.EPSILON) {
            radius1 = 0.0;
        }
        double radius2 = Double.parseDouble(argumentValues[4]);
        if (radius2 <= Numeric.EPSILON && radius2 >= -Numeric.EPSILON) {
            radius2 = 0.0;
        }

        if (radius1 == 0.0) {
            if (radius2 == 0.0) {
                return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
        } else if (radius2 != 0.0) {
            boolean a = radius1 < 0.0;
            boolean b = radius2 < 0.0;
            boolean check = (a && !b) || (!a && b); // Note: this is exclusive or / XOR a ^ b
            if (check) {
                return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
        } else {
            // Swap
            VertexContext swappedVertexContext;

            swappedVertexContext = v1Context;
            v1Context = v2Context;
            v2Context = swappedVertexContext;
            v1Name = argumentValues[3];
            d = radius1;
            radius1 = radius2;
            radius2 = d;
        }
        int sign = radius2 < 0.0 ? -1 : 1;

        // Initialize
        Vector3Dd w = new Vector3Dd();

        w.x = v1Context.p.x - v2Context.p.x;
        w.y = v1Context.p.y - v2Context.p.y;
        w.z = v1Context.p.z - v2Context.p.z;

        d = w.normalizeAndGivePreviousNorm(Numeric.EPSILON);
        if (Numeric.doubleEqual(d, 0.0, Numeric.EPSILON)) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        normalOffset1 = normalOffset2 = (radius2 - radius1) / d;
        if (context.warpConeEnds) {
            // Hack for mgfEntitySphere and mgfEntityTorus
            d = Math.atan(normalOffset2) - (Math.PI / 4) / context.numberOfQuarterCircleDivisions;
            if (d <= -Math.PI / 2 + Numeric.EPSILON) {
                normalOffset2 = -Numeric.HUGE_FLOAT_VALUE;
            } else {
                normalOffset2 = Math.tan(d);
            }
        }

        Vector3Dd u = new Vector3Dd();
        Vector3Dd v = new Vector3Dd();
        MgfTessellationMath.mgfMakeAxes(u, v, w, Numeric.EPSILON);

        MgfTessellationMath.formatFloat(p3x, 24, v2Context.p.x + radius2 * u.x);
        if (normalOffset2 <= -Numeric.HUGE_FLOAT_VALUE) {
            MgfTessellationMath.formatFloat(n3x, 24, -w.x);
        } else {
            MgfTessellationMath.formatFloat(n3x, 24, u.x + w.x * normalOffset2);
        }

        MgfTessellationMath.formatFloat(p3y, 24, v2Context.p.y + radius2 * u.y);
        if (normalOffset2 <= -Numeric.HUGE_FLOAT_VALUE) {
            MgfTessellationMath.formatFloat(n3y, 24, -w.y);
        } else {
            MgfTessellationMath.formatFloat(n3y, 24, u.y + w.y * normalOffset2);
        }

        MgfTessellationMath.formatFloat(p3z, 24, v2Context.p.z + radius2 * u.z);
        if (normalOffset2 <= -Numeric.HUGE_FLOAT_VALUE) {
            MgfTessellationMath.formatFloat(n3z, 24, -w.z);
        } else {
            MgfTessellationMath.formatFloat(n3z, 24, u.z + w.z * normalOffset2);
        }

        p3Entity[1] = p3x.toString();
        p3Entity[2] = p3y.toString();
        p3Entity[3] = p3z.toString();
        n3Entity[1] = n3x.toString();
        n3Entity[2] = n3y.toString();
        n3Entity[3] = n3z.toString();

        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, v3Entity, context);
        if (errorCode != ParseErrorContext.MGF_OK) {
            return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity, context);
        if (errorCode != ParseErrorContext.MGF_OK) {
            return errorCode;
        }
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n3Entity, context);
        if (errorCode != ParseErrorContext.MGF_OK) {
            return errorCode;
        }
        if (radius1 == 0.0) {
            // TODO: Review floating point comparisons vs EPSILON
            // Triangles
            v1Entity[3] = v1Name;
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }

            MgfTessellationMath.formatFloat(n4x, 24, w.x);
            MgfTessellationMath.formatFloat(n4y, 24, w.y);
            MgfTessellationMath.formatFloat(n4z, 24, w.z);
            n4Entity[1] = n4x.toString();
            n4Entity[2] = n4y.toString();
            n4Entity[3] = n4z.toString();

            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n4Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            for (int i = 1; i <= 4 * context.numberOfQuarterCircleDivisions; i++) {
                theta = sign * i * (Math.PI / 2) / context.numberOfQuarterCircleDivisions;
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v2Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }

                d = u.x * Math.cos(theta) + v.x * Math.sin(theta);
                MgfTessellationMath.formatFloat(p3x, 24, v2Context.p.x + radius2 * d);
                if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
                    MgfTessellationMath.formatFloat(n3x, 24, d + w.x * normalOffset2);
                }

                d = u.y * Math.cos(theta) + v.y * Math.sin(theta);
                MgfTessellationMath.formatFloat(p3y, 24, v2Context.p.y + radius2 * d);
                if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
                    MgfTessellationMath.formatFloat(n3y, 24, d + w.y * normalOffset2);
                }

                d = u.z * Math.cos(theta) + v.z * Math.sin(theta);
                MgfTessellationMath.formatFloat(p3z, 24, v2Context.p.z + radius2 * d);
                if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
                    MgfTessellationMath.formatFloat(n3z, 24, d + w.z * normalOffset2);
                }
                p3Entity[1] = p3x.toString();
                p3Entity[2] = p3y.toString();
                p3Entity[3] = p3z.toString();
                n3Entity[1] = n3x.toString();
                n3Entity[2] = n3y.toString();
                n3Entity[3] = n3z.toString();

                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v3Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n3Entity, context);
                if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE && errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.FACE, 4, faceEntity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
            }
        } else {
            // Quads
            v1Entity[3] = "_cv4";
            if (context.warpConeEnds) {
                // Hack for mgfEntitySphere and mgfEntityTorus
                d = Math.atan(normalOffset1) + (Math.PI / 4) / context.numberOfQuarterCircleDivisions;
                if (d >= Math.PI / 2 - Numeric.EPSILON) {
                    normalOffset1 = Numeric.HUGE_FLOAT_VALUE;
                } else {
                    normalOffset1 = Math.tan(Math.atan(normalOffset1) + (Math.PI / 4) / context.numberOfQuarterCircleDivisions);
                }
            }

            MgfTessellationMath.formatFloat(p4x, 24, v1Context.p.x + radius1 * u.x);
            if (normalOffset1 >= Numeric.HUGE_FLOAT_VALUE) {
                MgfTessellationMath.formatFloat(n4x, 24, w.x);
            } else {
                MgfTessellationMath.formatFloat(n4x, 24, u.x + w.x * normalOffset1);
            }

            MgfTessellationMath.formatFloat(p4y, 24, v1Context.p.y + radius1 * u.y);
            if (normalOffset1 >= Numeric.HUGE_FLOAT_VALUE) {
                MgfTessellationMath.formatFloat(n4y, 24, w.y);
            } else {
                MgfTessellationMath.formatFloat(n4y, 24, u.y + w.y * normalOffset1);
            }

            MgfTessellationMath.formatFloat(p4z, 24, v1Context.p.z + radius1 * u.z);
            if (normalOffset1 >= Numeric.HUGE_FLOAT_VALUE) {
                MgfTessellationMath.formatFloat(n4z, 24, w.z);
            } else {
                MgfTessellationMath.formatFloat(n4z, 24, u.z + w.z * normalOffset1);
            }
            p4Entity[1] = p4x.toString();
            p4Entity[2] = p4y.toString();
            p4Entity[3] = p4z.toString();
            n4Entity[1] = n4x.toString();
            n4Entity[2] = n4y.toString();
            n4Entity[3] = n4z.toString();

            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, v4Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p4Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n4Entity, context);
            if (errorCode != ParseErrorContext.MGF_OK) {
                return errorCode;
            }
            for (int i = 1; i <= 4 * context.numberOfQuarterCircleDivisions; i++) {
                theta = sign * i * (Math.PI / 2) / context.numberOfQuarterCircleDivisions;
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v1Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 4, v2Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }

                d = u.x * Math.cos(theta) + v.x * Math.sin(theta);
                MgfTessellationMath.formatFloat(p3x, 24, v2Context.p.x + radius2 * d);
                if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
                    MgfTessellationMath.formatFloat(n3x, 24, d + w.x * normalOffset2);
                }
                MgfTessellationMath.formatFloat(p4x, 24, v1Context.p.x + radius1 * d);
                if (normalOffset1 < Numeric.HUGE_FLOAT_VALUE) {
                    MgfTessellationMath.formatFloat(n4x, 24, d + w.x * normalOffset1);
                }

                d = u.y * Math.cos(theta) + v.y * Math.sin(theta);
                MgfTessellationMath.formatFloat(p3y, 24, v2Context.p.y + radius2 * d);
                if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
                    MgfTessellationMath.formatFloat(n3y, 24, d + w.y * normalOffset2);
                }
                MgfTessellationMath.formatFloat(p4y, 24, v1Context.p.y + radius1 * d);
                if (normalOffset1 < Numeric.HUGE_FLOAT_VALUE) {
                    MgfTessellationMath.formatFloat(n4y, 24, d + w.y * normalOffset1);
                }

                d = u.z * Math.cos(theta) + v.z * Math.sin(theta);
                MgfTessellationMath.formatFloat(p3z, 24, v2Context.p.z + radius2 * d);
                if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE) {
                    MgfTessellationMath.formatFloat(n3z, 24, d + w.z * normalOffset2);
                }
                MgfTessellationMath.formatFloat(p4z, 24, v1Context.p.z + radius1 * d);
                if (normalOffset1 < Numeric.HUGE_FLOAT_VALUE) {
                    MgfTessellationMath.formatFloat(n4z, 24, d + w.z * normalOffset1);
                }

                p3Entity[1] = p3x.toString();
                p3Entity[2] = p3y.toString();
                p3Entity[3] = p3z.toString();
                n3Entity[1] = n3x.toString();
                n3Entity[2] = n3y.toString();
                n3Entity[3] = n3z.toString();
                p4Entity[1] = p4x.toString();
                p4Entity[2] = p4y.toString();
                p4Entity[3] = p4z.toString();
                n4Entity[1] = n4x.toString();
                n4Entity[2] = n4y.toString();
                n4Entity[3] = n4z.toString();

                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v3Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p3Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n3Entity, context);
                if (normalOffset2 > -Numeric.HUGE_FLOAT_VALUE && errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 2, v4Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, p4Entity, context);
                if (errorCode != ParseErrorContext.MGF_OK) {
                    return errorCode;
                }
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, n4Entity, context);
                if (normalOffset1 < Numeric.HUGE_FLOAT_VALUE &&
                    errorCode != ParseErrorContext.MGF_OK) {
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
