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

class ParseSnapshotContext {
  public:
    // Snapshot of MGF parsing state and outputs (non-owning references).
    ColorContext *currentColor;
    ArrayList<Patch *> *currentFaceList;
    ArrayList<Geometry *> *currentGeometryList;
    char *currentMaterialName;
    ArrayList<Vector3D *> *currentNormalList;
    char *currentObjectName;
    ArrayList<Vector3D *> *currentPointList;
    ArrayList<Vertex *> *currentVertexList;
    char *currentVertexName;
    ArrayList<Geometry *> *geometries;
    int geometryStackHeadIndex;
    bool inComplex;
    bool inSurface;
    ArrayList<Material *> *materials;
    bool monochrome;
    ReaderContext *readerContext;
    TransformStackContext *transformContext;

    ParseSnapshotContext();
};

#endif
