#ifndef __GEOMETRY_ASSEMBLY_CONTEXT__
#define __GEOMETRY_ASSEMBLY_CONTEXT__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

class GeometryAssemblyContext {
  public:
    static constexpr int MAXIMUM_GEOMETRY_STACK_DEPTH = 100;

    char *currentVertexName;
    int geometryStackHeadIndex;
    java::ArrayList<RpkGeometry *> *geometryStack[MAXIMUM_GEOMETRY_STACK_DEPTH];

    // Those lists are transferred to MeshSurface objects and should not be deleted on transfer.
    java::ArrayList<Vector3Df *> *currentPointList;
    java::ArrayList<Vector3Df *> *currentNormalList;
    java::ArrayList<RpkVertex *> *currentVertexList;
    java::ArrayList<Patch *> *currentFaceList;
    java::ArrayList<RpkGeometry *> *currentGeometryList;
    char *currentObjectName;
    bool inSurface;
    bool inComplex;
    java::ArrayList<RpkGeometry *> *allGeometries;

    // Return model geometry output
    java::ArrayList<RpkGeometry *> *geometries;

    GeometryAssemblyContext();
    ~GeometryAssemblyContext();
};

#endif
