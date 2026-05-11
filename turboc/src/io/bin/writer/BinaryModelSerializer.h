#ifndef __BINARY_MODEL_WRITTER__
#define __BINARY_MODEL_WRITTER__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "java/util/HashMap.h"
#include "common/color/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/bin/writer/BinaryModelSerializationGraph.h"
#include "io/context/ColorContext.h"
#include "io/context/ParseSnapshotContext.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformSequenceContext.h"
#include "io/context/TransformStackContext.h"
#include "material/Material.h"
#include "skin/AxisAlignedBoundingBox.h"
#include "skin/Geometry.h"
#include "environment/geometry/elements/Patch.h"
#include "environment/geometry/elements/Vertex.h"

class BinaryModelSerializer {
  public:
    static bool write(const ParseSnapshotContext *model, const char *fileName);

  private:
    static const unsigned char BINARY_MODEL_MAGIC[16];
    static const int BINARY_MODEL_VERSION;
    static const char *safeLabel(const char *text);

    static bool writeBytesChunked(OutputStream &output, const unsigned char *data, long length);
    static void writeTag(OutputStream &output, const char tag[4]);
    static bool checkedLongToInt32(long value, const char *what, int &result);
    static void writeInt64LE(OutputStream &output, long value);
    static void writeDoubleLE(OutputStream &output, double value);
    static bool writeString(OutputStream &output, const char *text);
    static void writeColor(OutputStream &output, const ColorRgb &color);
    static void writeVector(OutputStream &output, const Vector3D &vector);
    static void writeBoundingBox(OutputStream &output, const BoundingBox &boundingBox);

    template <typename T>
    static bool indexOfPointer(const T *ptr, const HashMap<const T *, int> &indices, const char *what, int &result);

    template <typename T>
    static bool writeIndexList(
        OutputStream &output,
        const ArrayList<T *> *list,
        const HashMap<const T *, int> &indices,
        const char *what);

    static bool writeMaterialRecord(OutputStream &output, const Material *material);
    static void writeColorContextRecord(OutputStream &output, const ColorContext *colorContext);
    static bool writeReaderContextRecord(
        OutputStream &output,
        const ReaderContext *readerContext,
        const BinaryModelSerializationGraph &context);
    static void writeTransformArrayRecord(OutputStream &output, const TransformSequenceContext *transformArray);
    static bool writeTransformContextRecord(
        OutputStream &output,
        const TransformStackContext *transformContext,
        const BinaryModelSerializationGraph &context);
    static bool writeVertexRecord(OutputStream &output, const Vertex *vertex, const BinaryModelSerializationGraph &context);
    static bool writePatchRecord(OutputStream &output, const Patch *patch, const BinaryModelSerializationGraph &context);
    static bool writeGeometryRecord(OutputStream &output, const Geometry *geometry, const BinaryModelSerializationGraph &context);
    static bool writeModelRecord(OutputStream &output, const ParseSnapshotContext *model, const BinaryModelSerializationGraph &context);
};

#endif
