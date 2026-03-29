#include "java/util/ArrayList.txx"

#include "io/context/GeometryBuildState.h"
#include "io/context/LookUpTable.h"

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
    vertexLookUpTable(new LookUpTable(LookUpBehaviors::owningCString())),
    allGeometries(new java::ArrayList<Geometry *>()),
    geometries(nullptr)
{
}

GeometryBuildState::~GeometryBuildState() {
    if ( currentObjectName != nullptr ) {
        delete[] currentObjectName;
        currentObjectName = nullptr;
    }
    if ( vertexLookUpTable != nullptr ) {
        delete vertexLookUpTable;
        vertexLookUpTable = nullptr;
    }
    if ( allGeometries != nullptr ) {
        allGeometries->dispose();
        delete allGeometries;
        allGeometries = nullptr;
    }
}
