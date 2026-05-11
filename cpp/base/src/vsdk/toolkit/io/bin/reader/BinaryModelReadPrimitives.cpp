#include <cstring>

#include "vsdk/toolkit/java/lang/Integer.h"
#include "vsdk/toolkit/java/util/ArrayList.txx"

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/skin/MinMaxBox.h"
#include "vsdk/toolkit/io/context/ParseSnapshotContext.h"
#include "vsdk/toolkit/io/wrapper/PersistenceElement.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelIndexListRef.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelSnapshotRecordData.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelReadPrimitives.h"

const unsigned char BinaryModelReadPrimitives::BINARY_MODEL_MAGIC[16] = {
    'R', 'P', 'K', '_', 'M', 'G', 'F', '_',
    'B', 'I', 'N', '_', '1', 0, 0, 0
};

bool
BinaryModelReadPrimitives::reportReadError(const char *routine, const char *message) {
    Logger::error(routine, "%s", message);
    return false;
}

void
BinaryModelReadPrimitives::releaseIndexListRecord(BinaryModelIndexListRef *record) {
    if ( record == nullptr ) {
        return;
    }
    if ( record->indices != nullptr ) {
        delete record->indices;
        record->indices = nullptr;
    }
    record->isNull = true;
}

void
BinaryModelReadPrimitives::readBytes(java::InputStream &input, unsigned char *buffer, int length) {
    if ( length <= 0 ) {
        return;
    }
    vsdk::PersistenceElement::readBytes(input, buffer, length);
}

bool
BinaryModelReadPrimitives::readBytesChunked(java::InputStream &input, unsigned char *buffer, long long length) {
    if ( length <= 0 ) {
        return true;
    }

    long long offset = 0;
    const long long maxChunk = static_cast<long long>(java::Integer::MAX_VALUE);
    while ( offset < length ) {
        const long long remaining = length - offset;
        const int chunk = static_cast<int>(remaining < maxChunk ? remaining : maxChunk);
        readBytes(input, buffer + offset, chunk);
        offset += chunk;
    }
    return true;
}

unsigned char
BinaryModelReadPrimitives::readByte(java::InputStream &input) {
    return static_cast<unsigned char>(vsdk::PersistenceElement::readByteUnsignedInt(input));
}

bool
BinaryModelReadPrimitives::readBool(java::InputStream &input) {
    return readByte(input) != 0;
}

short
BinaryModelReadPrimitives::readInt16LE(java::InputStream &input) {
    return static_cast<short>(vsdk::PersistenceElement::readSignedShortLE(input));
}

int
BinaryModelReadPrimitives::readInt32LE(java::InputStream &input) {
    const long value = vsdk::PersistenceElement::readLongLE(input);
    return static_cast<int>(value);
}

long long
BinaryModelReadPrimitives::readInt64LE(java::InputStream &input) {
    unsigned char bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    readBytes(input, bytes, 8);
    unsigned long long value = 0;
    for ( int i = 0; i < 8; i++ ) {
        value |= static_cast<unsigned long long>(bytes[i]) << (8 * i);
    }
    return static_cast<long long>(value);
}

float
BinaryModelReadPrimitives::readFloatLE(java::InputStream &input) {
    return vsdk::PersistenceElement::readFloatLE(input);
}

double
BinaryModelReadPrimitives::readDoubleLE(java::InputStream &input) {
    return vsdk::PersistenceElement::readDoubleLE(input);
}

bool
BinaryModelReadPrimitives::expectTag(java::InputStream &input, const char expected[4]) {
    unsigned char tag[4] = {0, 0, 0, 0};
    readBytes(input, tag, 4);
    if ( tag[0] != static_cast<unsigned char>(expected[0])
         || tag[1] != static_cast<unsigned char>(expected[1])
         || tag[2] != static_cast<unsigned char>(expected[2])
         || tag[3] != static_cast<unsigned char>(expected[3]) ) {
        return reportReadError("BinaryModelReadPrimitives::expectTag", "Unexpected section tag while reading binary model");
    }
    return true;
}

bool
BinaryModelReadPrimitives::readNonNegativeCount(java::InputStream &input, const char *what, int *count) {
    if ( count == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::readNonNegativeCount", "Null output count pointer");
    }
    *count = readInt32LE(input);
    if ( *count < 0 ) {
        Logger::error("BinaryModelReadPrimitives::readNonNegativeCount", "Negative count while reading binary model (%s)", what);
        return false;
    }
    return true;
}

bool
BinaryModelReadPrimitives::readNullableString(java::InputStream &input, char **value, bool *hasValue) {
    if ( value == nullptr || hasValue == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::readNullableString", "Null string output pointer");
    }
    if ( *value != nullptr ) {
        delete[] *value;
        *value = nullptr;
    }
    *hasValue = false;

    const int size = readInt32LE(input);
    if ( size == -1 ) {
        return true;
    }
    if ( size < -1 ) {
        return reportReadError("BinaryModelReadPrimitives::readNullableString", "Invalid negative string size");
    }

    char *text = new char[static_cast<size_t>(size) + 1];
    if ( size > 0 ) {
        readBytes(
            input,
            reinterpret_cast<unsigned char *>(text),
            size);
    }
    text[size] = '\0';
    *value = text;
    *hasValue = true;
    return true;
}

bool
BinaryModelReadPrimitives::duplicateNullableString(bool hasValue, const char *value, char **text) {
    if ( text == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::duplicateNullableString", "Null output string pointer");
    }
    *text = nullptr;
    if ( !hasValue || value == nullptr ) {
        return true;
    }
    const size_t length = std::strlen(value);
    *text = new char[length + 1];
    std::memcpy(*text, value, length + 1);
    return true;
}

bool
BinaryModelReadPrimitives::readColor(java::InputStream &input, ColorRgb *color) {
    if ( color == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::readColor", "Null color output pointer");
    }
    color->r = readFloatLE(input);
    color->g = readFloatLE(input);
    color->b = readFloatLE(input);
    return true;
}

bool
BinaryModelReadPrimitives::readVector(java::InputStream &input, Vector3D *vector) {
    if ( vector == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::readVector", "Null vector output pointer");
    }
    vector->x = readFloatLE(input);
    vector->y = readFloatLE(input);
    vector->z = readFloatLE(input);
    return true;
}

bool
BinaryModelReadPrimitives::readBoundingBoxCoordinates(java::InputStream &input, float coordinates[6]) {
    if ( coordinates == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::readBoundingBoxCoordinates", "Null bounding box coordinate buffer");
    }
    for ( int i = 0; i < 6; i++ ) {
        coordinates[i] = readFloatLE(input);
    }
    return true;
}

bool
BinaryModelReadPrimitives::setBoundingBoxFromCoordinates(AxisAlignedBoundingBox *boundingBox, const float coordinates[6]) {
    if ( boundingBox == nullptr || coordinates == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::setBoundingBoxFromCoordinates", "Invalid bounding box assignment");
    }
    AxisAlignedBoundingBox parsed;
    Vector3D minPoint;
    Vector3D maxPoint;
    minPoint.set(coordinates[MIN_X], coordinates[MIN_Y], coordinates[MIN_Z]);
    maxPoint.set(coordinates[MAX_X], coordinates[MAX_Y], coordinates[MAX_Z]);
    parsed.enlargeToIncludePoint(&minPoint);
    parsed.enlargeToIncludePoint(&maxPoint);
    boundingBox->copyFrom(&parsed);
    return true;
}

bool
BinaryModelReadPrimitives::readIndexList(java::InputStream &input, const char *what, BinaryModelIndexListRef *record) {
    if ( record == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::readIndexList", "Null output record");
    }
    record->isNull = false;
    record->indices = nullptr;

    const int count = readInt32LE(input);
    if ( count == -1 ) {
        record->isNull = true;
        return true;
    }
    if ( count < -1 ) {
        Logger::error("BinaryModelReadPrimitives::readIndexList", "Negative index list count while reading binary model (%s)", what);
        return false;
    }

    record->indices = new java::ArrayList<int>(count > 0 ? static_cast<long int>(count) : 1);
    for ( int i = 0; i < count; i++ ) {
        if ( !record->indices->add(readInt32LE(input)) ) {
            delete record->indices;
            record->indices = nullptr;
            Logger::error("BinaryModelReadPrimitives::readIndexList", "Failed to allocate index list while reading binary model (%s)", what);
            return false;
        }
    }

    return true;
}

bool
BinaryModelReadPrimitives::validateBinaryHeader(java::InputStream &input) {
    unsigned char magic[16] = {0};
    readBytes(input, magic, 16);
    if ( std::memcmp(magic, BINARY_MODEL_MAGIC, 16) != 0 ) {
        return reportReadError("BinaryModelReadPrimitives::validateBinaryHeader", "Invalid binary model magic header");
    }

    const int version = readInt32LE(input);
    if ( version != BINARY_MODEL_VERSION ) {
        return reportReadError("BinaryModelReadPrimitives::validateBinaryHeader", "Unsupported binary model version");
    }

    const int pointerSize = readInt32LE(input);
    const int longSize = readInt32LE(input);
    const int modelSize = readInt32LE(input);

    if ( pointerSize != static_cast<int>(sizeof(void *))
         || longSize != static_cast<int>(sizeof(long))
         || modelSize != static_cast<int>(sizeof(ParseSnapshotContext)) ) {
        return reportReadError("BinaryModelReadPrimitives::validateBinaryHeader", "Incompatible binary model platform/type sizes");
    }
    return true;
}

bool
BinaryModelReadPrimitives::populateModelStrings(ParseSnapshotContext *model, const BinaryModelSnapshotRecordData &record) {
    if ( model == nullptr ) {
        return reportReadError("BinaryModelReadPrimitives::populateModelStrings", "Null model in string population");
    }

    if ( !duplicateNullableString(
        record.hasCurrentMaterialName,
        record.currentMaterialName,
        &model->currentMaterialName) ) {
        return false;
    }
    if ( !duplicateNullableString(
        record.hasCurrentObjectName,
        record.currentObjectName,
        &model->currentObjectName) ) {
        return false;
    }
    if ( !duplicateNullableString(
        record.hasCurrentVertexName,
        record.currentVertexName,
        &model->currentVertexName) ) {
        return false;
    }
    return true;
}
