#ifndef __BINARY_MODEL_WRITTER__
#define __BINARY_MODEL_WRITTER__

#include "java/util/HashMap.h"

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
class BinaryModelWriterSerializationContext;

class BinaryModelWriter {
  public:
    static bool write(const PersistedSceneModel *model, const char *fileName);

  private:
    static const unsigned char BINARY_MODEL_MAGIC[16];
    static const int BINARY_MODEL_VERSION;
    static const char *safeLabel(const char *text);

    static bool writeBytesChunked(java::OutputStream &output, const unsigned char *data, long long length);
    static void writeTag(java::OutputStream &output, const char tag[4]);
    static bool checkedLongToInt32(long value, const char *what, int &result);
    static bool writeString(java::OutputStream &output, const char *text);
    static void writeColor(java::OutputStream &output, const ColorRgb &color);
    static void writeVector(java::OutputStream &output, const Vector3D &vector);
    static void writeBoundingBox(java::OutputStream &output, const BoundingBox &boundingBox);

    template <typename T>
    static bool indexOfPointer(const T *ptr, const java::HashMap<const T *, int> &indices, const char *what, int &result);

    template <typename T>
    static bool writeIndexList(
        java::OutputStream &output,
        const java::ArrayList<T *> *list,
        const java::HashMap<const T *, int> &indices,
        const char *what);

    static bool writeMaterialRecord(java::OutputStream &output, const Material *material);
    static void writeColorContextRecord(java::OutputStream &output, const ColorContext *colorContext);
    static bool writeReaderContextRecord(
        java::OutputStream &output,
        const ReaderContext *readerContext,
        const BinaryModelWriterSerializationContext &context);
    static void writeTransformArrayRecord(java::OutputStream &output, const TransformArray *transformArray);
    static bool writeTransformContextRecord(
        java::OutputStream &output,
        const TransformStackContext *transformContext,
        const BinaryModelWriterSerializationContext &context);
    static bool writeVertexRecord(java::OutputStream &output, const Vertex *vertex, const BinaryModelWriterSerializationContext &context);
    static bool writePatchRecord(java::OutputStream &output, const Patch *patch, const BinaryModelWriterSerializationContext &context);
    static bool writeGeometryRecord(java::OutputStream &output, const Geometry *geometry, const BinaryModelWriterSerializationContext &context);
    static bool writeModelRecord(java::OutputStream &output, const PersistedSceneModel *model, const BinaryModelWriterSerializationContext &context);
};

#endif
