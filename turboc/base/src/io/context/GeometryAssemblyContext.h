#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __GEOMETRY_BUILD_STATE__
#define __GEOMETRY_BUILD_STATE__

#include "java/util/ArrayList.h"
#include "common/linealAlgebra/Vector3D.h"
#include "skin/Geometry.h"
#include "environment/geometry/elements/Patch.h"
#include "environment/geometry/elements/Vertex.h"

class GeometryAssemblyContext {
  public:
    enum{
        MAXIMUM_GEOMETRY_STACK_DEPTH = 100
    };

    char *currentVertexName;
    int geometryStackHeadIndex;
    ArrayList<Geometry *> *geometryStack[MAXIMUM_GEOMETRY_STACK_DEPTH];

    // Those lists are transferred to MeshSurface objects and should not be deleted on transfer.
    ArrayList<Vector3D *> *currentPointList;
    ArrayList<Vector3D *> *currentNormalList;
    ArrayList<Vertex *> *currentVertexList;
    ArrayList<Patch *> *currentFaceList;
    ArrayList<Geometry *> *currentGeometryList;
    char *currentObjectName;
    bool inSurface;
    bool inComplex;
    bool warpConeEnds;
    ArrayList<Geometry *> *allGeometries;

    // Return model geometry output
    ArrayList<Geometry *> *geometries;

    GeometryAssemblyContext();
    ~GeometryAssemblyContext();
};

#endif
