#include "java/util/ArrayList.txx"

#include "io/context/GeometryBuildState.h"

GeometryBuildState::GeometryBuildState():
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

GeometryBuildState::~GeometryBuildState() {
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
