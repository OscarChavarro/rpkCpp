/**
Hierarchical object names tracking
*/

#include <string.h>

#include "java/util/ArrayList.txx"
#include "numericalAnalysis/MeshSurfaceVisitor.h"
#include "io/context/TokenValidationContext.h"
#include "io/mgf/MgfEntityControl.h"
#include "io/mgf/MgfObjectNameSupport.h"

void
MgfObjectNameSupport::disposeCurrentSurfaceLists(ParseRuntimeContext *context) {
    if ( context->currentPointList != NULL ) {
        for ( int i = 0; i < context->currentPointList->size(); i++ ) {
            delete context->currentPointList->get(i);
        }
        context->currentPointList->dispose();
        delete context->currentPointList;
        context->currentPointList = NULL;
    }
    if ( context->currentNormalList != NULL ) {
        for ( int i = 0; i < context->currentNormalList->size(); i++ ) {
            delete context->currentNormalList->get(i);
        }
        context->currentNormalList->dispose();
        delete context->currentNormalList;
        context->currentNormalList = NULL;
    }
    if ( context->currentVertexList != NULL ) {
        for ( int i = 0; i < context->currentVertexList->size(); i++ ) {
            delete context->currentVertexList->get(i);
        }
        context->currentVertexList->dispose();
        delete context->currentVertexList;
        context->currentVertexList = NULL;
    }
    if ( context->currentFaceList != NULL ) {
        for ( int i = 0; i < context->currentFaceList->size(); i++ ) {
            delete context->currentFaceList->get(i);
        }
        context->currentFaceList->dispose();
        delete context->currentFaceList;
        context->currentFaceList = NULL;
    }
}

void
MgfObjectNameSupport::pushCurrentGeometryList(ParseRuntimeContext *context) {
    if ( context->geometryStackHeadIndex >= GeometryAssemblyContext::MAXIMUM_GEOMETRY_STACK_DEPTH ) {
        MgfEntityControl::doError("Objects are nested too deep for this program. Recompile with larger GeometryAssemblyContext::MAXIMUM_GEOMETRY_STACK_DEPTH constant in read mgf", context);
        return;
    } else {
        context->geometryStack[context->geometryStackHeadIndex] = context->currentGeometryList;
        context->geometryStackHeadIndex++;
        context->currentGeometryList = NULL;
    }
}

void
MgfObjectNameSupport::popCurrentGeometryList(ParseRuntimeContext *context) {
    if ( context->geometryStackHeadIndex < 0 ) {
        MgfEntityControl::doError("Object stack underflow ... unbalanced 'o' contexts?", context);
        context->currentGeometryList = NULL;
        return;
    } else {
        context->geometryStackHeadIndex--;
        context->currentGeometryList = context->geometryStack[context->geometryStackHeadIndex];
    }
}

void
MgfObjectNameSupport::mgfObjectNewSurface(ParseRuntimeContext *context) {
    // Note: lists created here will be transferred to new MeshSurface,
    // should not be deleted from MgfParseSession
    context->currentPointList = new ArrayList<Vector3D *>();
    context->currentNormalList = new ArrayList<Vector3D *>();
    context->currentVertexList = new ArrayList<Vertex *>();
    context->currentFaceList = new ArrayList<Patch *>();
    context->inSurface = true;
}

/**
Handle an object entity statement
*/
int
MgfObjectNameSupport::handleObject2Entity(int ac, const char **av, ParseRuntimeContext *context) {
    if ( ac == 1 ) {
        // Just pop top object
        return context->objectHierarchyState.popName();
    }
    if ( ac != 2 ) {
        return MGF_ERRR_WRNG_NUM_O_ARGMN;
    }
    if ( !TokenValidationContext::isName(av[1]) ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }
    return context->objectHierarchyState.pushName(av[1]);
}

void
MgfObjectNameSupport::mgfObjectSurfaceDone(ParseRuntimeContext *context) {
    if ( context->currentGeometryList == NULL ) {
        context->currentGeometryList = new ArrayList<Geometry *>();
    }

    if ( context->readerContext != NULL ) {
        const char *head = context->readerContext->inputLine;
        if ( head[0] == 'o' && head[1] == ' ' ) {
            char *tail = &context->readerContext->inputLine[2];
            if ( strlen(tail) > 0 ) {
                if ( context->currentObjectName != NULL ) {
                    delete[] context->currentObjectName;
                }
                context->currentObjectName = NULL;
                tail[strlen(tail) - 1] = '\0';
                context->currentObjectName = new char[strlen(tail) + 1];
                strcpy(context->currentObjectName, tail);
            }
        }
    }

    if ( context->currentFaceList != NULL && context->currentFaceList->size() > 0 ) {
        Geometry *newGeometry = new MeshSurface(
            context->currentObjectName,
            context->currentMaterial,
            context->currentPointList,
            context->currentNormalList,
            NULL, // null texture coordinate list
            context->currentVertexList,
            context->currentFaceList,
            NO_COLORS);
        MeshSurfaceVisitor::initializeFacesDefaults(((MeshSurface *)(newGeometry)));
        context->currentGeometryList->add(newGeometry);
        context->allGeometries->add(newGeometry);
        context->currentObjectName = NULL;
        context->currentPointList = NULL;
        context->currentNormalList = NULL;
        context->currentVertexList = NULL;
        context->currentFaceList = NULL;
    } else {
        MgfObjectNameSupport::disposeCurrentSurfaceLists(context);
    }
    context->inSurface = false;
}

int
MgfObjectNameSupport::handleObjectEntity(int argc, const char **argv, ParseRuntimeContext *context) {
    if ( argc > 1 ) {
        // Beginning of a new object
        if ( context->inSurface ) {
            MgfObjectNameSupport::mgfObjectSurfaceDone(context);
        }

        MgfObjectNameSupport::pushCurrentGeometryList(context);

        MgfObjectNameSupport::mgfObjectNewSurface(context);
    } else {
        // End of object definition
        Geometry *newGeometry = NULL;

        if ( context->inSurface ) {
            MgfObjectNameSupport::mgfObjectSurfaceDone(context);
        }

        long listSize = 0;
        if ( context->currentGeometryList != NULL ) {
            listSize += context->currentGeometryList->size();
        }

        if ( listSize > 0 ) {
            newGeometry = new Compound(context->currentGeometryList);
        }

        MgfObjectNameSupport::popCurrentGeometryList(context);

        if ( newGeometry != NULL && context->currentGeometryList ) {
            context->currentGeometryList->add(newGeometry);
            context->geometries = context->currentGeometryList;
            context->allGeometries->add(newGeometry);
            MgfObjectNameSupport::mgfObjectNewSurface(context);
        }
    }

    return MgfObjectNameSupport::handleObject2Entity(argc, argv, context);
}

void
MgfObjectNameSupport::mgfObjectFreeMemory(ParseRuntimeContext *context) {
    if ( context != NULL ) {
        context->objectHierarchyState.clear();
    }
}
