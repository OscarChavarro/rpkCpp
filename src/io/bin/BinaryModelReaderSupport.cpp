#include <cstring>

#include "java/lang/Integer.h"
#include "java/util/ArrayList.txx"

#include "common/ColorRgb.h"
#include "common/Error.h"
#include "common/linealAlgebra/Vector3D.h"
#include "skin/MinMaxBox.h"
#include "io/context/PersistedSceneModel.h"
#include "io/wrapper/PersistenceElement.h"
#include "io/bin/BinaryModelReaderIndexListRecord.h"
#include "io/bin/BinaryModelReaderModelRecord.h"
#include "io/bin/BinaryModelReaderSupport.h"

const unsigned char BINARY_MODEL_MAGIC[16] = {
    'R', 'P', 'K', '_', 'M', 'G', 'F', '_',
    'B', 'I', 'N', '_', '1', 0, 0, 0
};

const int BINARY_MODEL_VERSION = 1;

bool
BinaryModelReaderSupport::reportReadError(const char *routine, const char *message) {
    Error::error(routine, "%s", message);
    return false;
}

void
BinaryModelReaderSupport::releaseIndexListRecord(BinaryModelReaderIndexListRecord *record) {
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
BinaryModelReaderSupport::readBytes(java::io::InputStream &input, unsigned char *buffer, int length) {
    if ( length <= 0 ) {
        return;
    }
    vsdk::PersistenceElement::readBytes(input, buffer, length);
}

bool
BinaryModelReaderSupport::readBytesChunked(java::io::InputStream &input, unsigned char *buffer, long long length) {
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
BinaryModelReaderSupport::readByte(java::io::InputStream &input) {
    return static_cast<unsigned char>(vsdk::PersistenceElement::readByteUnsignedInt(input));
}

bool
BinaryModelReaderSupport::readBool(java::io::InputStream &input) {
    return readByte(input) != 0;
}

short
BinaryModelReaderSupport::readInt16LE(java::io::InputStream &input) {
    return static_cast<short>(vsdk::PersistenceElement::readSignedShortLE(input));
}

int
BinaryModelReaderSupport::readInt32LE(java::io::InputStream &input) {
    const long value = vsdk::PersistenceElement::readLongLE(input);
    return static_cast<int>(value);
}

long long
BinaryModelReaderSupport::readInt64LE(java::io::InputStream &input) {
    unsigned char bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    readBytes(input, bytes, 8);
    unsigned long long value = 0;
    for ( int i = 0; i < 8; i++ ) {
        value |= static_cast<unsigned long long>(bytes[i]) << (8 * i);
    }
    return static_cast<long long>(value);
}

float
BinaryModelReaderSupport::readFloatLE(java::io::InputStream &input) {
    return vsdk::PersistenceElement::readFloatLE(input);
}

double
BinaryModelReaderSupport::readDoubleLE(java::io::InputStream &input) {
    return vsdk::PersistenceElement::readDoubleLE(input);
}

bool
BinaryModelReaderSupport::expectTag(java::io::InputStream &input, const char expected[4]) {
    unsigned char tag[4] = {0, 0, 0, 0};
    readBytes(input, tag, 4);
    if ( tag[0] != static_cast<unsigned char>(expected[0])
         || tag[1] != static_cast<unsigned char>(expected[1])
         || tag[2] != static_cast<unsigned char>(expected[2])
         || tag[3] != static_cast<unsigned char>(expected[3]) ) {
        return reportReadError("BinaryModelReaderSupport::expectTag", "Unexpected section tag while reading binary model");
    }
    return true;
}

bool
BinaryModelReaderSupport::readNonNegativeCount(java::io::InputStream &input, const char *what, int *count) {
    if ( count == nullptr ) {
        return reportReadError("BinaryModelReaderSupport::readNonNegativeCount", "Null output count pointer");
    }
    *count = readInt32LE(input);
    if ( *count < 0 ) {
        Error::error("BinaryModelReaderSupport::readNonNegativeCount", "Negative count while reading binary model (%s)", what);
        return false;
    }
    return true;
}

bool
BinaryModelReaderSupport::readNullableString(java::io::InputStream &input, char **value, bool *hasValue) {
    if ( value == nullptr || hasValue == nullptr ) {
        return reportReadError("BinaryModelReaderSupport::readNullableString", "Null string output pointer");
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
        return reportReadError("BinaryModelReaderSupport::readNullableString", "Invalid negative string size");
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
BinaryModelReaderSupport::duplicateNullableString(bool hasValue, const char *value, char **text) {
    if ( text == nullptr ) {
        return reportReadError("BinaryModelReaderSupport::duplicateNullableString", "Null output string pointer");
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
BinaryModelReaderSupport::readColor(java::io::InputStream &input, ColorRgb *color) {
    if ( color == nullptr ) {
        return reportReadError("BinaryModelReaderSupport::readColor", "Null color output pointer");
    }
    color->r = readFloatLE(input);
    color->g = readFloatLE(input);
    color->b = readFloatLE(input);
    return true;
}

bool
BinaryModelReaderSupport::readVector(java::io::InputStream &input, Vector3D *vector) {
    if ( vector == nullptr ) {
        return reportReadError("BinaryModelReaderSupport::readVector", "Null vector output pointer");
    }
    vector->x = readFloatLE(input);
    vector->y = readFloatLE(input);
    vector->z = readFloatLE(input);
    return true;
}

bool
BinaryModelReaderSupport::readBoundingBoxCoordinates(java::io::InputStream &input, float coordinates[6]) {
    if ( coordinates == nullptr ) {
        return reportReadError("BinaryModelReaderSupport::readBoundingBoxCoordinates", "Null bounding box coordinate buffer");
    }
    for ( int i = 0; i < 6; i++ ) {
        coordinates[i] = readFloatLE(input);
    }
    return true;
}

bool
BinaryModelReaderSupport::setBoundingBoxFromCoordinates(BoundingBox *boundingBox, const float coordinates[6]) {
    if ( boundingBox == nullptr || coordinates == nullptr ) {
        return reportReadError("BinaryModelReaderSupport::setBoundingBoxFromCoordinates", "Invalid bounding box assignment");
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
BinaryModelReaderSupport::readIndexList(java::io::InputStream &input, const char *what, BinaryModelReaderIndexListRecord *record) {
    if ( record == nullptr ) {
        return reportReadError("BinaryModelReaderSupport::readIndexList", "Null output record");
    }
    record->isNull = false;
    record->indices = nullptr;

    const int count = readInt32LE(input);
    if ( count == -1 ) {
        record->isNull = true;
        return true;
    }
    if ( count < -1 ) {
        Error::error("BinaryModelReaderSupport::readIndexList", "Negative index list count while reading binary model (%s)", what);
        return false;
    }

    record->indices = new java::ArrayList<int>(count > 0 ? static_cast<long int>(count) : 1);
    for ( int i = 0; i < count; i++ ) {
        if ( !record->indices->add(readInt32LE(input)) ) {
            delete record->indices;
            record->indices = nullptr;
            Error::error("BinaryModelReaderSupport::readIndexList", "Failed to allocate index list while reading binary model (%s)", what);
            return false;
        }
    }

    return true;
}

bool
BinaryModelReaderSupport::validateBinaryHeader(java::io::InputStream &input) {
    unsigned char magic[16] = {0};
    readBytes(input, magic, 16);
    if ( std::memcmp(magic, BINARY_MODEL_MAGIC, 16) != 0 ) {
        return reportReadError("BinaryModelReaderSupport::validateBinaryHeader", "Invalid binary model magic header");
    }

    const int version = readInt32LE(input);
    if ( version != BINARY_MODEL_VERSION ) {
        return reportReadError("BinaryModelReaderSupport::validateBinaryHeader", "Unsupported binary model version");
    }

    const int pointerSize = readInt32LE(input);
    const int longSize = readInt32LE(input);
    const int modelSize = readInt32LE(input);

    if ( pointerSize != static_cast<int>(sizeof(void *))
         || longSize != static_cast<int>(sizeof(long))
         || modelSize != static_cast<int>(sizeof(PersistedSceneModel)) ) {
        return reportReadError("BinaryModelReaderSupport::validateBinaryHeader", "Incompatible binary model platform/type sizes");
    }
    return true;
}

bool
BinaryModelReaderSupport::populateModelStrings(PersistedSceneModel *model, const BinaryModelReaderModelRecord &record) {
    if ( model == nullptr ) {
        return reportReadError("BinaryModelReaderSupport::populateModelStrings", "Null model in string population");
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
