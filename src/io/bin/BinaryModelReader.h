#ifndef __BINARY_MODEL_READER__
#define __BINARY_MODEL_READER__

#include <cstdint>

namespace java {
template <class T>
class ArrayList;

namespace io {
class InputStream;
}
}

class BoundingBox;
class ColorRgb;
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

class BinaryModelReader {
  public:
    static PersistedSceneModel *read(const char *fileName);

  private:
    static const unsigned char BINARY_MODEL_MAGIC[16];
    static const int32_t BINARY_MODEL_VERSION;

    class IndexListRecord;
    class VertexRecord;
    class PatchRecord;
    class GeometryRecord;
    class ModelRecord;

    template <typename T>
    static bool initializeArrayList(java::ArrayList<T> *list, int32_t count, T initialValue, const char *what);

    static void releaseIndexListRecord(IndexListRecord *record);
    static void readBytes(java::io::InputStream &input, unsigned char *buffer, int length);
    static bool readBytesChunked(java::io::InputStream &input, unsigned char *buffer, int64_t length);
    static unsigned char readByte(java::io::InputStream &input);
    static bool readBool(java::io::InputStream &input);
    static int16_t readInt16LE(java::io::InputStream &input);
    static int32_t readInt32LE(java::io::InputStream &input);
    static int64_t readInt64LE(java::io::InputStream &input);
    static float readFloatLE(java::io::InputStream &input);
    static double readDoubleLE(java::io::InputStream &input);
    static bool expectTag(java::io::InputStream &input, const char expected[4]);
    static bool readNonNegativeCount(java::io::InputStream &input, const char *what, int32_t *count);
    static bool readNullableString(java::io::InputStream &input, char **value, bool *hasValue);
    static bool duplicateNullableString(bool hasValue, const char *value, char **text);
    static bool readColor(java::io::InputStream &input, ColorRgb *color);
    static bool readVector(java::io::InputStream &input, Vector3D *vector);
    static bool readBoundingBoxCoordinates(java::io::InputStream &input, float coordinates[6]);
    static bool setBoundingBoxFromCoordinates(BoundingBox *boundingBox, const float coordinates[6]);
    static bool readIndexList(java::io::InputStream &input, const char *what, IndexListRecord *record);

    template <typename T>
    static bool pointerFromIndex(const java::ArrayList<T *> &values, int32_t index, const char *what, T **result);

    template <typename T>
    static bool arrayListFromIndices(
        const IndexListRecord &record,
        const java::ArrayList<T *> &values,
        const char *what,
        java::ArrayList<T *> **result);

    static bool validateBinaryHeader(java::io::InputStream &input);
    static bool populateModelStrings(PersistedSceneModel *model, const ModelRecord &record);
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
    static void releaseVertexRecordIndexLists(java::ArrayList<VertexRecord> &vertexRecords);
    static void releaseGeometryRecordIndexLists(java::ArrayList<GeometryRecord> &geometryRecords);
    static void releaseModelRecordIndexLists(ModelRecord *modelRecord);
};

#endif
