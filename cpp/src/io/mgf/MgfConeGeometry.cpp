#include <cstdlib>

#include "io/context/WordsContext.h"
#include "io/mgf/MgfDefinitions.h"
#include "io/mgf/MgfGeometry.h"
#include "io/mgf/MgfHandlerGeometry.h"
#include "io/mgf/MgfConeGeometry.h"

/**
Turn a cone into polygons
*/
int
MgfConeGeometry::handleEntity(int argumentCount, const char **argumentValues, ParseSession *context) {
    char p3[3][24];
    char p4[3][24];
    char n3[3][24];
    char n4[3][24];
    const char *v1Entity[5] = {
        context->entityNames[EntityContext::VERTEX],
        "_cv1",
        "="
    };
    const char *v2Entity[5] = {
        context->entityNames[EntityContext::VERTEX],
        "_cv2",
        "=",
        "_cv3"
    };
    const char *v3Entity[4] = {
        context->entityNames[EntityContext::VERTEX],
        "_cv3",
        "="
    };
    const char *p3Entity[5] = {
        context->entityNames[EntityContext::MGF_POINT], p3[0], p3[1], p3[2]};
    const char *n3Entity[5] = {
        context->entityNames[EntityContext::MGF_NORMAL], n3[0], n3[1], n3[2]};
    const char *v4Entity[4] = {
        context->entityNames[EntityContext::VERTEX],
        "_cv4",
        "="};
    const char *p4Entity[5] = {
        context->entityNames[EntityContext::MGF_POINT],
        p4[0],
        p4[1],
        p4[2]
    };
    const char *n4Entity[5] = {
        context->entityNames[EntityContext::MGF_NORMAL],
        n4[0],
        n4[1],
        n4[2]
    };
    const char *faceEntity[6] = {
        context->entityNames[EntityContext::FACE],
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
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    v1Context = MgfHandlerGeometry::getNamedVertex(argumentValues[1], context);
    v2Context = MgfHandlerGeometry::getNamedVertex(argumentValues[3], context);
    if ( v1Context == nullptr || v2Context == nullptr) {
        return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
    }
    v1Name = argumentValues[1];
    if ( !WordsContext::isFloat(argumentValues[2]) || !WordsContext::isFloat(argumentValues[4]) ) {
        return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
    }

    // Set up (radius1, radius2)
    double radius1 = strtod(argumentValues[2], nullptr);
    Numeric::roundDeltaToZero(radius1, Numeric::EPSILON);
    double radius2 = strtod(argumentValues[4], nullptr);
    Numeric::roundDeltaToZero(radius2, Numeric::EPSILON);

    if ( radius1 == 0.0 ) {
        if ( radius2 == 0.0 ) {
            return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
    } else if ( radius2 != 0.0 ) {
        bool a = radius1 < 0.0;
        bool b = radius2 < 0.0;
        bool check = (a && !b) || (!a && b); // Note: this is exclusive or / XOR a ^ b
        if ( check ) {
            return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
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
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    normalOffset1 = normalOffset2 = (radius2 - radius1) / d;
    if ( context->warpConeEnds ) {
        // Hack for mgfEntitySphere and mgfEntityTorus
        d = java::Math::atan(normalOffset2) - (M_PI / 4) / context->numberOfQuarterCircleDivisions;
        if ( d <= -M_PI / 2 + Numeric::EPSILON ) {
            normalOffset2 = -Numeric::HUGE_FLOAT_VALUE;
        } else {
            normalOffset2 = java::Math::tan(d);
        }
    }

    Vector3Dd u;
    Vector3Dd v;
    MgfGeometry::mgfMakeAxes(&u, &v, &w, Numeric::EPSILON);

    MgfGeometry::formatFloat(p3[0], 24, v2Context->p.x + radius2 * u.x);
    if ( normalOffset2 <= -Numeric::HUGE_FLOAT_VALUE) {
        MgfGeometry::formatFloat(n3[0], 24, -w.x);
    } else {
        MgfGeometry::formatFloat(n3[0], 24, u.x + w.x * normalOffset2);
    }

    MgfGeometry::formatFloat(p3[1], 24, v2Context->p.y + radius2 * u.y);
    if ( normalOffset2 <= -Numeric::HUGE_FLOAT_VALUE) {
        MgfGeometry::formatFloat(n3[1], 24, -w.y);
    } else {
        MgfGeometry::formatFloat(n3[1], 24, u.y + w.y * normalOffset2);
    }

    MgfGeometry::formatFloat(p3[2], 24, v2Context->p.z + radius2 * u.z);
    if ( normalOffset2 <= -Numeric::HUGE_FLOAT_VALUE) {
        MgfGeometry::formatFloat(n3[2], 24, -w.z);
    } else {
        MgfGeometry::formatFloat(n3[2], 24, u.z + w.z * normalOffset2);
    }

    errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 3, v3Entity, context);
    if ( errorCode != ErrorCodeContext::MGF_OK ) {
        return errorCode;
    }
    errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p3Entity, context);
    if ( errorCode != ErrorCodeContext::MGF_OK ) {
        return errorCode;
    }
    errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n3Entity, context);
    if ( errorCode != ErrorCodeContext::MGF_OK ) {
        return errorCode;
    }
    if ( radius1 == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        // Triangles
        v1Entity[3] = v1Name;
        errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v1Entity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }

        MgfGeometry::formatFloat(n4[0], 24, w.x);
        MgfGeometry::formatFloat(n4[1], 24, w.y);
        MgfGeometry::formatFloat(n4[2], 24, w.z);

        errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n4Entity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = sign * i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v2Entity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }

            d = u.x * java::Math::cos(theta) + v.x * java::Math::sin(theta);
            MgfGeometry::formatFloat(p3[0], 24, v2Context->p.x + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfGeometry::formatFloat(n3[0], 24, d + w.x * normalOffset2);
            }

            d = u.y * java::Math::cos(theta) + v.y * java::Math::sin(theta);
            MgfGeometry::formatFloat(p3[1], 24, v2Context->p.y + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfGeometry::formatFloat(n3[1], 24, d + w.y * normalOffset2);
            }

            d = u.z * java::Math::cos(theta) + v.z * java::Math::sin(theta);
            MgfGeometry::formatFloat(p3[2], 24, v2Context->p.z + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfGeometry::formatFloat(n3[2], 24, d + w.z * normalOffset2);
            }

            errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v3Entity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p3Entity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n3Entity, context);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE && errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::FACE, 4, faceEntity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
        }
    } else {
        // Quads
        v1Entity[3] = "_cv4";
        if ( context->warpConeEnds ) {
            // Hack for mgfEntitySphere and mgfEntityTorus
            d = java::Math::atan(normalOffset1) + (M_PI / 4) / context->numberOfQuarterCircleDivisions;
            if ( d >= M_PI / 2 - Numeric::EPSILON ) {
                normalOffset1 = Numeric::HUGE_FLOAT_VALUE;
            } else {
                normalOffset1 = java::Math::tan(java::Math::atan(normalOffset1) + (M_PI / 4) / context->numberOfQuarterCircleDivisions);
            }
        }

        MgfGeometry::formatFloat(p4[0], 24, v1Context->p.x + radius1 * u.x);
        if ( normalOffset1 >= Numeric::HUGE_FLOAT_VALUE) {
            MgfGeometry::formatFloat(n4[0], 24, w.x);
        } else {
            MgfGeometry::formatFloat(n4[0], 24, u.x + w.x * normalOffset1);
        }

        MgfGeometry::formatFloat(p4[1], 24, v1Context->p.y + radius1 * u.y);
        if ( normalOffset1 >= Numeric::HUGE_FLOAT_VALUE) {
            MgfGeometry::formatFloat(n4[1], 24, w.y);
        } else {
            MgfGeometry::formatFloat(n4[1], 24, u.y + w.y * normalOffset1);
        }

        MgfGeometry::formatFloat(p4[2], 24, v1Context->p.z + radius1 * u.z);
        if ( normalOffset1 >= Numeric::HUGE_FLOAT_VALUE) {
            MgfGeometry::formatFloat(n4[2], 24, w.z);
        } else {
            MgfGeometry::formatFloat(n4[2], 24, u.z + w.z * normalOffset1);
        }

        errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 3, v4Entity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p4Entity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n4Entity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = sign * i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v1Entity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v2Entity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }

            d = u.x * java::Math::cos(theta) + v.x * java::Math::sin(theta);
            MgfGeometry::formatFloat(p3[0], 24, v2Context->p.x + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfGeometry::formatFloat(n3[0], 24, d + w.x * normalOffset2);
            }
            MgfGeometry::formatFloat(p4[0], 24, v1Context->p.x + radius1 * d);
            if ( normalOffset1 < Numeric::HUGE_FLOAT_VALUE) {
                MgfGeometry::formatFloat(n4[0], 24, d + w.x * normalOffset1);
            }

            d = u.y * java::Math::cos(theta) + v.y * java::Math::sin(theta);
            MgfGeometry::formatFloat(p3[1], 24, v2Context->p.y + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfGeometry::formatFloat(n3[1], 24, d + w.y * normalOffset2);
            }
            MgfGeometry::formatFloat(p4[1], 24, v1Context->p.y + radius1 * d);
            if ( normalOffset1 < Numeric::HUGE_FLOAT_VALUE) {
                MgfGeometry::formatFloat(n4[1], 24, d + w.y * normalOffset1);
            }

            d = u.z * java::Math::cos(theta) + v.z * java::Math::sin(theta);
            MgfGeometry::formatFloat(p3[2], 24, v2Context->p.z + radius2 * d);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE) {
                MgfGeometry::formatFloat(n3[2], 24, d + w.z * normalOffset2);
            }
            MgfGeometry::formatFloat(p4[2], 24, v1Context->p.z + radius1 * d);
            if ( normalOffset1 < Numeric::HUGE_FLOAT_VALUE) {
                MgfGeometry::formatFloat(n4[2], 24, d + w.z * normalOffset1);
            }

            errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v3Entity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p3Entity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n3Entity, context);
            if ( normalOffset2 > -Numeric::HUGE_FLOAT_VALUE && errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v4Entity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p4Entity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n4Entity, context);
            if ( normalOffset1 < Numeric::HUGE_FLOAT_VALUE &&
                 errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::FACE, 5, faceEntity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
        }
    }
    return ErrorCodeContext::MGF_OK;
}
