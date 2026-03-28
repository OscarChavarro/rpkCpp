#include "io/bin/BinaryModelReader.h"

#include <cstring>

#include "java/io/BufferedInputStream.h"
#include "java/io/FileInputStream.h"
#include "java/lang/Integer.h"
#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/ColorRgb.h"
#include "common/linealAlgebra/Jacobian.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/wrapper/PersistenceElement.h"
#include "io/context/ColorContext.h"
#include "io/context/PersistedSceneModel.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformArray.h"
#include "io/context/TransformStackContext.h"
#include "material/Material.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"
#include "material/PhongEmittanceDistributionFunction.h"
#include "material/Texture.h"
#include "skin/Compound.h"
#include "skin/Geometry.h"
#include "skin/MaterialColorFlags.h"
#include "skin/MeshSurface.h"
#include "skin/MinMaxBox.h"
#include "skin/Patch.h"
#include "skin/PatchSet.h"
#include "skin/Vertex.h"

const unsigned char BinaryModelReader::BINARY_MODEL_MAGIC[16] = {
    'R', 'P', 'K', '_', 'M', 'G', 'F', '_',
    'B', 'I', 'N', '_', '1', 0, 0, 0
};
const int BinaryModelReader::BINARY_MODEL_VERSION = 1;

namespace {
template <typename T>
class ScopedArray {
  private:
    T *value;

  public:
    explicit ScopedArray(T *initialValue = nullptr):
        value(initialValue)
    {
    }

    ~ScopedArray() {
        delete[] value;
        value = nullptr;
    }

    ScopedArray(const ScopedArray &) = delete;
    ScopedArray &operator=(const ScopedArray &) = delete;

    void
    reset(T *newValue = nullptr) {
        if ( value != newValue ) {
            delete[] value;
            value = newValue;
        }
    }

    T *
    get() const {
        return value;
    }
};

bool
reportReadError(const char *routine, const char *message) {
    logError(routine, "%s", message);
    return false;
}
}

class BinaryModelReader::IndexListRecord {
  public:
    bool isNull;
    java::ArrayList<int> *indices;

    IndexListRecord():
        isNull(true),
        indices(nullptr)
    {
    }
};

class BinaryModelReader::VertexRecord {
  public:
    int id;
    int pointIndex;
    int normalIndex;
    int textureCoordinateIndex;
    ColorRgb color;
    int backIndex;
    int tmp;
    bool hasRadianceData;
    IndexListRecord patchIndices;
};

class BinaryModelReader::PatchRecord {
  public:
    int id;
    int twinIndex;
    int numberOfVertices;
    int vertexIndices[MAXIMUM_VERTICES_PER_PATCH];
    bool hasBoundingBox;
    float boundingBoxCoordinates[6];
    Vector3D normal;
    float planeConstant;
    float tolerance;
    float area;
    Vector3D midPoint;
    bool hasJacobian;
    float jacobianA;
    float jacobianB;
    float jacobianC;
    float directPotential;
    int dominantIndex;
    bool omit;
    unsigned char flags;
    ColorRgb color;
    int materialIndex;
    bool hasRadianceData;
};

class BinaryModelReader::GeometryRecord {
  public:
    int classId;
    int id;
    int itemCount;
    bool bounded;
    bool shaftCullGeometry;
    bool omit;
    bool isDuplicate;
    float boundingBoxCoordinates[6];
    bool hasRayIntersectionBox;
    bool hasRadianceData;

    bool hasObjectName;
    char *objectName;
    int meshId;
    int materialIndex;
    IndexListRecord positions;
    IndexListRecord normals;
    IndexListRecord vertices;
    IndexListRecord faces;

    IndexListRecord children;
    IndexListRecord patchSetPatches;
};

class BinaryModelReader::ModelRecord {
  public:
    int currentColorIndex;
    bool hasCurrentMaterialName;
    char *currentMaterialName;
    bool hasCurrentObjectName;
    char *currentObjectName;
    bool hasCurrentVertexName;
    char *currentVertexName;
    int geometryStackHeadIndex;
    bool inComplex;
    bool inSurface;
    bool monochrome;
    int readerContextIndex;
    int transformContextIndex;

    IndexListRecord currentFaceList;
    IndexListRecord currentGeometryList;
    IndexListRecord currentNormalList;
    IndexListRecord currentPointList;
    IndexListRecord currentVertexList;
    IndexListRecord geometries;
    IndexListRecord materials;

    ModelRecord():
        currentColorIndex(0),
        hasCurrentMaterialName(false),
        currentMaterialName(nullptr),
        hasCurrentObjectName(false),
        currentObjectName(nullptr),
        hasCurrentVertexName(false),
        currentVertexName(nullptr),
        geometryStackHeadIndex(0),
        inComplex(false),
        inSurface(false),
        monochrome(false),
        readerContextIndex(0),
        transformContextIndex(0)
    {
    }
};

template <typename T>
bool
BinaryModelReader::initializeArrayList(java::ArrayList<T> *list, int count, T initialValue, const char *what) {
    if ( list == nullptr ) {
        return reportReadError("BinaryModelReader::initializeArrayList", "Null list pointer");
    }
    if ( count < 0 ) {
        logError("BinaryModelReader::initializeArrayList", "Negative count while reading binary model (%s)", what);
        return false;
    }
    for ( int i = 0; i < count; i++ ) {
        if ( !list->add(initialValue) ) {
            logError("BinaryModelReader::initializeArrayList", "Failed to allocate entries while reading binary model (%s)", what);
            return false;
        }
    }
    return true;
}

void
BinaryModelReader::releaseIndexListRecord(IndexListRecord *record) {
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
BinaryModelReader::readBytes(java::io::InputStream &input, unsigned char *buffer, int length) {
    if ( length <= 0 ) {
        return;
    }
    vsdk::PersistenceElement::readBytes(input, buffer, length);
}

bool
BinaryModelReader::readBytesChunked(java::io::InputStream &input, unsigned char *buffer, long long length) {
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
BinaryModelReader::readByte(java::io::InputStream &input) {
    return static_cast<unsigned char>(vsdk::PersistenceElement::readByteUnsignedInt(input));
}

bool
BinaryModelReader::readBool(java::io::InputStream &input) {
    return readByte(input) != 0;
}

short
BinaryModelReader::readInt16LE(java::io::InputStream &input) {
    return static_cast<short>(vsdk::PersistenceElement::readSignedShortLE(input));
}

int
BinaryModelReader::readInt32LE(java::io::InputStream &input) {
    const long value = vsdk::PersistenceElement::readLongLE(input);
    return static_cast<int>(value);
}

long long
BinaryModelReader::readInt64LE(java::io::InputStream &input) {
    unsigned char bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    readBytes(input, bytes, 8);
    unsigned long long value = 0;
    for ( int i = 0; i < 8; i++ ) {
        value |= static_cast<unsigned long long>(bytes[i]) << (8 * i);
    }
    return static_cast<long long>(value);
}

float
BinaryModelReader::readFloatLE(java::io::InputStream &input) {
    return vsdk::PersistenceElement::readFloatLE(input);
}

double
BinaryModelReader::readDoubleLE(java::io::InputStream &input) {
    return vsdk::PersistenceElement::readDoubleLE(input);
}

bool
BinaryModelReader::expectTag(java::io::InputStream &input, const char expected[4]) {
    unsigned char tag[4] = {0, 0, 0, 0};
    readBytes(input, tag, 4);
    if ( tag[0] != static_cast<unsigned char>(expected[0])
         || tag[1] != static_cast<unsigned char>(expected[1])
         || tag[2] != static_cast<unsigned char>(expected[2])
         || tag[3] != static_cast<unsigned char>(expected[3]) ) {
        return reportReadError("BinaryModelReader::expectTag", "Unexpected section tag while reading binary model");
    }
    return true;
}

bool
BinaryModelReader::readNonNegativeCount(java::io::InputStream &input, const char *what, int *count) {
    if ( count == nullptr ) {
        return reportReadError("BinaryModelReader::readNonNegativeCount", "Null output count pointer");
    }
    *count = readInt32LE(input);
    if ( *count < 0 ) {
        logError("BinaryModelReader::readNonNegativeCount", "Negative count while reading binary model (%s)", what);
        return false;
    }
    return true;
}

bool
BinaryModelReader::readNullableString(java::io::InputStream &input, char **value, bool *hasValue) {
    if ( value == nullptr || hasValue == nullptr ) {
        return reportReadError("BinaryModelReader::readNullableString", "Null string output pointer");
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
        return reportReadError("BinaryModelReader::readNullableString", "Invalid negative string size");
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
BinaryModelReader::duplicateNullableString(bool hasValue, const char *value, char **text) {
    if ( text == nullptr ) {
        return reportReadError("BinaryModelReader::duplicateNullableString", "Null output string pointer");
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
BinaryModelReader::readColor(java::io::InputStream &input, ColorRgb *color) {
    if ( color == nullptr ) {
        return reportReadError("BinaryModelReader::readColor", "Null color output pointer");
    }
    color->r = readFloatLE(input);
    color->g = readFloatLE(input);
    color->b = readFloatLE(input);
    return true;
}

bool
BinaryModelReader::readVector(java::io::InputStream &input, Vector3D *vector) {
    if ( vector == nullptr ) {
        return reportReadError("BinaryModelReader::readVector", "Null vector output pointer");
    }
    vector->x = readFloatLE(input);
    vector->y = readFloatLE(input);
    vector->z = readFloatLE(input);
    return true;
}

bool
BinaryModelReader::readBoundingBoxCoordinates(java::io::InputStream &input, float coordinates[6]) {
    if ( coordinates == nullptr ) {
        return reportReadError("BinaryModelReader::readBoundingBoxCoordinates", "Null bounding box coordinate buffer");
    }
    for ( int i = 0; i < 6; i++ ) {
        coordinates[i] = readFloatLE(input);
    }
    return true;
}

bool
BinaryModelReader::setBoundingBoxFromCoordinates(BoundingBox *boundingBox, const float coordinates[6]) {
    if ( boundingBox == nullptr || coordinates == nullptr ) {
        return reportReadError("BinaryModelReader::setBoundingBoxFromCoordinates", "Invalid bounding box assignment");
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
BinaryModelReader::readIndexList(java::io::InputStream &input, const char *what, BinaryModelReader::IndexListRecord *record) {
    if ( record == nullptr ) {
        return reportReadError("BinaryModelReader::readIndexList", "Null output record");
    }
    record->isNull = false;
    record->indices = nullptr;

    const int count = readInt32LE(input);
    if ( count == -1 ) {
        record->isNull = true;
        return true;
    }
    if ( count < -1 ) {
        logError("BinaryModelReader::readIndexList", "Negative index list count while reading binary model (%s)", what);
        return false;
    }

    record->indices = new java::ArrayList<int>(count > 0 ? static_cast<long int>(count) : 1);
    for ( int i = 0; i < count; i++ ) {
        if ( !record->indices->add(readInt32LE(input)) ) {
            delete record->indices;
            record->indices = nullptr;
            logError("BinaryModelReader::readIndexList", "Failed to allocate index list while reading binary model (%s)", what);
            return false;
        }
    }

    return true;
}

template <typename T>
bool
BinaryModelReader::pointerFromIndex(const java::ArrayList<T *> &values, int index, const char *what, T **result) {
    if ( result == nullptr ) {
        return reportReadError("BinaryModelReader::pointerFromIndex", "Null output pointer");
    }
    *result = nullptr;
    if ( index == -1 ) {
        return true;
    }
    if ( index < 0 || static_cast<long int>(index) >= values.size() ) {
        logError("BinaryModelReader::pointerFromIndex", "Out of range index while reading binary model (%s)", what);
        return false;
    }
    *result = values.get(static_cast<long int>(index));
    return true;
}

template <typename T>
bool
BinaryModelReader::arrayListFromIndices(
    const BinaryModelReader::IndexListRecord &record,
    const java::ArrayList<T *> &values,
    const char *what,
    java::ArrayList<T *> **result)
{
    if ( result == nullptr ) {
        return reportReadError("BinaryModelReader::arrayListFromIndices", "Null output pointer");
    }
    *result = nullptr;
    if ( record.isNull ) {
        return true;
    }
    if ( record.indices == nullptr ) {
        return reportReadError("BinaryModelReader::arrayListFromIndices", "Missing index list while reading binary model");
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
            return reportReadError("BinaryModelReader::arrayListFromIndices", "Failed to allocate output list while reading binary model");
        }
    }
    *result = list;
    return true;
}

bool
BinaryModelReader::validateBinaryHeader(java::io::InputStream &input) {
    unsigned char magic[16] = {0};
    readBytes(input, magic, 16);
    if ( std::memcmp(magic, BINARY_MODEL_MAGIC, 16) != 0 ) {
        return reportReadError("BinaryModelReader::validateBinaryHeader", "Invalid binary model magic header");
    }

    const int version = readInt32LE(input);
    if ( version != BINARY_MODEL_VERSION ) {
        return reportReadError("BinaryModelReader::validateBinaryHeader", "Unsupported binary model version");
    }

    const int pointerSize = readInt32LE(input);
    const int longSize = readInt32LE(input);
    const int modelSize = readInt32LE(input);

    if ( pointerSize != static_cast<int>(sizeof(void *))
         || longSize != static_cast<int>(sizeof(long))
         || modelSize != static_cast<int>(sizeof(PersistedSceneModel)) ) {
        return reportReadError("BinaryModelReader::validateBinaryHeader", "Incompatible binary model platform/type sizes");
    }
    return true;
}

bool
BinaryModelReader::populateModelStrings(PersistedSceneModel *model, const BinaryModelReader::ModelRecord &record) {
    if ( model == nullptr ) {
        return reportReadError("BinaryModelReader::populateModelStrings", "Null model in string population");
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

void
BinaryModelReader::cleanupPartialModel(
    java::ArrayList<Vector3D *> &vectors,
    java::ArrayList<Vertex *> &vertices,
    java::ArrayList<Patch *> &patches,
    java::ArrayList<Material *> &materials,
    java::ArrayList<Geometry *> &geometries,
    java::ArrayList<ColorContext *> &colorContexts,
    java::ArrayList<ReaderContext *> &readerContexts,
    java::ArrayList<TransformArray *> &transformArrays,
    java::ArrayList<TransformStackContext *> &transformContexts,
    PersistedSceneModel *model)
{
    const bool hasGeometry = geometries.size() > 0;
    bool hasSurfaceGeometry = false;
    for ( long int i = 0; i < geometries.size(); i++ ) {
        Geometry *geometry = geometries.get(i);
        if ( geometry != nullptr && geometry->className == GeometryClassId::SURFACE_MESH ) {
            hasSurfaceGeometry = true;
            break;
        }
    }

    for ( long int i = 0; i < geometries.size(); i++ ) {
        delete geometries.get(i);
    }

    if ( !hasGeometry || !hasSurfaceGeometry ) {
        for ( long int i = 0; i < patches.size(); i++ ) {
            delete patches.get(i);
        }
        for ( long int i = 0; i < vertices.size(); i++ ) {
            delete vertices.get(i);
        }
        for ( long int i = 0; i < vectors.size(); i++ ) {
            delete vectors.get(i);
        }
    }

    for ( long int i = 0; i < materials.size(); i++ ) {
        delete materials.get(i);
    }
    for ( long int i = 0; i < colorContexts.size(); i++ ) {
        delete colorContexts.get(i);
    }
    for ( long int i = 0; i < readerContexts.size(); i++ ) {
        delete readerContexts.get(i);
    }
    for ( long int i = 0; i < transformArrays.size(); i++ ) {
        delete transformArrays.get(i);
    }
    for ( long int i = 0; i < transformContexts.size(); i++ ) {
        delete transformContexts.get(i);
    }

    if ( model != nullptr ) {
        delete[] model->currentMaterialName;
        delete[] model->currentObjectName;
        delete[] model->currentVertexName;
        delete model;
    }
}

void
BinaryModelReader::releaseVertexRecordIndexLists(java::ArrayList<BinaryModelReader::VertexRecord> &vertexRecords) {
    for ( long int i = 0; i < vertexRecords.size(); i++ ) {
        releaseIndexListRecord(&vertexRecords[i].patchIndices);
    }
}

void
BinaryModelReader::releaseGeometryRecordIndexLists(java::ArrayList<BinaryModelReader::GeometryRecord> &geometryRecords) {
    for ( long int i = 0; i < geometryRecords.size(); i++ ) {
        GeometryRecord &record = geometryRecords[i];
        if ( record.objectName != nullptr ) {
            delete[] record.objectName;
            record.objectName = nullptr;
        }
        record.hasObjectName = false;
        releaseIndexListRecord(&record.positions);
        releaseIndexListRecord(&record.normals);
        releaseIndexListRecord(&record.vertices);
        releaseIndexListRecord(&record.faces);
        releaseIndexListRecord(&record.children);
        releaseIndexListRecord(&record.patchSetPatches);
    }
}

void
BinaryModelReader::releaseModelRecordIndexLists(BinaryModelReader::ModelRecord *modelRecord) {
    if ( modelRecord == nullptr ) {
        return;
    }
    if ( modelRecord->currentMaterialName != nullptr ) {
        delete[] modelRecord->currentMaterialName;
        modelRecord->currentMaterialName = nullptr;
    }
    if ( modelRecord->currentObjectName != nullptr ) {
        delete[] modelRecord->currentObjectName;
        modelRecord->currentObjectName = nullptr;
    }
    if ( modelRecord->currentVertexName != nullptr ) {
        delete[] modelRecord->currentVertexName;
        modelRecord->currentVertexName = nullptr;
    }
    modelRecord->hasCurrentMaterialName = false;
    modelRecord->hasCurrentObjectName = false;
    modelRecord->hasCurrentVertexName = false;
    releaseIndexListRecord(&modelRecord->currentFaceList);
    releaseIndexListRecord(&modelRecord->currentGeometryList);
    releaseIndexListRecord(&modelRecord->currentNormalList);
    releaseIndexListRecord(&modelRecord->currentPointList);
    releaseIndexListRecord(&modelRecord->currentVertexList);
    releaseIndexListRecord(&modelRecord->geometries);
    releaseIndexListRecord(&modelRecord->materials);
}

PersistedSceneModel *
BinaryModelReader::read(const char *fileName) {
    if ( fileName == nullptr || fileName[0] == '\0' ) {
        return nullptr;
    }
    java::io::File file(fileName);
    if ( !(file.exists() && file.canRead() && file.isFile()) ) {
        return nullptr;
    }

    java::io::FileInputStream fileInput(fileName);
    java::io::BufferedInputStream input(&fileInput);

    java::ArrayList<Vector3D *> vectors;
    java::ArrayList<Vertex *> vertices;
    java::ArrayList<Patch *> patches;
    java::ArrayList<Material *> materials;
    java::ArrayList<Geometry *> geometries;
    java::ArrayList<ColorContext *> colorContexts;
    java::ArrayList<ReaderContext *> readerContexts;
    java::ArrayList<TransformArray *> transformArrays;
    java::ArrayList<TransformStackContext *> transformContexts;
    java::ArrayList<VertexRecord> vertexRecords;
    java::ArrayList<PatchRecord> patchRecords;
    java::ArrayList<GeometryRecord> geometryRecords;
    ModelRecord modelRecord;
    PersistedSceneModel *model = nullptr;
    bool ok = false;
    int vectorCount = 0;
    int vertexCount = 0;
    int patchCount = 0;
    int materialCount = 0;
    int geometryCount = 0;
    int colorContextCount = 0;
    int readerContextCount = 0;
    int transformArrayCount = 0;
    int transformContextCount = 0;

    try {
        if ( !validateBinaryHeader(input) ) goto fail;

        if ( !readNonNegativeCount(input, "vectors", &vectorCount) ) goto fail;
        if ( !readNonNegativeCount(input, "vertices", &vertexCount) ) goto fail;
        if ( !readNonNegativeCount(input, "patches", &patchCount) ) goto fail;
        if ( !readNonNegativeCount(input, "materials", &materialCount) ) goto fail;
        if ( !readNonNegativeCount(input, "geometries", &geometryCount) ) goto fail;
        if ( !readNonNegativeCount(input, "color contexts", &colorContextCount) ) goto fail;
        if ( !readNonNegativeCount(input, "reader contexts", &readerContextCount) ) goto fail;
        if ( !readNonNegativeCount(input, "transform arrays", &transformArrayCount) ) goto fail;
        if ( !readNonNegativeCount(input, "transform contexts", &transformContextCount) ) goto fail;

        if ( !initializeArrayList(&vectors, vectorCount, static_cast<Vector3D *>(nullptr), "vectors") ) goto fail;
        if ( !initializeArrayList(&vertices, vertexCount, static_cast<Vertex *>(nullptr), "vertices") ) goto fail;
        if ( !initializeArrayList(&patches, patchCount, static_cast<Patch *>(nullptr), "patches") ) goto fail;
        if ( !initializeArrayList(&materials, materialCount, static_cast<Material *>(nullptr), "materials") ) goto fail;
        if ( !initializeArrayList(&geometries, geometryCount, static_cast<Geometry *>(nullptr), "geometries") ) goto fail;
        if ( !initializeArrayList(&colorContexts, colorContextCount, static_cast<ColorContext *>(nullptr), "color contexts") ) goto fail;
        if ( !initializeArrayList(&readerContexts, readerContextCount, static_cast<ReaderContext *>(nullptr), "reader contexts") ) goto fail;
        if ( !initializeArrayList(&transformArrays, transformArrayCount, static_cast<TransformArray *>(nullptr), "transform arrays") ) goto fail;
        if ( !initializeArrayList(&transformContexts, transformContextCount, static_cast<TransformStackContext *>(nullptr), "transform contexts") ) goto fail;
        if ( !initializeArrayList(&vertexRecords, vertexCount, VertexRecord(), "vertex records") ) goto fail;
        if ( !initializeArrayList(&patchRecords, patchCount, PatchRecord(), "patch records") ) goto fail;
        if ( !initializeArrayList(&geometryRecords, geometryCount, GeometryRecord(), "geometry records") ) goto fail;

        if ( !expectTag(input, "VEC3") ) goto fail;
        for ( int i = 0; i < vectorCount; i++ ) {
            Vector3D *vector = new Vector3D();
            if ( !readVector(input, vector) ) {
                delete vector;
                goto fail;
            }
            vectors.set(static_cast<long int>(i), vector);
        }

        if ( !expectTag(input, "MTLS") ) goto fail;
        for ( int i = 0; i < materialCount; i++ ) {
            ScopedArray<char> materialNameGuard;
            char *materialName = nullptr;
            bool hasMaterialName = false;
            if ( !readNullableString(input, &materialName, &hasMaterialName) ) goto fail;
            materialNameGuard.reset(materialName);
            const bool sided = readBool(input);

            PhongEmittanceDistributionFunction *edf = nullptr;
            const bool hasEdf = readBool(input);
            if ( hasEdf ) {
                ColorRgb kd;
                ColorRgb ks;
                if ( !readColor(input, &kd) ) goto fail;
                if ( !readColor(input, &ks) ) goto fail;
                const float ns = readFloatLE(input);
                edf = new PhongEmittanceDistributionFunction(&kd, &ks, ns);
            }

            PhongBidirectionalScatteringDistributionFunction *bsdf = nullptr;
            const bool hasBsdf = readBool(input);
            if ( hasBsdf ) {
                PhongBidirectionalReflectanceDistributionFunction *brdf = nullptr;
                PhongBidirectionalTransmittanceDistributionFunction *btdf = nullptr;
                Texture *texture = nullptr;

                const bool hasBrdf = readBool(input);
                if ( hasBrdf ) {
                    ColorRgb kd;
                    ColorRgb ks;
                    if ( !readColor(input, &kd) ) goto fail;
                    if ( !readColor(input, &ks) ) goto fail;
                    const float ns = readFloatLE(input);
                    brdf = new PhongBidirectionalReflectanceDistributionFunction(&kd, &ks, ns);
                }

                const bool hasBtdf = readBool(input);
                if ( hasBtdf ) {
                    ColorRgb kd;
                    ColorRgb ks;
                    if ( !readColor(input, &kd) ) goto fail;
                    if ( !readColor(input, &ks) ) goto fail;
                    const float ns = readFloatLE(input);
                    const float nr = readFloatLE(input);
                    const float ni = readFloatLE(input);
                    btdf = new PhongBidirectionalTransmittanceDistributionFunction(&kd, &ks, ns, nr, ni);
                }

                const bool hasTexture = readBool(input);
                if ( hasTexture ) {
                    const int width = readInt32LE(input);
                    const int height = readInt32LE(input);
                    const int channels = readInt32LE(input);
                    const long long dataBytes = readInt64LE(input);

                    if ( width < 0 || height < 0 || channels < 0 || dataBytes < 0 ) {
                        logError("BinaryModelReader::read", "%s", "Invalid texture dimensions in binary material");
                        goto fail;
                    }

                    const long long expectedBytes = static_cast<long long>(width)
                                                  * static_cast<long long>(height)
                                                  * static_cast<long long>(channels);
                    if ( expectedBytes != dataBytes ) {
                        logError("BinaryModelReader::read", "%s", "Texture byte count mismatch in binary material");
                        goto fail;
                    }

                    ScopedArray<unsigned char> textureData;
                    if ( dataBytes > 0 ) {
                        if ( dataBytes > static_cast<long long>(java::Integer::MAX_VALUE) ) {
                            logError("BinaryModelReader::read", "%s", "Texture data too large for current platform");
                            goto fail;
                        }
                        textureData.reset(new unsigned char[static_cast<int>(dataBytes)]);
                        if ( !readBytesChunked(input, textureData.get(), dataBytes) ) goto fail;
                    }
                    texture = new Texture(
                        width,
                        height,
                        channels,
                        textureData.get());
                }

                bsdf = new PhongBidirectionalScatteringDistributionFunction(brdf, btdf, texture);
            }

            const char *materialNameCstr = hasMaterialName ? materialNameGuard.get() : "";
            materials.set(static_cast<long int>(i), new Material(materialNameCstr, edf, bsdf, sided));
        }

        if ( !expectTag(input, "COLR") ) goto fail;
        for ( int i = 0; i < colorContextCount; i++ ) {
            ColorContext *colorContext = new ColorContext();
            colorContext->clock = readInt32LE(input);
            colorContext->flags = readInt16LE(input);
            for ( int j = 0; j < NUMBER_OF_SPECTRAL_SAMPLES; j++ ) {
                colorContext->straightSamples[j] = readInt16LE(input);
            }
            colorContext->spectralStraightSum = static_cast<long>(readInt64LE(input));
            colorContext->cx = readFloatLE(input);
            colorContext->cy = readFloatLE(input);
            colorContext->eff = readFloatLE(input);
            colorContexts.set(static_cast<long int>(i), colorContext);
        }

        if ( !expectTag(input, "RCTX") ) goto fail;
        java::ArrayList<int> readerContextPrevIndex;
        if ( !initializeArrayList(&readerContextPrevIndex, readerContextCount, static_cast<int>(-1), "reader context prev index") ) goto fail;
        for ( int i = 0; i < readerContextCount; i++ ) {
            ReaderContext *readerContext = new ReaderContext();
            readBytes(input, reinterpret_cast<unsigned char *>(readerContext->fileName), 96);
            readerContext->fileName[95] = '\0';

            const bool hasInputStream = readBool(input);
            readerContext->inputStream = nullptr;
            if ( hasInputStream ) {
                readerContext->inputStream = nullptr;
            }

            readerContext->fileContextId = readInt32LE(input);
            readBytes(
                input,
                reinterpret_cast<unsigned char *>(readerContext->inputLine),
                MGF_MAXIMUM_INPUT_LINE_LENGTH);
            readerContext->inputLine[MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
            readerContext->lineNumber = readInt32LE(input);
            readerContext->isPipe = static_cast<char>(readByte(input));
            readerContextPrevIndex.set(static_cast<long int>(i), readInt32LE(input));
            readerContext->prev = nullptr;
            readerContexts.set(static_cast<long int>(i), readerContext);
        }
        for ( int i = 0; i < readerContextCount; i++ ) {
            ReaderContext *prev = nullptr;
            if ( !pointerFromIndex(
                     readerContexts,
                     readerContextPrevIndex.get(static_cast<long int>(i)),
                     "readerContext.prev",
                     &prev) ) goto fail;
            readerContexts.get(static_cast<long int>(i))->prev = prev;
        }

        if ( !expectTag(input, "XFAR") ) goto fail;
        for ( int i = 0; i < transformArrayCount; i++ ) {
            TransformArray *transformArray = new TransformArray();
            transformArray->startingPosition.fileId = readInt32LE(input);
            transformArray->startingPosition.lineNumber = readInt32LE(input);
            transformArray->startingPosition.offset = static_cast<long>(readInt64LE(input));
            transformArray->numberOfDimensions = readInt32LE(input);
            for ( int j = 0; j < TRANSFORM_MAXIMUM_DIMENSIONS; j++ ) {
                transformArray->transformArguments[j].i = readInt16LE(input);
                transformArray->transformArguments[j].n = readInt16LE(input);
                readBytes(
                    input,
                    reinterpret_cast<unsigned char *>(transformArray->transformArguments[j].arg),
                    8);
                transformArray->transformArguments[j].arg[7] = '\0';
            }
            transformArrays.set(static_cast<long int>(i), transformArray);
        }

        if ( !expectTag(input, "XFCT") ) goto fail;
        java::ArrayList<int> transformContextArrayIndex;
        java::ArrayList<int> transformContextPrevIndex;
        if ( !initializeArrayList(&transformContextArrayIndex, transformContextCount, static_cast<int>(-1), "transform context array index") ) goto fail;
        if ( !initializeArrayList(&transformContextPrevIndex, transformContextCount, static_cast<int>(-1), "transform context prev index") ) goto fail;
        for ( int i = 0; i < transformContextCount; i++ ) {
            TransformStackContext *transformContext = new TransformStackContext();
            transformContext->xid = static_cast<long>(readInt64LE(input));
            transformContext->xac = readInt16LE(input);
            transformContext->rev = readInt16LE(input);

            for ( int row = 0; row < 4; row++ ) {
                for ( int col = 0; col < 4; col++ ) {
                    transformContext->xf.transformMatrix.m[row][col] = readDoubleLE(input);
                }
            }
            transformContext->xf.scaleFactor = readDoubleLE(input);
            transformContextArrayIndex.set(static_cast<long int>(i), readInt32LE(input));
            transformContextPrevIndex.set(static_cast<long int>(i), readInt32LE(input));
            transformContext->transformationArray = nullptr;
            transformContext->prev = nullptr;
            transformContexts.set(static_cast<long int>(i), transformContext);
        }
        for ( int i = 0; i < transformContextCount; i++ ) {
            TransformStackContext *transformContext = transformContexts.get(static_cast<long int>(i));
            TransformArray *transformArray = nullptr;
            if ( !pointerFromIndex(
                     transformArrays,
                     transformContextArrayIndex.get(static_cast<long int>(i)),
                     "transformContext.transformationArray",
                     &transformArray) ) goto fail;
            transformContext->transformationArray = transformArray;

            TransformStackContext *previous = nullptr;
            if ( !pointerFromIndex(
                     transformContexts,
                     transformContextPrevIndex.get(static_cast<long int>(i)),
                     "transformContext.prev",
                     &previous) ) goto fail;
            transformContext->prev = previous;
        }

        if ( !expectTag(input, "VRTX") ) goto fail;
        for ( int i = 0; i < vertexCount; i++ ) {
            VertexRecord &record = vertexRecords[static_cast<long int>(i)];
            record.id = readInt32LE(input);
            record.pointIndex = readInt32LE(input);
            record.normalIndex = readInt32LE(input);
            record.textureCoordinateIndex = readInt32LE(input);
            if ( !readColor(input, &record.color) ) goto fail;
            record.backIndex = readInt32LE(input);
            record.tmp = readInt32LE(input);
            record.hasRadianceData = readBool(input);
            if ( record.hasRadianceData ) {
                logError("BinaryModelReader::read", "%s", "Vertex radianceData is not supported in binary reader");
                goto fail;
            }
            if ( !readIndexList(input, "vertex.patches", &record.patchIndices) ) goto fail;

            Vector3D *point = nullptr;
            Vector3D *normal = nullptr;
            Vector3D *texCoords = nullptr;
            if ( !pointerFromIndex(vectors, record.pointIndex, "vertex.point", &point) ) goto fail;
            if ( !pointerFromIndex(vectors, record.normalIndex, "vertex.normal", &normal) ) goto fail;
            if ( !pointerFromIndex(vectors, record.textureCoordinateIndex, "vertex.textureCoordinates", &texCoords) ) goto fail;

            Vertex *vertex = new Vertex(point, normal, texCoords, new java::ArrayList<Patch *>());
            vertex->id = record.id;
            vertex->color = record.color;
            vertex->tmp = record.tmp;
            vertex->radianceData = nullptr;
            vertices.set(static_cast<long int>(i), vertex);
        }

        for ( int i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices.get(static_cast<long int>(i));
            const VertexRecord &record = vertexRecords[static_cast<long int>(i)];
            Vertex *back = nullptr;
            if ( !pointerFromIndex(vertices, record.backIndex, "vertex.back", &back) ) goto fail;
            vertex->back = back;
        }

        if ( !expectTag(input, "PTCH") ) goto fail;
        for ( int i = 0; i < patchCount; i++ ) {
            PatchRecord &record = patchRecords[static_cast<long int>(i)];
            record.id = readInt32LE(input);
            record.twinIndex = readInt32LE(input);
            record.numberOfVertices = readInt32LE(input);
            if ( record.numberOfVertices != 3 && record.numberOfVertices != 4 ) {
                logError("BinaryModelReader::read", "%s", "Invalid patch vertex count while loading binary model");
                goto fail;
            }
            for ( int j = 0; j < MAXIMUM_VERTICES_PER_PATCH; j++ ) {
                record.vertexIndices[j] = readInt32LE(input);
            }

            record.hasBoundingBox = readBool(input);
            if ( record.hasBoundingBox ) {
                if ( !readBoundingBoxCoordinates(input, record.boundingBoxCoordinates) ) goto fail;
            }

            if ( !readVector(input, &record.normal) ) goto fail;
            record.planeConstant = readFloatLE(input);
            record.tolerance = readFloatLE(input);
            record.area = readFloatLE(input);
            if ( !readVector(input, &record.midPoint) ) goto fail;

            record.hasJacobian = readBool(input);
            record.jacobianA = 0.0f;
            record.jacobianB = 0.0f;
            record.jacobianC = 0.0f;
            if ( record.hasJacobian ) {
                record.jacobianA = readFloatLE(input);
                record.jacobianB = readFloatLE(input);
                record.jacobianC = readFloatLE(input);
            }

            record.directPotential = readFloatLE(input);
            record.dominantIndex = readInt32LE(input);
            record.omit = readBool(input);
            record.flags = readByte(input);
            if ( !readColor(input, &record.color) ) goto fail;
            record.materialIndex = readInt32LE(input);
            record.hasRadianceData = readBool(input);
            if ( record.hasRadianceData ) {
                logError("BinaryModelReader::read", "%s", "Patch radianceData is not supported in binary reader");
                goto fail;
            }

            Vertex *v1 = nullptr;
            Vertex *v2 = nullptr;
            Vertex *v3 = nullptr;
            Vertex *v4 = nullptr;
            if ( !pointerFromIndex(vertices, record.vertexIndices[0], "patch.vertex[0]", &v1) ) goto fail;
            if ( !pointerFromIndex(vertices, record.vertexIndices[1], "patch.vertex[1]", &v2) ) goto fail;
            if ( !pointerFromIndex(vertices, record.vertexIndices[2], "patch.vertex[2]", &v3) ) goto fail;
            if ( record.numberOfVertices == 4 ) {
                if ( !pointerFromIndex(vertices, record.vertexIndices[3], "patch.vertex[3]", &v4) ) goto fail;
            }

            Patch *patch = new Patch(record.numberOfVertices, v1, v2, v3, v4);
            patch->id = static_cast<unsigned>(record.id);
            patch->normal = record.normal;
            patch->planeConstant = record.planeConstant;
            patch->tolerance = record.tolerance;
            patch->area = record.area;
            patch->midPoint = record.midPoint;
            patch->directPotential = record.directPotential;
            patch->index = static_cast<char>(record.dominantIndex);
            patch->omit = static_cast<char>(record.omit ? 1 : 0);
            patch->setFlags(record.flags);
            patch->color = record.color;
            Material *material = nullptr;
            if ( !pointerFromIndex(materials, record.materialIndex, "patch.material", &material) ) goto fail;
            patch->material = material;
            patch->radianceData = nullptr;

            if ( patch->jacobian != nullptr ) {
                delete patch->jacobian;
                patch->jacobian = nullptr;
            }
            if ( record.hasJacobian ) {
                patch->jacobian = new Jacobian(record.jacobianA, record.jacobianB, record.jacobianC);
            }

            if ( patch->boundingBox != nullptr ) {
                delete patch->boundingBox;
                patch->boundingBox = nullptr;
            }
            if ( record.hasBoundingBox ) {
                patch->boundingBox = new BoundingBox();
                if ( !setBoundingBoxFromCoordinates(patch->boundingBox, record.boundingBoxCoordinates) ) goto fail;
            }

            patches.set(static_cast<long int>(i), patch);
        }

        for ( int i = 0; i < patchCount; i++ ) {
            Patch *patch = patches.get(static_cast<long int>(i));
            const PatchRecord &record = patchRecords[static_cast<long int>(i)];
            Patch *twin = nullptr;
            if ( !pointerFromIndex(patches, record.twinIndex, "patch.twin", &twin) ) goto fail;
            patch->twin = twin;
        }

        for ( int i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices.get(static_cast<long int>(i));
            delete vertex->patches;
            java::ArrayList<Patch *> *patchList = nullptr;
            if ( !arrayListFromIndices(
                     vertexRecords[static_cast<long int>(i)].patchIndices,
                     patches,
                     "vertex.patches",
                     &patchList) ) goto fail;
            vertex->patches = patchList;
        }

        if ( !expectTag(input, "GEOM") ) goto fail;
        for ( int i = 0; i < geometryCount; i++ ) {
            GeometryRecord &record = geometryRecords[static_cast<long int>(i)];
            record.classId = readInt32LE(input);
            record.id = readInt32LE(input);
            record.itemCount = readInt32LE(input);
            record.bounded = readBool(input);
            record.shaftCullGeometry = readBool(input);
            record.omit = readBool(input);
            record.isDuplicate = readBool(input);
            if ( !readBoundingBoxCoordinates(input, record.boundingBoxCoordinates) ) goto fail;
            record.hasRayIntersectionBox = readBool(input);
            record.hasRadianceData = readBool(input);
            if ( record.hasRadianceData ) {
                logError("BinaryModelReader::read", "%s", "Geometry radianceData is not supported in binary reader");
                goto fail;
            }

            record.hasObjectName = false;
            if ( record.objectName != nullptr ) {
                delete[] record.objectName;
                record.objectName = nullptr;
            }
            record.meshId = 0;
            record.materialIndex = -1;

            if ( record.classId == static_cast<int>(GeometryClassId::SURFACE_MESH) ) {
                if ( !readNullableString(input, &record.objectName, &record.hasObjectName) ) goto fail;
                record.meshId = readInt32LE(input);
                record.materialIndex = readInt32LE(input);
                if ( !readIndexList(input, "surface.positions", &record.positions) ) goto fail;
                if ( !readIndexList(input, "surface.normals", &record.normals) ) goto fail;
                if ( !readIndexList(input, "surface.vertices", &record.vertices) ) goto fail;
                if ( !readIndexList(input, "surface.faces", &record.faces) ) goto fail;
            } else if ( record.classId == static_cast<int>(GeometryClassId::COMPOUND) ) {
                if ( !readIndexList(input, "compound.children", &record.children) ) goto fail;
            } else if ( record.classId == static_cast<int>(GeometryClassId::PATCH_SET) ) {
                if ( !readIndexList(input, "patchSet.patchList", &record.patchSetPatches) ) goto fail;
            } else {
                logError("BinaryModelReader::read", "%s", "Unsupported geometry type in binary model");
                goto fail;
            }
        }

        for ( int i = 0; i < geometryCount; i++ ) {
            const GeometryRecord &record = geometryRecords[static_cast<long int>(i)];
            Geometry *geometry = nullptr;

            if ( record.classId == static_cast<int>(GeometryClassId::SURFACE_MESH) ) {
                char *objectName = nullptr;
                if ( !duplicateNullableString(record.hasObjectName, record.objectName, &objectName) ) goto fail;

                java::ArrayList<Vector3D *> *positions = nullptr;
                java::ArrayList<Vector3D *> *normals = nullptr;
                java::ArrayList<Vertex *> *surfaceVertices = nullptr;
                java::ArrayList<Patch *> *faces = nullptr;
                Material *material = nullptr;
                if ( !arrayListFromIndices(record.positions, vectors, "surface.positions", &positions) ) goto fail;
                if ( !arrayListFromIndices(record.normals, vectors, "surface.normals", &normals) ) goto fail;
                if ( !arrayListFromIndices(record.vertices, vertices, "surface.vertices", &surfaceVertices) ) goto fail;
                if ( !arrayListFromIndices(record.faces, patches, "surface.faces", &faces) ) goto fail;
                if ( !pointerFromIndex(materials, record.materialIndex, "surface.material", &material) ) goto fail;

                MeshSurface *surface = new MeshSurface(
                    objectName,
                    material,
                    positions,
                    normals,
                    nullptr,
                    surfaceVertices,
                    faces,
                    MaterialColorFlags::NO_COLORS);
                surface->meshId = record.meshId;
                geometry = surface;
            } else if ( record.classId == static_cast<int>(GeometryClassId::COMPOUND) ) {
                geometry = new Compound(new java::ArrayList<Geometry *>());
            } else if ( record.classId == static_cast<int>(GeometryClassId::PATCH_SET) ) {
                java::ArrayList<Patch *> *patchList = nullptr;
                if ( !arrayListFromIndices(record.patchSetPatches, patches, "patchSet.patchList", &patchList) ) goto fail;
                geometry = new PatchSet(patchList);
                delete patchList;
            }

            if ( geometry == nullptr ) {
                logError("BinaryModelReader::read", "%s", "Could not instantiate geometry while loading binary model");
                goto fail;
            }

            geometry->className = static_cast<GeometryClassId>(record.classId);
            geometry->id = record.id;
            geometry->itemCount = record.itemCount;
            geometry->bounded = static_cast<char>(record.bounded ? 1 : 0);
            geometry->shaftCullGeometry = static_cast<char>(record.shaftCullGeometry ? 1 : 0);
            geometry->omit = static_cast<char>(record.omit ? 1 : 0);
            geometry->isDuplicate = record.isDuplicate;
            if ( !setBoundingBoxFromCoordinates(&geometry->boundingBox, record.boundingBoxCoordinates) ) goto fail;

            if ( record.hasRayIntersectionBox ) {
                if ( geometry->rayIntersectionBox == nullptr ) {
                    geometry->rayIntersectionBox = new MinMaxBox(&geometry->boundingBox);
                } else {
                    geometry->rayIntersectionBox->updateFromBoundingBox(&geometry->boundingBox);
                }
            } else if ( geometry->rayIntersectionBox != nullptr ) {
                delete geometry->rayIntersectionBox;
                geometry->rayIntersectionBox = nullptr;
            }

            geometry->radianceData = nullptr;
            geometries.set(static_cast<long int>(i), geometry);
        }

        for ( int i = 0; i < geometryCount; i++ ) {
            const GeometryRecord &record = geometryRecords[static_cast<long int>(i)];
            if ( record.classId == static_cast<int>(GeometryClassId::COMPOUND) ) {
                Compound *compound = static_cast<Compound *>(geometries.get(static_cast<long int>(i)));
                delete compound->children;
                java::ArrayList<Geometry *> *children = nullptr;
                if ( !arrayListFromIndices(record.children, geometries, "compound.children", &children) ) goto fail;
                compound->children = children;
            }
        }

        if ( !expectTag(input, "MODL") ) goto fail;
        modelRecord.currentColorIndex = readInt32LE(input);
        if ( !readNullableString(input, &modelRecord.currentMaterialName, &modelRecord.hasCurrentMaterialName) ) goto fail;
        if ( !readNullableString(input, &modelRecord.currentObjectName, &modelRecord.hasCurrentObjectName) ) goto fail;
        if ( !readNullableString(input, &modelRecord.currentVertexName, &modelRecord.hasCurrentVertexName) ) goto fail;
        modelRecord.geometryStackHeadIndex = readInt32LE(input);
        modelRecord.inComplex = readBool(input);
        modelRecord.inSurface = readBool(input);
        modelRecord.monochrome = readBool(input);
        modelRecord.readerContextIndex = readInt32LE(input);
        modelRecord.transformContextIndex = readInt32LE(input);

        if ( !readIndexList(input, "model.currentFaceList", &modelRecord.currentFaceList) ) goto fail;
        if ( !readIndexList(input, "model.currentGeometryList", &modelRecord.currentGeometryList) ) goto fail;
        if ( !readIndexList(input, "model.currentNormalList", &modelRecord.currentNormalList) ) goto fail;
        if ( !readIndexList(input, "model.currentPointList", &modelRecord.currentPointList) ) goto fail;
        if ( !readIndexList(input, "model.currentVertexList", &modelRecord.currentVertexList) ) goto fail;
        if ( !readIndexList(input, "model.geometries", &modelRecord.geometries) ) goto fail;
        if ( !readIndexList(input, "model.materials", &modelRecord.materials) ) goto fail;

        model = new PersistedSceneModel();
        ColorContext *modelCurrentColor = nullptr;
        ReaderContext *modelReaderContext = nullptr;
        TransformStackContext *modelTransformContext = nullptr;
        if ( !pointerFromIndex(colorContexts, modelRecord.currentColorIndex, "model.currentColor", &modelCurrentColor) ) goto fail;
        if ( !pointerFromIndex(readerContexts, modelRecord.readerContextIndex, "model.readerContext", &modelReaderContext) ) goto fail;
        if ( !pointerFromIndex(transformContexts, modelRecord.transformContextIndex, "model.transformContext", &modelTransformContext) ) goto fail;
        model->currentColor = modelCurrentColor;
        model->geometryStackHeadIndex = modelRecord.geometryStackHeadIndex;
        model->inComplex = modelRecord.inComplex;
        model->inSurface = modelRecord.inSurface;
        model->monochrome = modelRecord.monochrome;
        model->readerContext = modelReaderContext;
        model->transformContext = modelTransformContext;

        if ( !populateModelStrings(model, modelRecord) ) goto fail;

        if ( !arrayListFromIndices(modelRecord.currentFaceList, patches, "model.currentFaceList", &model->currentFaceList) ) goto fail;
        if ( !arrayListFromIndices(modelRecord.currentGeometryList, geometries, "model.currentGeometryList", &model->currentGeometryList) ) goto fail;
        if ( !arrayListFromIndices(modelRecord.currentNormalList, vectors, "model.currentNormalList", &model->currentNormalList) ) goto fail;
        if ( !arrayListFromIndices(modelRecord.currentPointList, vectors, "model.currentPointList", &model->currentPointList) ) goto fail;
        if ( !arrayListFromIndices(modelRecord.currentVertexList, vertices, "model.currentVertexList", &model->currentVertexList) ) goto fail;
        if ( !arrayListFromIndices(modelRecord.geometries, geometries, "model.geometries", &model->geometries) ) goto fail;
        if ( !arrayListFromIndices(modelRecord.materials, materials, "model.materials", &model->materials) ) goto fail;

        int maxPatchId = 0;
        for ( long int i = 0; i < patches.size(); i++ ) {
            Patch *patch = patches.get(i);
            if ( patch != nullptr && static_cast<int>(patch->id) > maxPatchId ) {
                maxPatchId = static_cast<int>(patch->id);
            }
        }
        Patch::setNextId(maxPatchId + 1);

        int maxGeometryId = -1;
        for ( long int i = 0; i < geometries.size(); i++ ) {
            Geometry *geometry = geometries.get(i);
            if ( geometry != nullptr && geometry->id > maxGeometryId ) {
                maxGeometryId = geometry->id;
            }
        }
        Geometry::nextGeometryId = maxGeometryId + 1;

        ok = true;
    } catch (...) {
        logError("BinaryModelReader::read", "%s", "Unexpected failure while reading binary model");
        ok = false;
    }

fail:
    input.dispose();
    releaseVertexRecordIndexLists(vertexRecords);
    releaseGeometryRecordIndexLists(geometryRecords);
    releaseModelRecordIndexLists(&modelRecord);

    if ( !ok ) {
        cleanupPartialModel(
            vectors,
            vertices,
            patches,
            materials,
            geometries,
            colorContexts,
            readerContexts,
            transformArrays,
            transformContexts,
            model);
        return nullptr;
    }

    return model;
}
