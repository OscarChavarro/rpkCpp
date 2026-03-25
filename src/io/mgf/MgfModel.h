#ifndef __MGF_MODEL__
#define __MGF_MODEL__

#include "java/util/ArrayList.h"

class Geometry;
class Material;
class Patch;
class Vector3D;
class Vertex;
class MgfColorContext;
class MgfReaderContext;
class MgfTransformContext;

class MgfModel {
  public:
    // Snapshot of MGF parsing state and outputs (non-owning references).
    MgfColorContext *currentColor;
    java::ArrayList<Patch *> *currentFaceList;
    java::ArrayList<Geometry *> *currentGeometryList;
    char *currentMaterialName;
    java::ArrayList<Vector3D *> *currentNormalList;
    char *currentObjectName;
    java::ArrayList<Vector3D *> *currentPointList;
    java::ArrayList<Vertex *> *currentVertexList;
    char *currentVertexName;
    java::ArrayList<Geometry *> *geometries;
    int geometryStackHeadIndex;
    bool inComplex;
    bool inSurface;
    java::ArrayList<Material *> *materials;
    bool monochrome;
    MgfReaderContext *readerContext;
    MgfTransformContext *transformContext;

    MgfModel():
        currentColor(nullptr),
        currentFaceList(nullptr),
        currentGeometryList(nullptr),
        currentMaterialName(nullptr),
        currentNormalList(nullptr),
        currentObjectName(nullptr),
        currentPointList(nullptr),
        currentVertexList(nullptr),
        currentVertexName(nullptr),
        geometries(nullptr),
        geometryStackHeadIndex(0),
        inComplex(false),
        inSurface(false),
        materials(nullptr),
        monochrome(false),
        readerContext(nullptr),
        transformContext(nullptr)
    {
    }
};

#endif
