/**
Hierarchical object names tracking
*/

#include <cstring>

#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/numericalAnalysis/MeshSurfaceVisitor.h"
#include "vsdk/toolkit/io/context/TokenValidationContext.h"
#include "vsdk/toolkit/io/mgf/MgfEntityControl.h"
#include "vsdk/toolkit/io/mgf/MgfObjectNameSupport.h"

void
MgfObjectNameSupport::disposeCurrentSurfaceLists(ParseRuntimeContext *context) {
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
MgfObjectNameSupport::pushCurrentGeometryList(ParseRuntimeContext *context) {
    if ( context->geometryStackHeadIndex >= GeometryAssemblyContext::MAXIMUM_GEOMETRY_STACK_DEPTH ) {
        MgfEntityControl::doError("Objects are nested too deep for this program. Recompile with larger GeometryAssemblyContext::MAXIMUM_GEOMETRY_STACK_DEPTH constant in read mgf", context);
        return;
    } else {
        context->geometryStack[context->geometryStackHeadIndex] = context->currentGeometryList;
        context->geometryStackHeadIndex++;
        context->currentGeometryList = nullptr;
    }
}

void
MgfObjectNameSupport::popCurrentGeometryList(ParseRuntimeContext *context) {
    if ( context->geometryStackHeadIndex < 0 ) {
        MgfEntityControl::doError("Object stack underflow ... unbalanced 'o' contexts?", context);
        context->currentGeometryList = nullptr;
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
MgfObjectNameSupport::handleObject2Entity(int ac, const char **av, ParseRuntimeContext *context) {
    if ( ac == 1 ) {
        // Just pop top object
        return context->objectHierarchyState.popName();
    }
    if ( ac != 2 ) {
        return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( !TokenValidationContext::isName(av[1]) ) {
        return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    return context->objectHierarchyState.pushName(av[1]);
}

void
MgfObjectNameSupport::mgfObjectSurfaceDone(ParseRuntimeContext *context) {
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
        Geometry *newGeometry = nullptr;

        if ( context->inSurface ) {
            MgfObjectNameSupport::mgfObjectSurfaceDone(context);
        }

        long listSize = 0;
        if ( context->currentGeometryList != nullptr ) {
            listSize += context->currentGeometryList->size();
        }

        if ( listSize > 0 ) {
            newGeometry = new Compound(context->currentGeometryList);
        }

        MgfObjectNameSupport::popCurrentGeometryList(context);

        if ( newGeometry != nullptr && context->currentGeometryList ) {
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
    if ( context != nullptr ) {
        context->objectHierarchyState.clear();
    }
}
