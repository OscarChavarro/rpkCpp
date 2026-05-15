#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

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
    static bool initializeArrayList(ArrayList<T> *list, int count, T initialValue, const char *what);

    static void releaseIndexListRecord(BinaryModelIndexListRef *record);
    static void readBytes(InputStream &input, unsigned char *buffer, int length);
    static bool readBytesChunked(InputStream &input, unsigned char *buffer, long length);
    static unsigned char readByte(InputStream &input);
    static bool readBool(InputStream &input);
    static short readInt16LE(InputStream &input);
    static int readInt32LE(InputStream &input);
    static long readInt64LE(InputStream &input);
    static float readFloatLE(InputStream &input);
    static double readDoubleLE(InputStream &input);
    static bool expectTag(InputStream &input, const char expected[4]);
    static bool readNonNegativeCount(InputStream &input, const char *what, int *count);
    static bool readNullableString(InputStream &input, char **value, bool *hasValue);
    static bool duplicateNullableString(bool hasValue, const char *value, char **text);
    static bool readColor(InputStream &input, ColorRgb *color);
    static bool readVector(InputStream &input, Vector3D *vector);
    static bool readBoundingBoxCoordinates(InputStream &input, float coordinates[6]);
    static bool setBoundingBoxFromCoordinates(BoundingBox *boundingBox, const float coordinates[6]);
    static bool readIndexList(InputStream &input, const char *what, BinaryModelIndexListRef *record);

    template <typename T>
    static bool pointerFromIndex(const ArrayList<T *> &values, int index, const char *what, T **result);

    template <typename T>
    static bool arrayListFromIndices(
        const BinaryModelIndexListRef &record,
        const ArrayList<T *> &values,
        const char *what,
        ArrayList<T *> **result);

    static bool validateBinaryHeader(InputStream &input);
    static bool populateModelStrings(ParseSnapshotContext *model, const BinaryModelSnapshotRecordData &record);

  private:
    static const unsigned char BINARY_MODEL_MAGIC[16];
    #define BINARY_MODEL_VERSION 1
};

template <typename T>
inline bool
BinaryModelReadPrimitives::initializeArrayList(ArrayList<T> *list, int count, T initialValue, const char *what) {
    if ( list == NULL ) {
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
BinaryModelReadPrimitives::pointerFromIndex(const ArrayList<T *> &values, int index, const char *what, T **result) {
    if ( result == NULL ) {
        return reportReadError("BinaryModelReadPrimitives::pointerFromIndex", "Null output pointer");
    }
    *result = NULL;
    if ( index == -1 ) {
        return true;
    }
    if ( index < 0 || ((long int)(index)) >= values.size() ) {
        Logger::error("BinaryModelReadPrimitives::pointerFromIndex", "Out of range index while reading binary model (%s)", what);
        return false;
    }
    *result = values.get(((long int)(index)));
    return true;
}

template <typename T>
inline bool
BinaryModelReadPrimitives::arrayListFromIndices(
    const BinaryModelIndexListRef &record,
    const ArrayList<T *> &values,
    const char *what,
    ArrayList<T *> **result)
{
    if ( result == NULL ) {
        return reportReadError("BinaryModelReadPrimitives::arrayListFromIndices", "Null output pointer");
    }
    *result = NULL;
    if ( record.isNull ) {
        return true;
    }
    if ( record.indices == NULL ) {
        return reportReadError("BinaryModelReadPrimitives::arrayListFromIndices", "Missing index list while reading binary model");
    }
    ArrayList<T *> *list = new ArrayList<T *>();
    for ( long int i = 0; i < record.indices->size(); i++ ) {
        T *element = NULL;
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
