/**
Hierarchical object names tracking
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "numericalAnalysis/MeshSurfaceVisitor.h"
#include "io/context/WordsContext.h"
#include "io/mgf/MgfDefinitions.h"
#include "io/mgf/MgfHandlerObject.h"

void
MgfHandlerObject::disposeCurrentSurfaceLists(MgfParseSession *context) {
    if ( context->currentPointList != nullptr ) {
        for ( int i = 0; i < context->currentPointList->size(); i++ ) {
            delete context->currentPointList->get(i);
        }
        context->currentPointList->dispose();
        delete context->currentPointList;
        context->currentPointList = nullptr;
    }
    if ( context->currentNormalList != nullptr ) {
        for ( int i = 0; i < context->currentNormalList->size(); i++ ) {
            delete context->currentNormalList->get(i);
        }
        context->currentNormalList->dispose();
        delete context->currentNormalList;
        context->currentNormalList = nullptr;
    }
    if ( context->currentVertexList != nullptr ) {
        for ( int i = 0; i < context->currentVertexList->size(); i++ ) {
            delete context->currentVertexList->get(i);
        }
        context->currentVertexList->dispose();
        delete context->currentVertexList;
        context->currentVertexList = nullptr;
    }
    if ( context->currentFaceList != nullptr ) {
        for ( int i = 0; i < context->currentFaceList->size(); i++ ) {
            delete context->currentFaceList->get(i);
        }
        context->currentFaceList->dispose();
        delete context->currentFaceList;
        context->currentFaceList = nullptr;
    }
}

void
MgfHandlerObject::pushCurrentGeometryList(MgfParseSession *context) {
    if ( context->geometryStackHeadIndex >= MAXIMUM_GEOMETRY_STACK_DEPTH ) {
        MgfDefinitions::doError("Objects are nested too deep for this program. Recompile with larger MAXIMUM_GEOMETRY_STACK_DEPTH constant in read mgf", context);
        return;
    } else {
        context->geometryStack[context->geometryStackHeadIndex] = context->currentGeometryList;
        context->geometryStackHeadIndex++;
        context->currentGeometryList = nullptr;
    }
}

void
MgfHandlerObject::popCurrentGeometryList(MgfParseSession *context) {
    if ( context->geometryStackHeadIndex < 0 ) {
        MgfDefinitions::doError("Object stack underflow ... unbalanced 'o' contexts?", context);
        context->currentGeometryList = nullptr;
        return;
    } else {
        context->geometryStackHeadIndex--;
        context->currentGeometryList = context->geometryStack[context->geometryStackHeadIndex];
    }
}

void
MgfHandlerObject::mgfObjectNewSurface(MgfParseSession *context) {
    // Note: lists created here will be transferred to new MeshSurface,
    // should not be deleted from MgfParseSession
    context->currentPointList = new java::ArrayList<Vector3D *>();
    context->currentNormalList = new java::ArrayList<Vector3D *>();
    context->currentVertexList = new java::ArrayList<Vertex *>();
    context->currentFaceList = new java::ArrayList<Patch *>();
    context->inSurface = true;
}

/**
Handle an object entity statement
*/
int
MgfHandlerObject::handleObject2Entity(int ac, const char **av, MgfParseSession *context) {
    if ( ac == 1 ) {
        // Just pop top object
        return context->objectHierarchyState.popName();
    }
    if ( ac != 2 ) {
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( !WordsContext::isName(av[1]) ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    return context->objectHierarchyState.pushName(av[1]);
}

void
MgfHandlerObject::mgfObjectSurfaceDone(MgfParseSession *context) {
    if ( context->currentGeometryList == nullptr ) {
        context->currentGeometryList = new java::ArrayList<Geometry *>();
    }

    if ( context->readerContext != nullptr ) {
        const char *head = context->readerContext->inputLine;
        if ( head[0] == 'o' && head[1] == ' ' ) {
            char *tail = &context->readerContext->inputLine[2];
            if ( strlen(tail) > 0 ) {
                if ( context->currentObjectName != nullptr ) {
                    delete[] context->currentObjectName;
                }
                context->currentObjectName = nullptr;
                tail[strlen(tail) - 1] = '\0';
                context->currentObjectName = new char[strlen(tail) + 1];
                strcpy(context->currentObjectName, tail);
            }
        }
    }

    if ( context->currentFaceList != nullptr && context->currentFaceList->size() > 0 ) {
        Geometry *newGeometry = new MeshSurface(
            context->currentObjectName,
            context->currentMaterial,
            context->currentPointList,
            context->currentNormalList,
            nullptr, // null texture coordinate list
            context->currentVertexList,
            context->currentFaceList,
            MaterialColorFlags::NO_COLORS);
        MeshSurfaceVisitor::initializeFacesDefaults(dynamic_cast<MeshSurface *>(newGeometry));
        context->currentGeometryList->add(newGeometry);
        context->allGeometries->add(newGeometry);
        context->currentObjectName = nullptr;
        context->currentPointList = nullptr;
        context->currentNormalList = nullptr;
        context->currentVertexList = nullptr;
        context->currentFaceList = nullptr;
    } else {
        MgfHandlerObject::disposeCurrentSurfaceLists(context);
    }
    context->inSurface = false;
}

int
MgfHandlerObject::handleObjectEntity(int argc, const char **argv, MgfParseSession *context) {
    if ( argc > 1 ) {
        // Beginning of a new object
        if ( context->inSurface ) {
            MgfHandlerObject::mgfObjectSurfaceDone(context);
        }

        MgfHandlerObject::pushCurrentGeometryList(context);

        MgfHandlerObject::mgfObjectNewSurface(context);
    } else {
        // End of object definition
        Geometry *newGeometry = nullptr;

        if ( context->inSurface ) {
            MgfHandlerObject::mgfObjectSurfaceDone(context);
        }

        long listSize = 0;
        if ( context->currentGeometryList != nullptr ) {
            listSize += context->currentGeometryList->size();
        }

        if ( listSize > 0 ) {
            newGeometry = new Compound(context->currentGeometryList);
        }

        MgfHandlerObject::popCurrentGeometryList(context);

        if ( newGeometry != nullptr && context->currentGeometryList ) {
            context->currentGeometryList->add(newGeometry);
            context->geometries = context->currentGeometryList;
            context->allGeometries->add(newGeometry);
            MgfHandlerObject::mgfObjectNewSurface(context);
        }
    }

    return MgfHandlerObject::handleObject2Entity(argc, argv, context);
}

void
MgfHandlerObject::mgfObjectFreeMemory(MgfParseSession *context) {
    if ( context != nullptr ) {
        context->objectHierarchyState.clear();
    }
}
