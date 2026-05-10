#ifndef __BINARY_MODEL_READER_SUPPORT__
#define __BINARY_MODEL_READER_SUPPORT__

#include "common/logging/Logger.h"
#include "common/color/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/bin/reader/BinaryModelIndexListRef.h"
#include "io/bin/reader/BinaryModelSnapshotRecordData.h"
#include "io/context/ParseSnapshotContext.h"
#include "skin/AxisAlignedBoundingBox.h"

class BinaryModelReadPrimitives {
  public:
    static bool reportReadError(const char *routine, const char *message);

    template <typename T>
    static bool initializeArrayList(java::ArrayList<T> *list, int count, T initialValue, const char *what);

    static void releaseIndexListRecord(BinaryModelIndexListRef *record);
    static void readBytes(java::InputStream &input, unsigned char *buffer, int length);
    static bool readBytesChunked(java::InputStream &input, unsigned char *buffer, long long length);
    static unsigned char readByte(java::InputStream &input);
    static bool readBool(java::InputStream &input);
    static short readInt16LE(java::InputStream &input);
    static int readInt32LE(java::InputStream &input);
    static long long readInt64LE(java::InputStream &input);
    static float readFloatLE(java::InputStream &input);
    static double readDoubleLE(java::InputStream &input);
    static bool expectTag(java::InputStream &input, const char expected[4]);
    static bool readNonNegativeCount(java::InputStream &input, const char *what, int *count);
    static bool readNullableString(java::InputStream &input, char **value, bool *hasValue);
    static bool duplicateNullableString(bool hasValue, const char *value, char **text);
    static bool readColor(java::InputStream &input, ColorRgb *color);
    static bool readVector(java::InputStream &input, Vector3D *vector);
    static bool readBoundingBoxCoordinates(java::InputStream &input, float coordinates[6]);
    static bool setBoundingBoxFromCoordinates(AxisAlignedBoundingBox *boundingBox, const float coordinates[6]);
    static bool readIndexList(java::InputStream &input, const char *what, BinaryModelIndexListRef *record);

    template <typename T>
    static bool pointerFromIndex(const java::ArrayList<T *> &values, int index, const char *what, T **result);

    template <typename T>
    static bool arrayListFromIndices(
        const BinaryModelIndexListRef &record,
        const java::ArrayList<T *> &values,
        const char *what,
        java::ArrayList<T *> **result);

    static bool validateBinaryHeader(java::InputStream &input);
    static bool populateModelStrings(ParseSnapshotContext *model, const BinaryModelSnapshotRecordData &record);

  private:
    static const unsigned char BINARY_MODEL_MAGIC[16];
    static constexpr int BINARY_MODEL_VERSION = 1;
};

template <typename T>
inline bool
BinaryModelReadPrimitives::initializeArrayList(java::ArrayList<T> *list, int count, T initialValue, const char *what) {
    if ( list == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::initializeArrayList", "Null list pointer");
    }
    if ( count < 0 ) {
        Logger::error("BinaryModelReadPrimitives::initializeArrayList", "Negative count while reading binary model (%s)", what);
        return false;
    }
    for ( int i = 0; i < count; i++ ) {
        if ( !list->add(initialValue) ) {
            Logger::error("BinaryModelReadPrimitives::initializeArrayList", "Failed to allocate entries while reading binary model (%s)", what);
            return false;
        }
    }
    return true;
}

template <typename T>
inline bool
BinaryModelReadPrimitives::pointerFromIndex(const java::ArrayList<T *> &values, int index, const char *what, T **result) {
    if ( result == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::pointerFromIndex", "Null output pointer");
    }
    *result = nullptr;
    if ( index == -1 ) {
        return true;
    }
    if ( index < 0 || static_cast<long int>(index) >= values.size() ) {
        Logger::error("BinaryModelReadPrimitives::pointerFromIndex", "Out of range index while reading binary model (%s)", what);
        return false;
    }
    *result = values.get(static_cast<long int>(index));
    return true;
}

template <typename T>
inline bool
BinaryModelReadPrimitives::arrayListFromIndices(
    const BinaryModelIndexListRef &record,
    const java::ArrayList<T *> &values,
    const char *what,
    java::ArrayList<T *> **result)
{
    if ( result == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::arrayListFromIndices", "Null output pointer");
    }
    *result = nullptr;
    if ( record.isNull ) {
        return true;
    }
    if ( record.indices == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::arrayListFromIndices", "Missing index list while reading binary model");
    }
    java::ArrayList<T *> *list = new java::ArrayList<T *>();
    for ( long int i = 0; i < record.indices->size(); i++ ) {
        T *element = nullptr;
        if ( !pointerFromIndex(values, record.indices->get(i), what, &element) ) {
            delete list;
            return false;
        }
        if ( !list->add(element) ) {
            delete list;
            return reportReadError("BinaryModelReadPrimitives::arrayListFromIndices", "Failed to allocate output list while reading binary model");
        }
    }
    *result = list;
    return true;
}

#endif
