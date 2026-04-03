#include <cstdlib>
#include <cstring>

#include "io/context/TokenValidationContext.h"
#include "io/mgf/MgfEntityControl.h"
#include "io/mgf/MgfTessellationMath.h"
#include "io/mgf/MgfVertexFaceEntitySupport.h"
#include "io/mgf/MgfSphereEntityExpander.h"

/**
Expand a sphere into cones
*/
int
MgfSphereEntityExpander::handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    char p2x[24];
    char p2y[24];
    char p2z[24];
    char radius1[24];
    char radius2[24];
    const char *v1Entity[5] = {
        context->entityNames[EntityTypeContext::VERTEX],
        "_sv1",
        "=",
        "_sv2"
    };
    const char *v2Entity[4] = {
        context->entityNames[EntityTypeContext::VERTEX],
        "_sv2",
        "="
    };
    const char *p2Entity[5] = {context->entityNames[EntityTypeContext::MGF_POINT], p2x, p2y, p2z};
    const char *coneEntity[6] = {
        context->entityNames[EntityTypeContext::CONE],
        "_sv1",
        radius1,
        "_sv2",
        radius2
    };

    if ( argumentCount != 3 ) {
        return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    const VertexContext *vertexContext = MgfVertexFaceEntitySupport::getNamedVertex(argumentValues[1], context);
    if ( vertexContext == nullptr) {
        return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( !TokenValidationContext::isFloat(argumentValues[2]) ) {
        return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
    }
    double radius = strtod(argumentValues[2], nullptr);

    // Initialize
    context->warpConeEnds = true;
    int errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 3, v2Entity, context);
    if ( errorCode != ParseErrorContext::MGF_OK ) {
        return errorCode;
    }
    MgfTessellationMath::formatFloat(p2x, 24, vertexContext->p.x);
    MgfTessellationMath::formatFloat(p2y, 24, vertexContext->p.y);
    MgfTessellationMath::formatFloat(p2z, 24, vertexContext->p.z + radius);
    errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::MGF_POINT, 4, p2Entity, context);
    if ( errorCode != ParseErrorContext::MGF_OK ) {
        return errorCode;
    }
    radius2[0] = '0';
    radius2[1] = '\0';
    for ( int i = 1; i <= 2 * context->numberOfQuarterCircleDivisions; i++ ) {
        double theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
        errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 4, v1Entity, context);
        if ( errorCode != ParseErrorContext::MGF_OK ) {
            return errorCode;
        }
        MgfTessellationMath::formatFloat(p2z, 24, vertexContext->p.z + radius * java::Math::cos(theta));
        errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::VERTEX, 2, v2Entity, context);
        if ( errorCode != ParseErrorContext::MGF_OK ) {
            return errorCode;
        }
        errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::MGF_POINT, 4, p2Entity, context);
        if ( errorCode != ParseErrorContext::MGF_OK ) {
            return errorCode;
        }
        strcpy(radius1, radius2);
        MgfTessellationMath::formatFloat(radius2, 24, radius * java::Math::sin(theta));
        errorCode = MgfEntityControl::mgfHandle(EntityTypeContext::CONE, 5, coneEntity, context);
        if ( errorCode != ParseErrorContext::MGF_OK ) {
            return errorCode;
        }
    }
    context->warpConeEnds = false;
    return ParseErrorContext::MGF_OK;
}
