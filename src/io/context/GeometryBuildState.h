#ifndef __GEOMETRY_BUILD_STATE__
#define __GEOMETRY_BUILD_STATE__

class Geometry;
class Patch;
class Vector3D;
class Vertex;

class GeometryBuildState {
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

    GeometryBuildState();
    ~GeometryBuildState();
};

#endif
