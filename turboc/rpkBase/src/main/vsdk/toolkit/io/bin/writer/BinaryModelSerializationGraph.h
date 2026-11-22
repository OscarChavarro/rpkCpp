#ifndef BNRY_MDL_WRTR_SRLZT_CNTXT
#define BNRY_MDL_WRTR_SRLZT_CNTXT

#include "java/util/ArrayList.h"
#include "java/util/HashMap.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/io/context/ParseSnapshotContext.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/context/TransformSequenceContext.h"
#include "vsdk/toolkit/io/context/TransformStackContext.h"
#include "vsdk/toolkit/material/Material.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/Vertex.h"

class BinaryModelSerializationGraph {
  public:
    HashMap<const Vector3Df *, int> vectorIndices;
    ArrayList<const Vector3Df *> vectors;

    HashMap<const Vertex *, int> vertexIndices;
    ArrayList<const Vertex *> vertices;

    HashMap<const Patch *, int> patchIndices;
    ArrayList<const Patch *> patches;

    HashMap<const Material *, int> materialIndices;
    ArrayList<const Material *> materials;

    HashMap<const Geometry *, int> geometryIndices;
    ArrayList<const Geometry *> geometries;

    HashMap<const ColorContext *, int> colorContextIndices;
    ArrayList<const ColorContext *> colorContexts;

    HashMap<const ReaderContext *, int> readerContextIndices;
    ArrayList<const ReaderContext *> readerContexts;

    HashMap<const TransformSequenceContext *, int> transformArrayIndices;
    ArrayList<const TransformSequenceContext *> transformArrays;

    HashMap<const TransformStackContext *, int> transformContextIndices;
    ArrayList<const TransformStackContext *> transformContexts;

    bool ensureVector(const Vector3Df *value);
    bool ensureMaterial(const Material *value);
    bool ensureVertex(const Vertex *value);
    bool ensurePatch(const Patch *value);
    bool ensureGeometry(const Geometry *value);
    bool ensureColorContext(const ColorContext *value);
    bool ensureReaderContext(const ReaderContext *value);
    bool ensureTransformArray(const TransformSequenceContext *value);
    bool ensureTransformContext(const TransformStackContext *value);

    bool collectVectorList(const ArrayList<Vector3Df *> *list);
    bool collectVertexList(const ArrayList<Vertex *> *list);
    bool collectPatchList(const ArrayList<Patch *> *list);
    bool collectMaterialList(const ArrayList<Material *> *list);
    bool collectGeometryList(const ArrayList<Geometry *> *list);
    bool collectModel(const ParseSnapshotContext *model);
};

#endif
