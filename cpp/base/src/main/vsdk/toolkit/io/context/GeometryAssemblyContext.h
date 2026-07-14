#ifndef GEOMETRY_BUILD_STATE__
#define GEOMETRY_BUILD_STATE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/Vertex.h"

class GeometryAssemblyContext {
  public:
    static constexpr int MAXIMUM_GEOMETRY_STACK_DEPTH = 100;

    char *currentVertexName;
    int geometryStackHeadIndex;
    java::ArrayList<Geometry *> *geometryStack[MAXIMUM_GEOMETRY_STACK_DEPTH];

    // Those lists are transferred to MeshSurface objects and should not be deleted on transfer.
    java::ArrayList<Vector3D *> *currentPointList;
    java::ArrayList<Vector3D *> *currentNormalList;
    java::ArrayList<Vertex *> *currentVertexList;
    java::ArrayList<Patch *> *currentFaceList;
    java::ArrayList<Geometry *> *currentGeometryList;
    char *currentObjectName;
    bool inSurface;
    bool inComplex;
    bool warpConeEnds;
    java::ArrayList<Geometry *> *allGeometries;

    // Return model geometry output
    java::ArrayList<Geometry *> *geometries;

    GeometryAssemblyContext();
    ~GeometryAssemblyContext();
};

#endif
