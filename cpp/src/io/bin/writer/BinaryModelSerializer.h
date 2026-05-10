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

    static bool writeBytesChunked(java::OutputStream &output, const unsigned char *data, long long length);
    static void writeTag(java::OutputStream &output, const char tag[4]);
    static bool checkedLongToInt32(long value, const char *what, int &result);
    static bool writeString(java::OutputStream &output, const char *text);
    static void writeColor(java::OutputStream &output, const ColorRgb &color);
    static void writeVector(java::OutputStream &output, const Vector3D &vector);
    static void writeBoundingBox(java::OutputStream &output, const AxisAlignedBoundingBox &boundingBox);

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
        const BinaryModelSerializationGraph &context);
    static void writeTransformArrayRecord(java::OutputStream &output, const TransformSequenceContext *transformArray);
    static bool writeTransformContextRecord(
        java::OutputStream &output,
        const TransformStackContext *transformContext,
        const BinaryModelSerializationGraph &context);
    static bool writeVertexRecord(java::OutputStream &output, const Vertex *vertex, const BinaryModelSerializationGraph &context);
    static bool writePatchRecord(java::OutputStream &output, const Patch *patch, const BinaryModelSerializationGraph &context);
    static bool writeGeometryRecord(java::OutputStream &output, const Geometry *geometry, const BinaryModelSerializationGraph &context);
    static bool writeModelRecord(java::OutputStream &output, const ParseSnapshotContext *model, const BinaryModelSerializationGraph &context);
};

#endif
