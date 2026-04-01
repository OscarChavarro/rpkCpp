#ifndef __BINARY_MODEL_READER_CLEANUP__
#define __BINARY_MODEL_READER_CLEANUP__

class Geometry;
class Material;
class ColorContext;
class PersistedSceneModel;
class ReaderContext;
class TransformArray;
class TransformStackContext;
class Patch;
class Vector3D;
class Vertex;
class BinaryModelReaderVertexRecord;
class BinaryModelReaderGeometryRecord;
class BinaryModelReaderModelRecord;

class BinaryModelReaderCleanup {
  public:
    static void cleanupPartialModel(
        java::ArrayList<Vector3D *> &vectors,
        java::ArrayList<Vertex *> &vertices,
        java::ArrayList<Patch *> &patches,
        java::ArrayList<Material *> &materials,
        java::ArrayList<Geometry *> &geometries,
        java::ArrayList<ColorContext *> &colorContexts,
        java::ArrayList<ReaderContext *> &readerContexts,
        java::ArrayList<TransformArray *> &transformArrays,
        java::ArrayList<TransformStackContext *> &transformContexts,
        PersistedSceneModel *model);
    static void releaseVertexRecordIndexLists(java::ArrayList<BinaryModelReaderVertexRecord> &vertexRecords);
    static void releaseGeometryRecordIndexLists(java::ArrayList<BinaryModelReaderGeometryRecord> &geometryRecords);
    static void releaseModelRecordIndexLists(BinaryModelReaderModelRecord *modelRecord);
};

#endif
