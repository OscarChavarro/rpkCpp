#include <stdlib.h>

#include "io/context/TokenValidationContext.h"
#include "io/mgf/MgfEntityControl.h"
#include "io/mgf/MgfTessellationMath.h"
#include "io/mgf/MgfVertexFaceEntitySupport.h"
#include "io/mgf/MgfRingEntityTessellator.h"

/**
Turn a ring into polygons
*/
int
MgfRingEntityTessellator::handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    char p3[3][24];
    char p4[3][24];
    const char *namesEntity[5] = {
        context->entityNames[MGF_NORMAL],
        "0",
        "0",
        "0"
    };
    const char *v1Entity[5] = {
        context->entityNames[VERTEX],
        "_rv1",
        "="
    };
    const char *v2Entity[5] = {
        context->entityNames[VERTEX],
        "_rv2",
        "=",
        "_rv3"
    };
    const char *v3Entity[4] = {
        context->entityNames[VERTEX],
        "_rv3",
        "="
    };
    const char *p3Entity[5] = {
        context->entityNames[MGF_POINT],
        p3[0],
        p3[1],
        p3[2]
    };
    const char *v4Entity[4] = {
        context->entityNames[VERTEX],
        "_rv4",
        "="
    };
    const char *p4Entity[5] = {
        context->entityNames[MGF_POINT],
        p4[0],
        p4[1],
        p4[2]
    };
    const char *faceEntity[6] = {
        context->entityNames[FACE],
        "_rv1",
        "_rv2",
        "_rv3",
        "_rv4"
    };
    double theta;

    if ( argumentCount != 4 ) {
        return MGF_ERRR_WRNG_NUM_O_ARGMN;
    }

    const VertexContext *vertexContext = MgfVertexFaceEntitySupport::getNamedVertex(argumentValues[1], context);
    if ( vertexContext == NULL) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( vertexContext->n.isNull(Numeric::EPSILON) ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }
    if ( !TokenValidationContext::isFloat(argumentValues[2]) || !TokenValidationContext::isFloat(argumentValues[3]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    double minRadius = strtod(argumentValues[2], NULL);
    Numeric::roundDeltaToZero(minRadius, Numeric::EPSILON);
    double maxRadius = strtod(argumentValues[3], NULL);
    if ( minRadius < 0.0 || maxRadius <= minRadius ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }

    // Initialize
    Vector3Dd u;
    Vector3Dd v;

    MgfTessellationMath::mgfMakeAxes(&u, &v, &vertexContext->n, Numeric::EPSILON);
    MgfTessellationMath::formatFloat(p3[0], 24, vertexContext->p.x + maxRadius * u.x);
    MgfTessellationMath::formatFloat(p3[1], 24, vertexContext->p.y + maxRadius * u.y);
    MgfTessellationMath::formatFloat(p3[2], 24, vertexContext->p.z + maxRadius * u.z);
    int errorCode = MgfEntityControl::mgfHandle(VERTEX, 3, v3Entity, context);
    if ( errorCode != MGF_OK ) {
        return errorCode;
    }
    errorCode = MgfEntityControl::mgfHandle(MGF_POINT, 4, p3Entity, context);
    if ( errorCode != MGF_OK ) {
        return errorCode;
    }

    if ( Numeric::doubleEqual(minRadius, 0.0, Numeric::EPSILON) ) {
        // Closed
        v1Entity[3] = argumentValues[1];
        errorCode = MgfEntityControl::mgfHandle(VERTEX, 4, v1Entity, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }
        errorCode = MgfEntityControl::mgfHandle(MGF_NORMAL, 4, namesEntity, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            errorCode = MgfEntityControl::mgfHandle(VERTEX, 4, v2Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }

            MgfTessellationMath::formatFloat(
                p3[0], 24,
                vertexContext->p.x + maxRadius * u.x * Math::cos(theta) + maxRadius * v.x * Math::sin(theta));
            MgfTessellationMath::formatFloat(
                p3[1], 24,
                vertexContext->p.y + maxRadius * u.y * Math::cos(theta) + maxRadius * v.y * Math::sin(theta));
            MgfTessellationMath::formatFloat(
                p3[2], 24,
                vertexContext->p.z + maxRadius * u.z * Math::cos(theta) + maxRadius * v.z * Math::sin(theta));

            errorCode = MgfEntityControl::mgfHandle(VERTEX, 2, v3Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(MGF_POINT, 4, p3Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(FACE, 4, faceEntity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
        }
    } else {
        // Open
        errorCode = MgfEntityControl::mgfHandle(VERTEX, 3, v4Entity, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }

        MgfTessellationMath::formatFloat(p4[0], 24, vertexContext->p.x + minRadius * u.x);
        MgfTessellationMath::formatFloat(p4[1], 24, vertexContext->p.y + minRadius * u.y);
        MgfTessellationMath::formatFloat(p4[2], 24, vertexContext->p.z + minRadius * u.z);

        errorCode = MgfEntityControl::mgfHandle(MGF_POINT, 4, p4Entity, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }
        v1Entity[3] = "_rv4";
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            errorCode = MgfEntityControl::mgfHandle(VERTEX, 4, v1Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(VERTEX, 4, v2Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }

            double delta = u.x * Math::cos(theta) + v.x * Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[0], 24, vertexContext->p.x + maxRadius * delta);
            MgfTessellationMath::formatFloat(p4[0], 24, vertexContext->p.x + minRadius * delta);

            delta = u.y * Math::cos(theta) + v.y * Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[1], 24, vertexContext->p.y + maxRadius * delta);
            MgfTessellationMath::formatFloat(p4[1], 24, vertexContext->p.y + minRadius * delta);

            delta = u.z * Math::cos(theta) + v.z * Math::sin(theta);
            MgfTessellationMath::formatFloat(p3[2], 24, vertexContext->p.z + maxRadius * delta);
            MgfTessellationMath::formatFloat(p4[2], 24, vertexContext->p.z + minRadius * delta);

            errorCode = MgfEntityControl::mgfHandle(VERTEX, 2, v3Entity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfEntityControl::mgfHandle(MGF_POINT, 4, p3Entity, context);
            if ( errorCode != MGF_OK ) {
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
            errorCode = MgfEntityControl::mgfHandle(FACE, 5, faceEntity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
        }
    }
    return MGF_OK;
}
