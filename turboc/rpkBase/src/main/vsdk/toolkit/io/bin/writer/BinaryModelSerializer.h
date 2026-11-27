#ifndef __BINARY_MODEL_SERIALIZER__
#define __BINARY_MODEL_SERIALIZER__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "java/util/HashMap.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/io/bin/writer/BinaryModelSerializationGraph.h"
#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/io/context/ParseSnapshotContext.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/context/TransformSequenceContext.h"
#include "vsdk/toolkit/io/context/TransformStackContext.h"
#include "vsdk/toolkit/material/RpkMaterial.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

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
    static void writeVector(OutputStream &output, const Vector3Dd &vector);
    static void writeBoundingBox(OutputStream &output, const RpkAxisAlignedBoundingBox &boundingBox);

    template <typename T>
    static bool indexOfPointer(const T *ptr, const HashMap<const T *, int> &indices, const char *what, int &result);

    template <typename T>
    static bool writeIndexList(
        OutputStream &output,
        const ArrayList<T *> *list,
        const HashMap<const T *, int> &indices,
        const char *what);

    static bool writeMaterialRecord(OutputStream &output, const RpkMaterial *material);
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
    static bool writeVertexRecord(OutputStream &output, const RpkVertex *vertex, const BinaryModelSerializationGraph &context);
    static bool writePatchRecord(OutputStream &output, const Patch *patch, const BinaryModelSerializationGraph &context);
    static bool writeGeometryRecord(OutputStream &output, const RpkGeometry *geometry, const BinaryModelSerializationGraph &context);
    static bool writeModelRecord(OutputStream &output, const ParseSnapshotContext *model, const BinaryModelSerializationGraph &context);
};

#endif
