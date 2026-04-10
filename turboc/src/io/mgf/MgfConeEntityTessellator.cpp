#include <stdlib.h>

#include "io/context/TokenValidationContext.h"
#include "io/mgf/MgfEntityControl.h"
#include "io/mgf/MgfTessellationMath.h"
#include "io/mgf/MgfVertexFaceEntitySupport.h"
#include "io/mgf/MgfConeEntityTessellator.h"

/**
Turn a cone into polygons
*/
int
MgfConeEntityTessellator::handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    char p3[3][24];
    char p4[3][24];
    char n3[3][24];
    char n4[3][24];
    const char *v1Entity[5] = {
        context->entityNames[VERTEX],
        "_cv1",
        "="
    };
    const char *v2Entity[5] = {
        context->entityNames[VERTEX],
        "_cv2",
        "=",
        "_cv3"
    };
    const char *v3Entity[4] = {
        context->entityNames[VERTEX],
        "_cv3",
        "="
    };
    const char *p3Entity[5] = {
        context->entityNames[MGF_POINT], p3[0], p3[1], p3[2]};
    const char *n3Entity[5] = {
        context->entityNames[MGF_NORMAL], n3[0], n3[1], n3[2]};
    const char *v4Entity[4] = {
        context->entityNames[VERTEX],
        "_cv4",
        "="};
    const char *p4Entity[5] = {
        context->entityNames[MGF_POINT],
        p4[0],
        p4[1],
        p4[2]
    };
    const char *n4Entity[5] = {
        context->entityNames[MGF_NORMAL],
        n4[0],
        n4[1],
        n4[2]
    };
    const char *faceEntity[6] = {
        context->entityNames[FACE],
        "_cv1",
        "_cv2",
        "_cv3",
        "_cv4"
    };
    const char *v1Name;
    VertexContext *v1Context;
    VertexContext *v2Context;
    double normalOffset1;
    double normalOffset2;
    double d;
    int errorCode;
    double theta;

    if ( argumentCount != 5 ) {
        return MGF_ERRR_WRNG_NUM_O_ARGMN;
    }
    v1Context = MgfVertexFaceEntitySupport::getNamedVertex(argumentValues[1], context);
    v2Context = MgfVertexFaceEntitySupport::getNamedVertex(argumentValues[3], context);
    if ( v1Context == NULL || v2Context == NULL) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    v1Name = argumentValues[1];
    if ( !TokenValidationContext::isFloat(argumentValues[2]) || !TokenValidationContext::isFloat(argumentValues[4]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }

    // Set up (radius1, radius2)
    double radius1 = strtod(argumentValues[2], NULL);
    Numeric::roundDeltaToZero(radius1, Numeric::EPSILON);
    double radius2 = strtod(argumentValues[4], NULL);
    Numeric::roundDeltaToZero(radius2, Numeric::EPSILON);

    if ( radius1 == 0.0 ) {
        if ( radius2 == 0.0 ) {
            return MGF_ERRR_ILLGL_ARGMN_VAL;
        }
    } else if ( radius2 != 0.0 ) {
        bool a = radius1 < 0.0;
        bool b = radius2 < 0.0;
        bool check = (a && !b) || (!a && b); // Note: this is exclusive or / XOR a ^ b
        if ( check ) {
            return MGF_ERRR_ILLGL_ARGMN_VAL;
        }
    } else {
        // Swap
        VertexContext *swappedVertexContext;

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
    Vector3Dd w;

    w.x = v1Context->p.x - v2Context->p.x;
    w.y = v1Context->p.y - v2Context->p.y;
    w.z = v1Context->p.z - v2Context->p.z;

    d = w.normalizeAndGivePreviousNorm(Numeric::EPSILON);
    if ( Numeric::doubleEqual(d, 0.0, Numeric::EPSILON) ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }
    normalOffset1 = normalOffset2 = (radius2 - radius1) / d;
    if ( context->warpConeEnds ) {
        // Hack for mgfEntitySphere and mgfEntityTorus
        d = Math::atan(normalOffset2) - (M_PI / 4) / context->numberOfQuarterCircleDivisions;
        if ( d <= -M_PI / 2 + Numeric::EPSILON ) {
            normalOffset2 = -Numeric::HUGE_FLOAT_VALUE;
        } else {
            normalOffset2 = Math::tan(d);
        }
    }

    Vector3Dd u;
    Vector3Dd v;
    MgfTessellationMath::mgfMakeAxes(&u, &v, &w, Numeric::EPSILON);

    MgfTessellationMath::formatFloat(p3[0], 24, v2Context->p.x + radius2 * u.x);
    if ( normalOffset2 <= -Numeric::HUGE_FLOAT_VALUE) {
        MgfTessellationMath::formatFloat(n3[0], 24, -w.x);
    } else {
        MgfTessellationMath::formatFloat(n3[0], 24, u.x + w.x * normalOffset2);
    }

    MgfTessellationMath::formatFloat(p3[1], 24, v2Context->p.y + radius2 * u.y);
    if ( normalOffset2 <= -Numeric::HUGE_FLOAT_VALUE) {
        MgfTessellationMath::formatFloat(n3[1], 24, -w.y);
    } else {
        MgfTessellationMath::formatFloat(n3[1], 24, u.y + w.y * normalOffset2);
    }

    MgfTessellationMath::formatFloat(p3[2], 24, v2Context->p.z + radius2 * u.z);
    if ( normalOffset2 <= -Numeric::HUGE_FLOAT_VALUE) {
        MgfTessellationMath::formatFloat(n3[2], 24, -w.z);
    } else {
        MgfTessellationMath::formatFloat(n3[2], 24, u.z + w.z * normalOffset2);
    }

    errorCode = MgfEntityControl::mgfHandle(VERTEX, 3, v3Entity, context);
    if ( errorCode != MGF_OK ) {
        return errorCode;
    }
    errorCode = MgfEntityControl::mgfHandle(MGF_POINT, 4, p3Entity, context);
    if ( errorCode != MGF_OK ) {
        return errorCode;
    }
    errorCode = MgfEntityControl::mgfHandle(MGF_NORMAL, 4, n3Entity, context);
    if ( errorCode != MGF_OK ) {
        return errorCode;
    }
    if ( radius1 == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        // Triangles
        v1Entity[3] = v1Name;
        errorCode = MgfEntityControl::mgfHandle(VERTEX, 4, v1Entity, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }

        MgfTessellationMath::formatFloat(n4[0], 24, w.x);
        MgfTessellationMath::formatFloat(n4[1], 24, w.y);
        MgfTessellationMath::formatFloat(n4[2], 24, w.z);

        errorCode = MgfEntityControl::mgfHandle(MGF_NORMAL, 4, n4Entity, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = sign * i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            errorCode = MgfEntityControl::mgfHandle(VERTEX, 4, v2Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }

            d = u.x * Math::cos(theta) + v.x * Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[0], 24, v2Context->p.x + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfTessellationMath::formatFloat(n3[0], 24, d + w.x * normalOffset2);
            }

            d = u.y * Math::cos(theta) + v.y * Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[1], 24, v2Context->p.y + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfTessellationMath::formatFloat(n3[1], 24, d + w.y * normalOffset2);
            }

            d = u.z * Math::cos(theta) + v.z * Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[2], 24, v2Context->p.z + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfTessellationMath::formatFloat(n3[2], 24, d + w.z * normalOffset2);
            }

            errorCode = MgfEntityControl::mgfHandle(VERTEX, 2, v3Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(MGF_POINT, 4, p3Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(MGF_NORMAL, 4, n3Entity, context);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE && errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(FACE, 4, faceEntity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
        }
    } else {
        // Quads
        v1Entity[3] = "_cv4";
        if ( context->warpConeEnds ) {
            // Hack for mgfEntitySphere and mgfEntityTorus
            d = Math::atan(normalOffset1) + (M_PI / 4) / context->numberOfQuarterCircleDivisions;
            if ( d >= M_PI / 2 - Numeric::EPSILON ) {
                normalOffset1 = Numeric::HUGE_FLOAT_VALUE;
            } else {
                normalOffset1 = Math::tan(Math::atan(normalOffset1) + (M_PI / 4) / context->numberOfQuarterCircleDivisions);
            }
        }

        MgfTessellationMath::formatFloat(p4[0], 24, v1Context->p.x + radius1 * u.x);
        if ( normalOffset1 >= Numeric::HUGE_FLOAT_VALUE) {
            MgfTessellationMath::formatFloat(n4[0], 24, w.x);
        } else {
            MgfTessellationMath::formatFloat(n4[0], 24, u.x + w.x * normalOffset1);
        }

        MgfTessellationMath::formatFloat(p4[1], 24, v1Context->p.y + radius1 * u.y);
        if ( normalOffset1 >= Numeric::HUGE_FLOAT_VALUE) {
            MgfTessellationMath::formatFloat(n4[1], 24, w.y);
        } else {
            MgfTessellationMath::formatFloat(n4[1], 24, u.y + w.y * normalOffset1);
        }

        MgfTessellationMath::formatFloat(p4[2], 24, v1Context->p.z + radius1 * u.z);
        if ( normalOffset1 >= Numeric::HUGE_FLOAT_VALUE) {
            MgfTessellationMath::formatFloat(n4[2], 24, w.z);
        } else {
            MgfTessellationMath::formatFloat(n4[2], 24, u.z + w.z * normalOffset1);
        }

        errorCode = MgfEntityControl::mgfHandle(VERTEX, 3, v4Entity, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }
        errorCode = MgfEntityControl::mgfHandle(MGF_POINT, 4, p4Entity, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }
        errorCode = MgfEntityControl::mgfHandle(MGF_NORMAL, 4, n4Entity, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = sign * i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            errorCode = MgfEntityControl::mgfHandle(VERTEX, 4, v1Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(VERTEX, 4, v2Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }

            d = u.x * Math::cos(theta) + v.x * Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[0], 24, v2Context->p.x + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfTessellationMath::formatFloat(n3[0], 24, d + w.x * normalOffset2);
            }
            MgfTessellationMath::formatFloat(p4[0], 24, v1Context->p.x + radius1 * d);
            if ( normalOffset1 < Numeric::HUGE_FLOAT_VALUE) {
                MgfTessellationMath::formatFloat(n4[0], 24, d + w.x * normalOffset1);
            }

            d = u.y * Math::cos(theta) + v.y * Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[1], 24, v2Context->p.y + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfTessellationMath::formatFloat(n3[1], 24, d + w.y * normalOffset2);
            }
            MgfTessellationMath::formatFloat(p4[1], 24, v1Context->p.y + radius1 * d);
            if ( normalOffset1 < Numeric::HUGE_FLOAT_VALUE) {
                MgfTessellationMath::formatFloat(n4[1], 24, d + w.y * normalOffset1);
            }

            d = u.z * Math::cos(theta) + v.z * Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[2], 24, v2Context->p.z + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfTessellationMath::formatFloat(n3[2], 24, d + w.z * normalOffset2);
            }
            MgfTessellationMath::formatFloat(p4[2], 24, v1Context->p.z + radius1 * d);
            if ( normalOffset1 < Numeric::HUGE_FLOAT_VALUE) {
                MgfTessellationMath::formatFloat(n4[2], 24, d + w.z * normalOffset1);
            }

            errorCode = MgfEntityControl::mgfHandle(VERTEX, 2, v3Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(MGF_POINT, 4, p3Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(MGF_NORMAL, 4, n3Entity, context);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE && errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(VERTEX, 2, v4Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(MGF_POINT, 4, p4Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(MGF_NORMAL, 4, n4Entity, context);
            if ( normalOffset1 < Numeric::HUGE_FLOAT_VALUE &&
                 errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(FACE, 5, faceEntity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
        }
    }
    return MGF_OK;
}
