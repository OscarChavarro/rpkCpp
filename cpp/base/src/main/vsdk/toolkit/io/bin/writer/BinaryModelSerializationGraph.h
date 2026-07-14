#ifndef BINARY_MODEL_WRITER_SERIALIZATION_CONTEXT__
#define BINARY_MODEL_WRITER_SERIALIZATION_CONTEXT__

#include "java/util/ArrayList.h"
#include "java/util/HashMap.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
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
    java::HashMap<const Vector3D *, int> vectorIndices;
    java::ArrayList<const Vector3D *> vectors;

    java::HashMap<const Vertex *, int> vertexIndices;
    java::ArrayList<const Vertex *> vertices;

    java::HashMap<const Patch *, int> patchIndices;
    java::ArrayList<const Patch *> patches;

    java::HashMap<const Material *, int> materialIndices;
    java::ArrayList<const Material *> materials;

    java::HashMap<const Geometry *, int> geometryIndices;
    java::ArrayList<const Geometry *> geometries;

    java::HashMap<const ColorContext *, int> colorContextIndices;
    java::ArrayList<const ColorContext *> colorContexts;

    java::HashMap<const ReaderContext *, int> readerContextIndices;
    java::ArrayList<const ReaderContext *> readerContexts;

    java::HashMap<const TransformSequenceContext *, int> transformArrayIndices;
    java::ArrayList<const TransformSequenceContext *> transformArrays;

    java::HashMap<const TransformStackContext *, int> transformContextIndices;
    java::ArrayList<const TransformStackContext *> transformContexts;

    bool ensureVector(const Vector3D *value);
    bool ensureMaterial(const Material *value);
    bool ensureVertex(const Vertex *value);
    bool ensurePatch(const Patch *value);
    bool ensureGeometry(const Geometry *value);
    bool ensureColorContext(const ColorContext *value);
    bool ensureReaderContext(const ReaderContext *value);
    bool ensureTransformArray(const TransformSequenceContext *value);
    bool ensureTransformContext(const TransformStackContext *value);

    bool collectVectorList(const java::ArrayList<Vector3D *> *list);
    bool collectVertexList(const java::ArrayList<Vertex *> *list);
    bool collectPatchList(const java::ArrayList<Patch *> *list);
    bool collectMaterialList(const java::ArrayList<Material *> *list);
    bool collectGeometryList(const java::ArrayList<Geometry *> *list);
    bool collectModel(const ParseSnapshotContext *model);
};

#endif
