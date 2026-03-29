#ifndef __BINARY_MODEL_WRITER_SERIALIZATION_CONTEXT__
#define __BINARY_MODEL_WRITER_SERIALIZATION_CONTEXT__

namespace java {
template <class T>
class ArrayList;

template <class K, class V>
class HashMap;
}

class ColorContext;
class Geometry;
class Material;
class Patch;
class PersistedSceneModel;
class ReaderContext;
class TransformArray;
class TransformStackContext;
class Vector3D;
class Vertex;

class BinaryModelWriterSerializationContext {
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

    java::HashMap<const TransformArray *, int> transformArrayIndices;
    java::ArrayList<const TransformArray *> transformArrays;

    java::HashMap<const TransformStackContext *, int> transformContextIndices;
    java::ArrayList<const TransformStackContext *> transformContexts;

    bool ensureVector(const Vector3D *value);
    bool ensureMaterial(const Material *value);
    bool ensureVertex(const Vertex *value);
    bool ensurePatch(const Patch *value);
    bool ensureGeometry(const Geometry *value);
    bool ensureColorContext(const ColorContext *value);
    bool ensureReaderContext(const ReaderContext *value);
    bool ensureTransformArray(const TransformArray *value);
    bool ensureTransformContext(const TransformStackContext *value);

    bool collectVectorList(const java::ArrayList<Vector3D *> *list);
    bool collectVertexList(const java::ArrayList<Vertex *> *list);
    bool collectPatchList(const java::ArrayList<Patch *> *list);
    bool collectMaterialList(const java::ArrayList<Material *> *list);
    bool collectGeometryList(const java::ArrayList<Geometry *> *list);
    bool collectModel(const PersistedSceneModel *model);
};

#endif
