#include "java/util/ArrayList.txx"

#include "io/context/GeometryAssemblyContext.h"

GeometryAssemblyContext::GeometryAssemblyContext():
    currentVertexName(NULL),
    geometryStackHeadIndex(0),
    geometryStack(),
    currentPointList(NULL),
    currentNormalList(NULL),
    currentVertexList(NULL),
    currentFaceList(NULL),
    currentGeometryList(NULL),
    currentObjectName(NULL),
    inSurface(false),
    inComplex(false),
    warpConeEnds(false),
    allGeometries(new ArrayList<Geometry *>()),
    geometries(NULL)
{
}

GeometryAssemblyContext::~GeometryAssemblyContext() {
    if ( currentObjectName != NULL ) {
        delete[] currentObjectName;
        currentObjectName = NULL;
    }
    if ( allGeometries != NULL ) {
        allGeometries->dispose();
        delete allGeometries;
        allGeometries = NULL;
    }
}
