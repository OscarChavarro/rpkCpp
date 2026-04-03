#include <cstdlib>

#include "java/util/Formatter.h"
#include "io/context/WordsContext.h"
#include "io/mgf/MgfDefinitions.h"
#include "io/mgf/MgfGeometry.h"
#include "io/mgf/MgfHandlerGeometry.h"
#include "io/mgf/MgfPrismGeometry.h"

/**
Turn a prism into polygons
*/
int
MgfPrismGeometry::handleEntity(int argumentCount, const char **argumentValues, ParseSession *context) {
    char p[3][24];
    const char *vertexEntity[5] = {
        context->entityNames[EntityContext::VERTEX],
        nullptr,
        "="
    };
    const char *pointEntity[5] = {
        context->entityNames[EntityContext::MGF_POINT],
        p[0],
        p[1],
        p[2]
    };
    const char *zeroNormal[5] = {
        context->entityNames[EntityContext::MGF_NORMAL],
        "0",
        "0",
        "0"
    };
    const char *newArgumentValues[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];
    char newVertexNames[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT - 1][MgfGeometry::MGF_PV_SIZE];
    const VertexContext *vertexContext;
    int errorCode;
    int i;

    // Check arguments
    if ( argumentCount < 5 ) {
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( !WordsContext::isFloat(argumentValues[argumentCount - 1]) ) {
        return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
    }
    double length = strtod(argumentValues[argumentCount - 1], nullptr);
    if ( length <= Numeric::EPSILON && length >= -Numeric::EPSILON ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Compute face normal
    const VertexContext *v0Context = MgfHandlerGeometry::getNamedVertex(argumentValues[1], context);
    if ( v0Context == nullptr ) {
        return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
    }
    int hasNormal = 0;

    Vector3Dd normal(0.0, 0.0, 0.0);
    Vector3Dd v1(0.0, 0.0, 0.0);

    for ( i = 2; i < argumentCount - 1; i++ ) {
        vertexContext = MgfHandlerGeometry::getNamedVertex(argumentValues[i], context);
        if ( vertexContext == nullptr) {
            return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
        }

        if ( !vertexContext->n.isNull(Numeric::EPSILON) ) {
            hasNormal++;
        }

        Vector3Dd v2;
        Vector3Dd v3;

        v2.x = vertexContext->p.x - v0Context->p.x;
        v2.y = vertexContext->p.y - v0Context->p.y;
        v2.z = vertexContext->p.z - v0Context->p.z;
        v3.crossProduct(&v1, &v2);
        normal.x += v3.x;
        normal.y += v3.y;
        normal.z += v3.z;
        v1.copy(&v2);
    }
    if ( normal.normalizeAndGivePreviousNorm(Numeric::EPSILON) == 0.0 ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Create moved vertices
    for ( i = 1; i < argumentCount - 1; i++ ) {
        java::Formatter::format(newVertexNames[i - 1], MgfGeometry::MGF_PV_SIZE, "_pv%d", i);
        vertexEntity[1] = newVertexNames[i - 1];
        vertexEntity[3] = argumentValues[i];
        errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, vertexEntity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        vertexContext = MgfHandlerGeometry::getNamedVertex(argumentValues[i], context); // Checked above
        MgfGeometry::formatFloat(p[0], 24, vertexContext->p.x - length * normal.x);
        MgfGeometry::formatFloat(p[1], 24, vertexContext->p.y - length * normal.y);
        MgfGeometry::formatFloat(p[2], 24, vertexContext->p.z - length * normal.z);
        errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, pointEntity, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
    }

    // Make faces
    newArgumentValues[0] = context->entityNames[EntityContext::FACE];
    // Do the side faces
    newArgumentValues[5] = nullptr;
    newArgumentValues[3] = argumentValues[argumentCount - 2];
    newArgumentValues[4] = newVertexNames[argumentCount - 3];
    for ( i = 1; i < argumentCount - 1; i++ ) {
        newArgumentValues[1] = newVertexNames[i - 1];
        newArgumentValues[2] = argumentValues[i];
        errorCode = MgfDefinitions::mgfHandle(EntityContext::FACE, 5, newArgumentValues, context);
        if ( errorCode != ErrorCodeContext::MGF_OK ) {
            return errorCode;
        }
        newArgumentValues[3] = newArgumentValues[2];
        newArgumentValues[4] = newArgumentValues[1];
    }

    // Do top face
    for ( i = 1; i < argumentCount - 1; i++ ) {
        if ( hasNormal ) {
            // Zero normals
            vertexEntity[1] = newVertexNames[i - 1];
            errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, vertexEntity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, zeroNormal, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
        }
        newArgumentValues[argumentCount - 1 - i] = newVertexNames[i - 1]; // Reverse
    }
    errorCode = MgfDefinitions::mgfHandle(EntityContext::FACE, argumentCount - 1, newArgumentValues, context);
    if ( errorCode != ErrorCodeContext::MGF_OK ) {
        return errorCode;
    }

    // Do bottom face
    if ( hasNormal != 0 ) {
        for ( i = 1; i < argumentCount - 1; i++ ) {
            vertexEntity[1] = newVertexNames[i - 1];
            vertexEntity[3] = argumentValues[i];
            errorCode = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, vertexEntity, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
            errorCode = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, zeroNormal, context);
            if ( errorCode != ErrorCodeContext::MGF_OK ) {
                return errorCode;
            }
            newArgumentValues[i] = newVertexNames[i - 1];
        }
    } else {
        for ( i = 1; i < argumentCount - 1; i++ ) {
            newArgumentValues[i] = argumentValues[i];
        }
    }
    newArgumentValues[i] = nullptr;
    errorCode = MgfDefinitions::mgfHandle(EntityContext::FACE, i, newArgumentValues, context);
    if ( errorCode != ErrorCodeContext::MGF_OK ) {
        return errorCode;
    }
    return ErrorCodeContext::MGF_OK;
}
