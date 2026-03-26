#ifndef __PERSISTED_SCENE_MODEL__
#define __PERSISTED_SCENE_MODEL__

#include "java/util/ArrayList.h"

class Geometry;
class Material;
class Patch;
class Vector3D;
class Vertex;
class ColorContext;
class ReaderContext;
class TransformStackContext;

class PersistedSceneModel {
  public:
    // Snapshot of MGF parsing state and outputs (non-owning references).
    ColorContext *currentColor;
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
    ReaderContext *readerContext;
    TransformStackContext *transformContext;

    PersistedSceneModel();
};

#endif
