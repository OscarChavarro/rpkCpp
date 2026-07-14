#include <cstdlib>

#include "vsdk/toolkit/io/context/TokenValidationContext.h"
#include "vsdk/toolkit/io/mgf/MgfEntityControl.h"
#include "vsdk/toolkit/io/mgf/MgfTessellationMath.h"
#include "vsdk/toolkit/io/mgf/MgfVertexFaceEntitySupport.h"
#include "vsdk/toolkit/io/mgf/MgfRingEntityTessellator.h"

/**
Turn a ring into polygons
*/
int
MgfRingEntityTessellator::handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    char p3[3][24];
    char p4[3][24];
    const char *namesEntity[5] = {
        context->entityNames[EntityTypeContext::MGF_NORMAL],
        "0",
        "0",
        "0"
    };
    const char *v1Entity[5] = {
        context->entityNames[EntityTypeContext::VERTEX],
        "_rv1",
        "="
    };
    const char *v2Entity[5] = {
        context->entityNames[EntityTypeContext::VERTEX],
        "_rv2",
        "=",
        "_rv3"
    };
    const char *v3Entity[4] = {
        context->entityNames[EntityTypeContext::VERTEX],
        "_rv3",
        "="
    };
    const char *p3Entity[5] = {
        context->entityNames[EntityTypeContext::MGF_POINT],
        p3[0],
        p3[1],
        p3[2]
    };
    const char *v4Entity[4] = {
        context->entityNames[EntityTypeContext::VERTEX],
        "_rv4",
        "="
    };
    const char *p4Entity[5] = {
        context->entityNames[EntityTypeContext::MGF_POINT],
        p4[0],
        p4[1],
        p4[2]
    };
    const char *faceEntity[6] = {
        context->entityNames[EntityTypeContext::FACE],
        "_rv1",
        "_rv2",
        "_rv3",
        "_rv4"
    };
    double theta;

    if ( argumentCount != 4 ) {
        return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    const VertexContext *vertexContext = MgfVertexFaceEntitySupport::getNamedVertex(argumentValues[1], context);
    if ( vertexContext == nullptr) {
        return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( vertexContext->n.isNull(Numeric::EPSILON) ) {
        return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( !TokenValidationContext::isFloat(argumentValues[2]) || !TokenValidationContext::isFloat(argumentValues[3]) ) {
        return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
    }
    double minRadius = strtod(argumentValues[2], nullptr);
    Numeric::roundDeltaToZero(minRadius, Numeric::EPSILON);
    double maxRadius = strtod(argumentValues[3], nullptr);
    if ( minRadius < 0.0 || maxRadius <= minRadius ) {
        return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    Vector3Dd u;
    Vector3Dd v;

    MgfTessellationMath::mgfMakeAxes(&u, &v, &vertexContext->n, Numeric::EPSILON);
    MgfTessellationMath::formatFloat(p3[0], 24, vertexContext->p.x + maxRadius * u.x);
    MgfTessellationMath::formatFloat(p3[1], 24, vertexContext->p.y + maxRadius * u.y);
    MgfTessellationMath::formatFloat(p3[2], 24, vertexContext->p.z + maxRadius * u.z);
    int errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 3, v3Entity, context);
    if ( errorCode != ParseErrorContext::MGF_OK ) {
        return errorCode;
    }
    errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::MGF_POINT, 4, p3Entity, context);
    if ( errorCode != ParseErrorContext::MGF_OK ) {
        return errorCode;
    }

    if ( Numeric::doubleEqual(minRadius, 0.0, Numeric::EPSILON) ) {
        // Closed
        v1Entity[3] = argumentValues[1];
        errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 4, v1Entity, context);
        if ( errorCode != ParseErrorContext::MGF_OK ) {
            return errorCode;
        }
        errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::MGF_NORMAL, 4, namesEntity, context);
        if ( errorCode != ParseErrorContext::MGF_OK ) {
            return errorCode;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 4, v2Entity, context);
            if ( errorCode != ParseErrorContext::MGF_OK ) {
                return errorCode;
            }

            MgfTessellationMath::formatFloat(
                p3[0], 24,
                vertexContext->p.x + maxRadius * u.x * java::Math::cos(theta) + maxRadius * v.x * java::Math::sin(theta));
            MgfTessellationMath::formatFloat(
                p3[1], 24,
                vertexContext->p.y + maxRadius * u.y * java::Math::cos(theta) + maxRadius * v.y * java::Math::sin(theta));
            MgfTessellationMath::formatFloat(
                p3[2], 24,
                vertexContext->p.z + maxRadius * u.z * java::Math::cos(theta) + maxRadius * v.z * java::Math::sin(theta));

            errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 2, v3Entity, context);
            if ( errorCode != ParseErrorContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::MGF_POINT, 4, p3Entity, context);
            if ( errorCode != ParseErrorContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::FACE, 4, faceEntity, context);
            if ( errorCode != ParseErrorContext::MGF_OK ) {
                return errorCode;
            }
        }
    } else {
        // Open
        errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 3, v4Entity, context);
        if ( errorCode != ParseErrorContext::MGF_OK ) {
            return errorCode;
        }

        MgfTessellationMath::formatFloat(p4[0], 24, vertexContext->p.x + minRadius * u.x);
        MgfTessellationMath::formatFloat(p4[1], 24, vertexContext->p.y + minRadius * u.y);
        MgfTessellationMath::formatFloat(p4[2], 24, vertexContext->p.z + minRadius * u.z);

        errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::MGF_POINT, 4, p4Entity, context);
        if ( errorCode != ParseErrorContext::MGF_OK ) {
            return errorCode;
        }
        v1Entity[3] = "_rv4";
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 4, v1Entity, context);
            if ( errorCode != ParseErrorContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 4, v2Entity, context);
            if ( errorCode != ParseErrorContext::MGF_OK ) {
                return errorCode;
            }

            double delta = u.x * java::Math::cos(theta) + v.x * java::Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[0], 24, vertexContext->p.x + maxRadius * delta);
            MgfTessellationMath::formatFloat(p4[0], 24, vertexContext->p.x + minRadius * delta);

            delta = u.y * java::Math::cos(theta) + v.y * java::Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[1], 24, vertexContext->p.y + maxRadius * delta);
            MgfTessellationMath::formatFloat(p4[1], 24, vertexContext->p.y + minRadius * delta);

            delta = u.z * java::Math::cos(theta) + v.z * java::Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[2], 24, vertexContext->p.z + maxRadius * delta);
            MgfTessellationMath::formatFloat(p4[2], 24, vertexContext->p.z + minRadius * delta);

            errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 2, v3Entity, context);
            if ( errorCode != ParseErrorContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::MGF_POINT, 4, p3Entity, context);
            if ( errorCode != ParseErrorContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 2, v4Entity, context);
            if ( errorCode != ParseErrorContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::MGF_POINT, 4, p4Entity, context);
            if ( errorCode != ParseErrorContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::FACE, 5, faceEntity, context);
            if ( errorCode != ParseErrorContext::MGF_OK ) {
                return errorCode;
            }
        }
    }
    return ParseErrorContext::MGF_OK;
}
