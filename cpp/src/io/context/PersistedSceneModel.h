#ifndef __PERSISTED_SCENE_MODEL__
#define __PERSISTED_SCENE_MODEL__

#include "java/util/ArrayList.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/context/ColorContext.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformStackContext.h"
#include "material/Material.h"
#include "skin/Geometry.h"
#include "skin/Patch.h"
#include "skin/Vertex.h"

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
