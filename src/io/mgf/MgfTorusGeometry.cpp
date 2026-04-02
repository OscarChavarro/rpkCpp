#include <cstdlib>
#include <cstring>

#include "io/context/WordsContext.h"
#include "io/mgf/MgfDefinitions.h"
#include "io/mgf/MgfGeometry.h"
#include "io/mgf/MgfHandlerGeometry.h"
#include "io/mgf/MgfTorusGeometry.h"

/**
Expand a torus into cones
*/
int
MgfTorusGeometry::handleEntity(int argumentCount, const char **argumentValues, ParseSession *context) {
    char p2[3][24];
    char radius1[24];
    char radius2[24];
    const char *v1Entity[5] = {
        context->entityNames[EntityContext::VERTEX],
        "_tv1",
        "=",
        "_tv2"
    };
    const char *v2Entity[5] = {
        context->entityNames[EntityContext::VERTEX],
        "_tv2",
        "="
    };
    const char *p2Entity[5] = {
        context->entityNames[EntityContext::MGF_POINT],
        p2[0],
        p2[1],
        p2[2]
    };
    const char *coneEntity[6] = {
        context->entityNames[EntityContext::CONE],
        "_tv1",
        radius1,
        "_tv2",
        radius2
    };
    const VertexContext *vertexContext;
    double averageRadius;
    double theta;

    if ( argumentCount != 4 ) {
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( (vertexContext = MgfHandlerGeometry::getNamedVertex(argumentValues[1], context)) == nullptr ) {
        return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( vertexContext->n.isNull(Numeric::EPSILON) ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( !WordsContext::isFloat(argumentValues[2]) || !WordsContext::isFloat(argumentValues[3]) ) {
        return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
    }
    double minRadius = strtod(argumentValues[2], nullptr);
    Numeric::roundDeltaToZero(minRadius, Numeric::EPSILON);
    double maxRadius = strtod(argumentValues[3], nullptr);

    // Check orientation
    int sign;
    if ( minRadius > 0.0 ) {
        sign = 1;
    } else if ( minRadius < 0.0 ) {
        sign = -1;
    } else {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( sign * (maxRadius - minRadius) <= 0.0 ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    context->warpConeEnds = true;
    v2Entity[3] = argumentValues[1];
    MgfGeometry::formatFloat(p2[0], 24, vertexContext->p.x + 0.5 * sign * (maxRadius - minRadius) * vertexContext->n.x);
    MgfGeometry::formatFloat(p2[1], 24, vertexContext->p.y + 0.5 * sign * (maxRadius - minRadius) * vertexContext->n.y);
    MgfGeometry::formatFloat(p2[2], 24, vertexContext->p.z + 0.5 * sign * (maxRadius - minRadius) * vertexContext->n.z);
    int errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v2Entity, context);
    if ( errorCode != ErrorCodeContext::MGF_OK ) {
        return errorCode;
    }
    errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p2Entity, context);
    if ( errorCode != ErrorCodeContext::MGF_OK ) {
        return errorCode;
    }
    MgfGeometry::formatFloat(radius2, 24, averageRadius = 0.5 * (minRadius + maxRadius));

    // Run outer section
    int i;
    for ( i = 1; i <= 2 * context->numberOfQuarterCircleDivisions; i++ ) {
        theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
        errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v1Entity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        MgfGeometry::formatFloat(p2[0], 24, vertexContext->p.x + 0.5 * sign * (maxRadius - minRadius) * java::Math::cos(theta) * vertexContext->n.x);
        MgfGeometry::formatFloat(p2[1], 24, vertexContext->p.y + 0.5 * sign * (maxRadius - minRadius) * java::Math::cos(theta) * vertexContext->n.y);
        MgfGeometry::formatFloat(p2[2], 24, vertexContext->p.z + 0.5 * sign * (maxRadius - minRadius) * java::Math::cos(theta) * vertexContext->n.z);
        errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v2Entity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p2Entity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        strcpy(radius1, radius2);
        MgfGeometry::formatFloat(radius2, 24, averageRadius + 0.5 * (maxRadius - minRadius) * java::Math::sin(theta));
        errorCode = MgfDefinitions::mgfHandle(EntityContext::CONE, 5, coneEntity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
    }

    // Run inner section
    MgfGeometry::formatFloat(radius2, 24, -0.5 * (minRadius + maxRadius));
    for ( ; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
        theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
        MgfGeometry::formatFloat(p2[0], 24, vertexContext->p.x + 0.5 * sign * (maxRadius - minRadius) * java::Math::cos(theta) * vertexContext->n.x);
        MgfGeometry::formatFloat(p2[1], 24, vertexContext->p.y + 0.5 * sign * (maxRadius - minRadius) * java::Math::cos(theta) * vertexContext->n.y);
        MgfGeometry::formatFloat(p2[2], 24, vertexContext->p.z + 0.5 * sign * (maxRadius - minRadius) * java::Math::cos(theta) * vertexContext->n.z);
        errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v1Entity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v2Entity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p2Entity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        strcpy(radius1, radius2);
        MgfGeometry::formatFloat(radius2, 24, -averageRadius - .5 * (maxRadius - minRadius) * java::Math::sin(theta));
        errorCode = MgfDefinitions::mgfHandle(EntityContext::CONE, 5, coneEntity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
    }
    context->warpConeEnds = false;
    return ErrorCodeContext::MGF_OK;
}
