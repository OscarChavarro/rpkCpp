#ifndef __BINARY_MODEL_READER__
#define __BINARY_MODEL_READER__

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
class BinaryModelReaderIndexListRecord;
class BinaryModelReaderVertexRecord;
class BinaryModelReaderPatchRecord;
class BinaryModelReaderGeometryRecord;
class BinaryModelReaderModelRecord;

class BinaryModelReader {
  public:
    static PersistedSceneModel *read(const char *fileName);

  private:
    static const unsigned char BINARY_MODEL_MAGIC[16];
    static const int BINARY_MODEL_VERSION;

    template <typename T>
    static bool initializeArrayList(java::ArrayList<T> *list, int count, T initialValue, const char *what);

    static void releaseIndexListRecord(BinaryModelReaderIndexListRecord *record);
    static void readBytes(java::io::InputStream &input, unsigned char *buffer, int length);
    static bool readBytesChunked(java::io::InputStream &input, unsigned char *buffer, long long length);
    static unsigned char readByte(java::io::InputStream &input);
    static bool readBool(java::io::InputStream &input);
    static short readInt16LE(java::io::InputStream &input);
    static int readInt32LE(java::io::InputStream &input);
    static long long readInt64LE(java::io::InputStream &input);
    static float readFloatLE(java::io::InputStream &input);
    static double readDoubleLE(java::io::InputStream &input);
    static bool expectTag(java::io::InputStream &input, const char expected[4]);
    static bool readNonNegativeCount(java::io::InputStream &input, const char *what, int *count);
    static bool readNullableString(java::io::InputStream &input, char **value, bool *hasValue);
    static bool duplicateNullableString(bool hasValue, const char *value, char **text);
    static bool readColor(java::io::InputStream &input, ColorRgb *color);
    static bool readVector(java::io::InputStream &input, Vector3D *vector);
    static bool readBoundingBoxCoordinates(java::io::InputStream &input, float coordinates[6]);
    static bool setBoundingBoxFromCoordinates(BoundingBox *boundingBox, const float coordinates[6]);
    static bool readIndexList(java::io::InputStream &input, const char *what, BinaryModelReaderIndexListRecord *record);

    template <typename T>
    static bool pointerFromIndex(const java::ArrayList<T *> &values, int index, const char *what, T **result);

    template <typename T>
    static bool arrayListFromIndices(
        const BinaryModelReaderIndexListRecord &record,
        const java::ArrayList<T *> &values,
        const char *what,
        java::ArrayList<T *> **result);

    static bool validateBinaryHeader(java::io::InputStream &input);
    static bool populateModelStrings(PersistedSceneModel *model, const BinaryModelReaderModelRecord &record);
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
