#ifndef __BINARY_MODEL_WRITTER__
#define __BINARY_MODEL_WRITTER__

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

    static bool writeBytesChunked(java::io::OutputStream &output, const unsigned char *data, long long length);
    static void writeTag(java::io::OutputStream &output, const char tag[4]);
    static bool checkedLongToInt32(long value, const char *what, int &result);
    static bool writeString(java::io::OutputStream &output, const char *text);
    static void writeColor(java::io::OutputStream &output, const ColorRgb &color);
    static void writeVector(java::io::OutputStream &output, const Vector3D &vector);
    static void writeBoundingBox(java::io::OutputStream &output, const BoundingBox &boundingBox);

    template <typename T>
    static bool indexOfPointer(const T *ptr, const java::HashMap<const T *, int> &indices, const char *what, int &result);

    template <typename T>
    static bool writeIndexList(
        java::io::OutputStream &output,
        const java::ArrayList<T *> *list,
        const java::HashMap<const T *, int> &indices,
        const char *what);

    static bool writeMaterialRecord(java::io::OutputStream &output, const Material *material);
    static void writeColorContextRecord(java::io::OutputStream &output, const ColorContext *colorContext);
    static bool writeReaderContextRecord(
        java::io::OutputStream &output,
        const ReaderContext *readerContext,
        const BinaryModelWriterSerializationContext &context);
    static void writeTransformArrayRecord(java::io::OutputStream &output, const TransformArray *transformArray);
    static bool writeTransformContextRecord(
        java::io::OutputStream &output,
        const TransformStackContext *transformContext,
        const BinaryModelWriterSerializationContext &context);
    static bool writeVertexRecord(java::io::OutputStream &output, const Vertex *vertex, const BinaryModelWriterSerializationContext &context);
    static bool writePatchRecord(java::io::OutputStream &output, const Patch *patch, const BinaryModelWriterSerializationContext &context);
    static bool writeGeometryRecord(java::io::OutputStream &output, const Geometry *geometry, const BinaryModelWriterSerializationContext &context);
    static bool writeModelRecord(java::io::OutputStream &output, const PersistedSceneModel *model, const BinaryModelWriterSerializationContext &context);
};

#endif
