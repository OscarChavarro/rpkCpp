#ifndef __BINARY_MODEL_WRITTER__
#define __BINARY_MODEL_WRITTER__

#include <cstdint>

namespace java {
template <class T>
class ArrayList;

template <class K, class V>
class HashMap;

namespace io {
class OutputStream;
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
class TransformContext;
class Patch;
class Vector3D;
class Vertex;

class BinaryModelWriter {
  public:
    static bool write(const PersistedSceneModel *model, const char *fileName);

  private:
    static const unsigned char BINARY_MODEL_MAGIC[16];
    static const int32_t BINARY_MODEL_VERSION;

    class SerializationContext;

    static void writeBytesChunked(java::io::OutputStream &output, const unsigned char *data, int64_t length);
    static void writeTag(java::io::OutputStream &output, const char tag[4]);
    static int32_t checkedLongToInt32(long value, const char *what);
    static void writeString(java::io::OutputStream &output, const char *text);
    static void writeColor(java::io::OutputStream &output, const ColorRgb &color);
    static void writeVector(java::io::OutputStream &output, const Vector3D &vector);
    static void writeBoundingBox(java::io::OutputStream &output, const BoundingBox &boundingBox);

    template <typename T>
    static int32_t indexOfPointer(const T *ptr, const java::HashMap<const T *, int> &indices, const char *what);

    template <typename T>
    static void writeIndexList(
        java::io::OutputStream &output,
        const java::ArrayList<T *> *list,
        const java::HashMap<const T *, int> &indices,
        const char *what);

    static void writeMaterialRecord(java::io::OutputStream &output, const Material *material);
    static void writeColorContextRecord(java::io::OutputStream &output, const ColorContext *colorContext);
    static void writeReaderContextRecord(
        java::io::OutputStream &output,
        const ReaderContext *readerContext,
        const SerializationContext &context);
    static void writeTransformArrayRecord(java::io::OutputStream &output, const TransformArray *transformArray);
    static void writeTransformContextRecord(
        java::io::OutputStream &output,
        const TransformContext *transformContext,
        const SerializationContext &context);
    static void writeVertexRecord(java::io::OutputStream &output, const Vertex *vertex, const SerializationContext &context);
    static void writePatchRecord(java::io::OutputStream &output, const Patch *patch, const SerializationContext &context);
    static void writeGeometryRecord(java::io::OutputStream &output, const Geometry *geometry, const SerializationContext &context);
    static void writeModelRecord(java::io::OutputStream &output, const PersistedSceneModel *model, const SerializationContext &context);
};

#endif
