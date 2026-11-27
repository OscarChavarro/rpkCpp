#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __GEOMETRY_ASSEMBLY_CONTEXT__
#define __GEOMETRY_ASSEMBLY_CONTEXT__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

class GeometryAssemblyContext {
  public:
    enum{
        MAXIMUM_GEOMETRY_STACK_DEPTH = 100
    };

    char *currentVertexName;
    int geometryStackHeadIndex;
    ArrayList<RpkGeometry *> *geometryStack[MAXIMUM_GEOMETRY_STACK_DEPTH];

    // Those lists are transferred to MeshSurface objects and should not be deleted on transfer.
    ArrayList<Vector3Dd *> *currentPointList;
    ArrayList<Vector3Dd *> *currentNormalList;
    ArrayList<RpkVertex *> *currentVertexList;
    ArrayList<Patch *> *currentFaceList;
    ArrayList<RpkGeometry *> *currentGeometryList;
    char *currentObjectName;
    bool inSurface;
    bool inComplex;
    ArrayList<RpkGeometry *> *allGeometries;

    // Return model geometry output
    ArrayList<RpkGeometry *> *geometries;

    GeometryAssemblyContext();
    ~GeometryAssemblyContext();
};

#endif
