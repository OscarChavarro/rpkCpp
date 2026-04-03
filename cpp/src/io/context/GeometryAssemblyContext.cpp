#include "java/util/ArrayList.txx"

#include "io/context/GeometryAssemblyContext.h"

GeometryAssemblyContext::GeometryAssemblyContext():
    currentVertexName(nullptr),
    geometryStackHeadIndex(0),
    geometryStack(),
    currentPointList(nullptr),
    currentNormalList(nullptr),
    currentVertexList(nullptr),
    currentFaceList(nullptr),
    currentGeometryList(nullptr),
    currentObjectName(nullptr),
    inSurface(false),
    inComplex(false),
    warpConeEnds(false),
    allGeometries(new java::ArrayList<Geometry *>()),
    geometries(nullptr)
{
}

GeometryAssemblyContext::~GeometryAssemblyContext() {
    if ( currentObjectName != nullptr ) {
        delete[] currentObjectName;
        currentObjectName = nullptr;
    }
    if ( allGeometries != nullptr ) {
        allGeometries->dispose();
        delete allGeometries;
        allGeometries = nullptr;
    }
}
