#include <string.h>
#include <limits.h>
#include <stdint.h>

#include "java/lang/Integer.h"
#include "java/util/ArrayList.txx"

#include "common/color/ColorRgb.h"
#include "common/Error.h"
#include "common/linealAlgebra/Vector3D.h"
#include "skin/MinMaxBox.h"
#include "io/context/ParseSnapshotContext.h"
#include "io/wrapper/PersistenceElement.h"
#include "io/bin/reader/BinaryModelIndexListRef.h"
#include "io/bin/reader/BinaryModelSnapshotRecordData.h"
#include "io/bin/reader/BinaryModelReadPrimitives.h"

const unsigned char BinaryModelReadPrimitives::BINARY_MODEL_MAGIC[16] = {
    'R', 'P', 'K', '_', 'M', 'G', 'F', '_',
    'B', 'I', 'N', '_', '1', 0, 0, 0
};

bool
BinaryModelReadPrimitives::reportReadError(const char *routine, const char *message) {
    Error::error(routine, "%s", message);
    return false;
}

void
BinaryModelReadPrimitives::releaseIndexListRecord(BinaryModelIndexListRef *record) {
    if ( record == NULL ) {
        return;
    }
    if ( record->indices != NULL ) {
        delete record->indices;
        record->indices = NULL;
    }
    record->isNull = true;
}

void
BinaryModelReadPrimitives::readBytes(InputStream &input, unsigned char *buffer, int length) {
    if ( length <= 0 ) {
        return;
    }
    PersistenceElement::readBytes(input, buffer, length);
}

bool
BinaryModelReadPrimitives::readBytesChunked(InputStream &input, unsigned char *buffer, long length) {
    if ( length <= 0 ) {
        return true;
    }

    long offset = 0;
    const long maxChunk = ((long)(Integer::MAX_VALUE));
    while ( offset < length ) {
        const long remaining = length - offset;
        const int chunk = ((int)(remaining < maxChunk ? remaining : maxChunk));
        readBytes(input, buffer + offset, chunk);
        offset += chunk;
    }
    return true;
}

unsigned char
BinaryModelReadPrimitives::readByte(InputStream &input) {
    return ((unsigned char)(PersistenceElement::readByteUnsignedInt(input)));
}

bool
BinaryModelReadPrimitives::readBool(InputStream &input) {
    return readByte(input) != 0;
}

short
BinaryModelReadPrimitives::readInt16LE(InputStream &input) {
    return ((short)(PersistenceElement::readSignedShortLE(input)));
}

int
BinaryModelReadPrimitives::readInt32LE(InputStream &input) {
    const long value = PersistenceElement::readLongLE(input);
    return ((int)(value));
}

long
BinaryModelReadPrimitives::readInt64LE(InputStream &input) {
    const int low = readInt32LE(input);
    const int high = readInt32LE(input);

    // The binary format stores 64-bit values, but this code path returns long (32-bit on DOS/ILP32).
    // Preserve value when representable, otherwise clamp and report.
    const bool signExtendedPositive = (high == 0 && low >= 0);
    const bool signExtendedNegative = (high == -1 && low < 0);
    if ( signExtendedPositive || signExtendedNegative ) {
        return ((long)(low));
    }

    Error::error("BinaryModelReadPrimitives::readInt64LE", "64-bit value out of 32-bit long range; clamping");
    if ( high < 0 ) {
        return LONG_MIN;
    }
    return LONG_MAX;
}

float
BinaryModelReadPrimitives::readFloatLE(InputStream &input) {
    return PersistenceElement::readFloatLE(input);
}

double
BinaryModelReadPrimitives::readDoubleLE(InputStream &input) {
    unsigned char bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    readBytes(input, bytes, 8);

    uint64_t bits = 0;
    for ( int i = 0; i < 8; i++ ) {
        bits |= (((uint64_t)(bytes[i])) << (8 * i));
    }

    double value = 0.0;
    memcpy(&value, &bits, sizeof(double));
    return value;
}

bool
BinaryModelReadPrimitives::expectTag(InputStream &input, const char expected[4]) {
    unsigned char tag[4] = {0, 0, 0, 0};
    readBytes(input, tag, 4);
    if ( tag[0] != ((unsigned char)(expected[0]))
         || tag[1] != ((unsigned char)(expected[1]))
         || tag[2] != ((unsigned char)(expected[2]))
         || tag[3] != ((unsigned char)(expected[3])) ) {
        return reportReadError("BinaryModelReadPrimitives::expectTag", "Unexpected section tag while reading binary model");
    }
    return true;
}

bool
BinaryModelReadPrimitives::readNonNegativeCount(InputStream &input, const char *what, int *count) {
    if ( count == NULL ) {
        return reportReadError("BinaryModelReadPrimitives::readNonNegativeCount", "Null output count pointer");
    }
    *count = readInt32LE(input);
    if ( *count < 0 ) {
        Error::error("BinaryModelReadPrimitives::readNonNegativeCount", "Negative count while reading binary model (%s)", what);
        return false;
    }
    return true;
}

bool
BinaryModelReadPrimitives::readNullableString(InputStream &input, char **value, bool *hasValue) {
    if ( value == NULL || hasValue == NULL ) {
        return reportReadError("BinaryModelReadPrimitives::readNullableString", "Null string output pointer");
    }
    if ( *value != NULL ) {
        delete[] *value;
        *value = NULL;
    }
    *hasValue = false;

    const int size = readInt32LE(input);
    if ( size == -1 ) {
        return true;
    }
    if ( size < -1 ) {
        return reportReadError("BinaryModelReadPrimitives::readNullableString", "Invalid negative string size");
    }

    char *text = new char[((size_t)(size)) + 1];
    if ( size > 0 ) {
        readBytes(
            input,
            ((unsigned char *)(text)),
            size);
    }
    text[size] = '\0';
    *value = text;
    *hasValue = true;
    return true;
}

bool
BinaryModelReadPrimitives::duplicateNullableString(bool hasValue, const char *value, char **text) {
    if ( text == NULL ) {
        return reportReadError("BinaryModelReadPrimitives::duplicateNullableString", "Null output string pointer");
    }
    *text = NULL;
    if ( !hasValue || value == NULL ) {
        return true;
    }
    const size_t length = strlen(value);
    *text = new char[length + 1];
    memcpy(*text, value, length + 1);
    return true;
}

bool
BinaryModelReadPrimitives::readColor(InputStream &input, ColorRgb *color) {
    if ( color == NULL ) {
        return reportReadError("BinaryModelReadPrimitives::readColor", "Null color output pointer");
    }
    color->r = readFloatLE(input);
    color->g = readFloatLE(input);
    color->b = readFloatLE(input);
    return true;
}

bool
BinaryModelReadPrimitives::readVector(InputStream &input, Vector3D *vector) {
    if ( vector == NULL ) {
        return reportReadError("BinaryModelReadPrimitives::readVector", "Null vector output pointer");
    }
    vector->x = readFloatLE(input);
    vector->y = readFloatLE(input);
    vector->z = readFloatLE(input);
    return true;
}

bool
BinaryModelReadPrimitives::readBoundingBoxCoordinates(InputStream &input, float coordinates[6]) {
    if ( coordinates == NULL ) {
        return reportReadError("BinaryModelReadPrimitives::readBoundingBoxCoordinates", "Null bounding box coordinate buffer");
    }
    for ( int i = 0; i < 6; i++ ) {
        coordinates[i] = readFloatLE(input);
    }
    return true;
}

bool
BinaryModelReadPrimitives::setBoundingBoxFromCoordinates(BoundingBox *boundingBox, const float coordinates[6]) {
    if ( boundingBox == NULL || coordinates == NULL ) {
        return reportReadError("BinaryModelReadPrimitives::setBoundingBoxFromCoordinates", "Invalid bounding box assignment");
    }
    BoundingBox parsed;
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
BinaryModelReadPrimitives::readIndexList(InputStream &input, const char *what, BinaryModelIndexListRef *record) {
    if ( record == NULL ) {
        return reportReadError("BinaryModelReadPrimitives::readIndexList", "Null output record");
    }
    record->isNull = false;
    record->indices = NULL;

    const int count = readInt32LE(input);
    if ( count == -1 ) {
        record->isNull = true;
        return true;
    }
    if ( count < -1 ) {
        Error::error("BinaryModelReadPrimitives::readIndexList", "Negative index list count while reading binary model (%s)", what);
        return false;
    }

    record->indices = new ArrayList<int>(count > 0 ? ((long int)(count)) : 1);
    for ( int i = 0; i < count; i++ ) {
        if ( !record->indices->add(readInt32LE(input)) ) {
            delete record->indices;
            record->indices = NULL;
            Error::error("BinaryModelReadPrimitives::readIndexList", "Failed to allocate index list while reading binary model (%s)", what);
            return false;
        }
    }

    return true;
}

bool
BinaryModelReadPrimitives::validateBinaryHeader(InputStream &input) {
    unsigned char magic[16] = {0};
    readBytes(input, magic, 16);
    if ( memcmp(magic, BINARY_MODEL_MAGIC, 16) != 0 ) {
        return reportReadError("BinaryModelReadPrimitives::validateBinaryHeader", "Invalid binary model magic header");
    }

    const int version = readInt32LE(input);
    if ( version != BINARY_MODEL_VERSION ) {
        return reportReadError("BinaryModelReadPrimitives::validateBinaryHeader", "Unsupported binary model version");
    }

    const int pointerSize = readInt32LE(input);
    const int longSize = readInt32LE(input);
    const int modelSize = readInt32LE(input);

    if ( pointerSize != ((int)(sizeof(void *)))
         || longSize != ((int)(sizeof(long)))
         || modelSize != ((int)(sizeof(ParseSnapshotContext))) ) {
        return reportReadError("BinaryModelReadPrimitives::validateBinaryHeader", "Incompatible binary model platform/type sizes");
    }
    return true;
}

bool
BinaryModelReadPrimitives::populateModelStrings(ParseSnapshotContext *model, const BinaryModelSnapshotRecordData &record) {
    if ( model == NULL ) {
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
