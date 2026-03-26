#include "io/bin/BinaryModelReader.h"

#include "java/io/BufferedInputStream.h"
#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/ColorRgb.h"
#include "common/linealAlgebra/Jacobian.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/PersistenceElement.h"
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
const int32_t BinaryModelReader::BINARY_MODEL_VERSION = 1;

class BinaryModelReader::IndexListRecord {
  public:
    bool isNull;
    java::ArrayList<int32_t> *indices;

    IndexListRecord():
        isNull(true),
        indices(nullptr)
    {
    }
};

class BinaryModelReader::VertexRecord {
  public:
    int32_t id;
    int32_t pointIndex;
    int32_t normalIndex;
    int32_t textureCoordinateIndex;
    ColorRgb color;
    int32_t backIndex;
    int32_t tmp;
    bool hasRadianceData;
    IndexListRecord patchIndices;
};

class BinaryModelReader::PatchRecord {
  public:
    int32_t id;
    int32_t twinIndex;
    int32_t numberOfVertices;
    int32_t vertexIndices[MAXIMUM_VERTICES_PER_PATCH];
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
    int32_t dominantIndex;
    bool omit;
    unsigned char flags;
    ColorRgb color;
    int32_t materialIndex;
    bool hasRadianceData;
};

class BinaryModelReader::GeometryRecord {
  public:
    int32_t classId;
    int32_t id;
    int32_t itemCount;
    bool bounded;
    bool shaftCullGeometry;
    bool omit;
    bool isDuplicate;
    float boundingBoxCoordinates[6];
    bool hasRayIntersectionBox;
    bool hasRadianceData;

    bool hasObjectName;
    char *objectName;
    int32_t meshId;
    int32_t materialIndex;
    IndexListRecord positions;
    IndexListRecord normals;
    IndexListRecord vertices;
    IndexListRecord faces;

    IndexListRecord children;
    IndexListRecord patchSetPatches;
};

class BinaryModelReader::ModelRecord {
  public:
    int32_t currentColorIndex;
    bool hasCurrentMaterialName;
    char *currentMaterialName;
    bool hasCurrentObjectName;
    char *currentObjectName;
    bool hasCurrentVertexName;
    char *currentVertexName;
    int32_t geometryStackHeadIndex;
    bool inComplex;
    bool inSurface;
    bool monochrome;
    int32_t readerContextIndex;
    int32_t transformContextIndex;

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
void
BinaryModelReader::initializeArrayList(java::ArrayList<T> *list, int32_t count, T initialValue, const char *what) {
    (void)what;
    if ( list == nullptr ) {
        throw std::runtime_error("Null list pointer");
    }
    if ( count < 0 ) {
        throw std::runtime_error("Negative count while reading binary model");
    }
    for ( int32_t i = 0; i < count; i++ ) {
        if ( !list->add(initialValue) ) {
            throw std::runtime_error("Failed to allocate entries while reading binary model");
        }
    }
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

void
BinaryModelReader::readBytesChunked(java::io::InputStream &input, unsigned char *buffer, int64_t length) {
    if ( length <= 0 ) {
        return;
    }
    if ( length > static_cast<int64_t>(std::numeric_limits<size_t>::max()) ) {
        throw std::runtime_error("Requested read length too large");
    }

    int64_t offset = 0;
    const int64_t maxChunk = static_cast<int64_t>(std::numeric_limits<int>::max());
    while ( offset < length ) {
        const int chunk = static_cast<int>(std::min(maxChunk, length - offset));
        readBytes(input, buffer + offset, chunk);
        offset += chunk;
    }
}

unsigned char
BinaryModelReader::readByte(java::io::InputStream &input) {
    return static_cast<unsigned char>(vsdk::PersistenceElement::readByteUnsignedInt(input));
}

bool
BinaryModelReader::readBool(java::io::InputStream &input) {
    return readByte(input) != 0;
}

int16_t
BinaryModelReader::readInt16LE(java::io::InputStream &input) {
    return static_cast<int16_t>(vsdk::PersistenceElement::readSignedShortLE(input));
}

int32_t
BinaryModelReader::readInt32LE(java::io::InputStream &input) {
    const long value = vsdk::PersistenceElement::readLongLE(input);
    return static_cast<int32_t>(value);
}

int64_t
BinaryModelReader::readInt64LE(java::io::InputStream &input) {
    unsigned char bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    readBytes(input, bytes, 8);
    uint64_t value = 0;
    for ( int i = 0; i < 8; i++ ) {
        value |= static_cast<uint64_t>(bytes[i]) << (8 * i);
    }
    return static_cast<int64_t>(value);
}

float
BinaryModelReader::readFloatLE(java::io::InputStream &input) {
    return vsdk::PersistenceElement::readFloatLE(input);
}

double
BinaryModelReader::readDoubleLE(java::io::InputStream &input) {
    return vsdk::PersistenceElement::readDoubleLE(input);
}

void
BinaryModelReader::expectTag(java::io::InputStream &input, const char expected[4]) {
    unsigned char tag[4] = {0, 0, 0, 0};
    readBytes(input, tag, 4);
    if ( tag[0] != static_cast<unsigned char>(expected[0])
         || tag[1] != static_cast<unsigned char>(expected[1])
         || tag[2] != static_cast<unsigned char>(expected[2])
         || tag[3] != static_cast<unsigned char>(expected[3]) ) {
        throw std::runtime_error("Unexpected section tag while reading binary model");
    }
}

int32_t
BinaryModelReader::readNonNegativeCount(java::io::InputStream &input, const char *what) {
    (void)what;
    const int32_t count = readInt32LE(input);
    if ( count < 0 ) {
        throw std::runtime_error("Negative count while reading binary model");
    }
    return count;
}

bool
BinaryModelReader::readNullableString(java::io::InputStream &input, char **value) {
    if ( value == nullptr ) {
        throw std::runtime_error("Null string output pointer");
    }
    if ( *value != nullptr ) {
        delete[] *value;
        *value = nullptr;
    }

    const int32_t size = readInt32LE(input);
    if ( size == -1 ) {
        return false;
    }
    if ( size < -1 ) {
        throw std::runtime_error("Invalid negative string size");
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
    return true;
}

char *
BinaryModelReader::duplicateNullableString(bool hasValue, const char *value) {
    if ( !hasValue || value == nullptr ) {
        return nullptr;
    }
    const size_t length = std::strlen(value);
    char *text = new char[length + 1];
    std::memcpy(text, value, length + 1);
    return text;
}

void
BinaryModelReader::readColor(java::io::InputStream &input, ColorRgb *color) {
    if ( color == nullptr ) {
        throw std::runtime_error("Null color output pointer");
    }
    color->r = readFloatLE(input);
    color->g = readFloatLE(input);
    color->b = readFloatLE(input);
}

void
BinaryModelReader::readVector(java::io::InputStream &input, Vector3D *vector) {
    if ( vector == nullptr ) {
        throw std::runtime_error("Null vector output pointer");
    }
    vector->x = readFloatLE(input);
    vector->y = readFloatLE(input);
    vector->z = readFloatLE(input);
}

void
BinaryModelReader::readBoundingBoxCoordinates(java::io::InputStream &input, float coordinates[6]) {
    if ( coordinates == nullptr ) {
        throw std::runtime_error("Null bounding box coordinate buffer");
    }
    for ( int i = 0; i < 6; i++ ) {
        coordinates[i] = readFloatLE(input);
    }
}

void
BinaryModelReader::setBoundingBoxFromCoordinates(BoundingBox *boundingBox, const float coordinates[6]) {
    if ( boundingBox == nullptr || coordinates == nullptr ) {
        throw std::runtime_error("Invalid bounding box assignment");
    }
    BoundingBox parsed;
    Vector3D minPoint;
    Vector3D maxPoint;
    minPoint.set(coordinates[MIN_X], coordinates[MIN_Y], coordinates[MIN_Z]);
    maxPoint.set(coordinates[MAX_X], coordinates[MAX_Y], coordinates[MAX_Z]);
    parsed.enlargeToIncludePoint(&minPoint);
    parsed.enlargeToIncludePoint(&maxPoint);
    boundingBox->copyFrom(&parsed);
}

BinaryModelReader::IndexListRecord
BinaryModelReader::readIndexList(java::io::InputStream &input, const char *what) {
    (void)what;
    IndexListRecord record;
    record.isNull = false;
    record.indices = nullptr;

    const int32_t count = readInt32LE(input);
    if ( count == -1 ) {
        record.isNull = true;
        return record;
    }
    if ( count < -1 ) {
        throw std::runtime_error("Negative index list count while reading binary model");
    }

    record.indices = new java::ArrayList<int32_t>(count > 0 ? static_cast<long int>(count) : 1);
    for ( int32_t i = 0; i < count; i++ ) {
        if ( !record.indices->add(readInt32LE(input)) ) {
            delete record.indices;
            record.indices = nullptr;
            throw std::runtime_error("Failed to allocate index list while reading binary model");
        }
    }

    return record;
}

template <typename T>
T *
BinaryModelReader::pointerFromIndex(const java::ArrayList<T *> &values, int32_t index, const char *what) {
    (void)what;
    if ( index == -1 ) {
        return nullptr;
    }
    if ( index < 0 || static_cast<long int>(index) >= values.size() ) {
        throw std::runtime_error("Out of range index while reading binary model");
    }
    return values.get(static_cast<long int>(index));
}

template <typename T>
java::ArrayList<T *> *
BinaryModelReader::arrayListFromIndices(
    const BinaryModelReader::IndexListRecord &record,
    const java::ArrayList<T *> &values,
    const char *what)
{
    if ( record.isNull ) {
        return nullptr;
    }
    if ( record.indices == nullptr ) {
        throw std::runtime_error("Missing index list while reading binary model");
    }
    java::ArrayList<T *> *list = new java::ArrayList<T *>();
    for ( long int i = 0; i < record.indices->size(); i++ ) {
        if ( !list->add(pointerFromIndex(values, record.indices->get(i), what)) ) {
            delete list;
            throw std::runtime_error("Failed to allocate output list while reading binary model");
        }
    }
    return list;
}

void
BinaryModelReader::validateBinaryHeader(java::io::InputStream &input) {
    unsigned char magic[16] = {0};
    readBytes(input, magic, 16);
    if ( std::memcmp(magic, BINARY_MODEL_MAGIC, 16) != 0 ) {
        throw std::runtime_error("Invalid binary model magic header");
    }

    const int32_t version = readInt32LE(input);
    if ( version != BINARY_MODEL_VERSION ) {
        throw std::runtime_error("Unsupported binary model version");
    }

    const int32_t pointerSize = readInt32LE(input);
    const int32_t longSize = readInt32LE(input);
    const int32_t modelSize = readInt32LE(input);

    if ( pointerSize != static_cast<int32_t>(sizeof(void *))
         || longSize != static_cast<int32_t>(sizeof(long))
         || modelSize != static_cast<int32_t>(sizeof(PersistedSceneModel)) ) {
        throw std::runtime_error("Incompatible binary model platform/type sizes");
    }
}

void
BinaryModelReader::populateModelStrings(PersistedSceneModel *model, const BinaryModelReader::ModelRecord &record) {
    if ( model == nullptr ) {
        throw std::runtime_error("Null model in string population");
    }

    model->currentMaterialName = duplicateNullableString(
        record.hasCurrentMaterialName,
        record.currentMaterialName);
    model->currentObjectName = duplicateNullableString(
        record.hasCurrentObjectName,
        record.currentObjectName);
    model->currentVertexName = duplicateNullableString(
        record.hasCurrentVertexName,
        record.currentVertexName);
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

    java::io::BufferedInputStream input;
    if ( !input.open(fileName) ) {
        return nullptr;
    }

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

    try {
        validateBinaryHeader(input);

        const int32_t vectorCount = readNonNegativeCount(input, "vectors");
        const int32_t vertexCount = readNonNegativeCount(input, "vertices");
        const int32_t patchCount = readNonNegativeCount(input, "patches");
        const int32_t materialCount = readNonNegativeCount(input, "materials");
        const int32_t geometryCount = readNonNegativeCount(input, "geometries");
        const int32_t colorContextCount = readNonNegativeCount(input, "color contexts");
        const int32_t readerContextCount = readNonNegativeCount(input, "reader contexts");
        const int32_t transformArrayCount = readNonNegativeCount(input, "transform arrays");
        const int32_t transformContextCount = readNonNegativeCount(input, "transform contexts");

        initializeArrayList(&vectors, vectorCount, static_cast<Vector3D *>(nullptr), "vectors");
        initializeArrayList(&vertices, vertexCount, static_cast<Vertex *>(nullptr), "vertices");
        initializeArrayList(&patches, patchCount, static_cast<Patch *>(nullptr), "patches");
        initializeArrayList(&materials, materialCount, static_cast<Material *>(nullptr), "materials");
        initializeArrayList(&geometries, geometryCount, static_cast<Geometry *>(nullptr), "geometries");
        initializeArrayList(&colorContexts, colorContextCount, static_cast<ColorContext *>(nullptr), "color contexts");
        initializeArrayList(&readerContexts, readerContextCount, static_cast<ReaderContext *>(nullptr), "reader contexts");
        initializeArrayList(&transformArrays, transformArrayCount, static_cast<TransformArray *>(nullptr), "transform arrays");
        initializeArrayList(&transformContexts, transformContextCount, static_cast<TransformStackContext *>(nullptr), "transform contexts");
        initializeArrayList(&vertexRecords, vertexCount, VertexRecord(), "vertex records");
        initializeArrayList(&patchRecords, patchCount, PatchRecord(), "patch records");
        initializeArrayList(&geometryRecords, geometryCount, GeometryRecord(), "geometry records");

        expectTag(input, "VEC3");
        for ( int32_t i = 0; i < vectorCount; i++ ) {
            Vector3D *vector = new Vector3D();
            readVector(input, vector);
            vectors.set(static_cast<long int>(i), vector);
        }

        expectTag(input, "MTLS");
        for ( int32_t i = 0; i < materialCount; i++ ) {
            char *materialName = nullptr;
            const bool hasMaterialName = readNullableString(input, &materialName);
            std::unique_ptr<char[]> materialNameGuard(materialName);
            const bool sided = readBool(input);

            PhongEmittanceDistributionFunction *edf = nullptr;
            const bool hasEdf = readBool(input);
            if ( hasEdf ) {
                ColorRgb kd;
                ColorRgb ks;
                readColor(input, &kd);
                readColor(input, &ks);
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
                    readColor(input, &kd);
                    readColor(input, &ks);
                    const float ns = readFloatLE(input);
                    brdf = new PhongBidirectionalReflectanceDistributionFunction(&kd, &ks, ns);
                }

                const bool hasBtdf = readBool(input);
                if ( hasBtdf ) {
                    ColorRgb kd;
                    ColorRgb ks;
                    readColor(input, &kd);
                    readColor(input, &ks);
                    const float ns = readFloatLE(input);
                    const float nr = readFloatLE(input);
                    const float ni = readFloatLE(input);
                    btdf = new PhongBidirectionalTransmittanceDistributionFunction(&kd, &ks, ns, nr, ni);
                }

                const bool hasTexture = readBool(input);
                if ( hasTexture ) {
                    const int32_t width = readInt32LE(input);
                    const int32_t height = readInt32LE(input);
                    const int32_t channels = readInt32LE(input);
                    const int64_t dataBytes = readInt64LE(input);

                    if ( width < 0 || height < 0 || channels < 0 || dataBytes < 0 ) {
                        throw std::runtime_error("Invalid texture dimensions in binary material");
                    }

                    const int64_t expectedBytes = static_cast<int64_t>(width)
                                                  * static_cast<int64_t>(height)
                                                  * static_cast<int64_t>(channels);
                    if ( expectedBytes != dataBytes ) {
                        throw std::runtime_error("Texture byte count mismatch in binary material");
                    }

                    std::unique_ptr<unsigned char[]> textureData;
                    if ( dataBytes > 0 ) {
                        if ( dataBytes > static_cast<int64_t>(std::numeric_limits<size_t>::max()) ) {
                            throw std::runtime_error("Texture data too large for current platform");
                        }
                        textureData = std::unique_ptr<unsigned char[]>(new unsigned char[static_cast<size_t>(dataBytes)]);
                        readBytesChunked(input, textureData.get(), dataBytes);
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

        expectTag(input, "COLR");
        for ( int32_t i = 0; i < colorContextCount; i++ ) {
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

        expectTag(input, "RCTX");
        java::ArrayList<int32_t> readerContextPrevIndex;
        initializeArrayList(&readerContextPrevIndex, readerContextCount, static_cast<int32_t>(-1), "reader context prev index");
        for ( int32_t i = 0; i < readerContextCount; i++ ) {
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
        for ( int32_t i = 0; i < readerContextCount; i++ ) {
            readerContexts.get(static_cast<long int>(i))->prev = pointerFromIndex(
                readerContexts,
                readerContextPrevIndex.get(static_cast<long int>(i)),
                "readerContext.prev");
        }

        expectTag(input, "XFAR");
        for ( int32_t i = 0; i < transformArrayCount; i++ ) {
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

        expectTag(input, "XFCT");
        java::ArrayList<int32_t> transformContextArrayIndex;
        java::ArrayList<int32_t> transformContextPrevIndex;
        initializeArrayList(&transformContextArrayIndex, transformContextCount, static_cast<int32_t>(-1), "transform context array index");
        initializeArrayList(&transformContextPrevIndex, transformContextCount, static_cast<int32_t>(-1), "transform context prev index");
        for ( int32_t i = 0; i < transformContextCount; i++ ) {
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
        for ( int32_t i = 0; i < transformContextCount; i++ ) {
            TransformStackContext *transformContext = transformContexts.get(static_cast<long int>(i));
            transformContext->transformationArray = pointerFromIndex(
                transformArrays,
                transformContextArrayIndex.get(static_cast<long int>(i)),
                "transformContext.transformationArray");
            transformContext->prev = pointerFromIndex(
                transformContexts,
                transformContextPrevIndex.get(static_cast<long int>(i)),
                "transformContext.prev");
        }

        expectTag(input, "VRTX");
        for ( int32_t i = 0; i < vertexCount; i++ ) {
            VertexRecord &record = vertexRecords[static_cast<long int>(i)];
            record.id = readInt32LE(input);
            record.pointIndex = readInt32LE(input);
            record.normalIndex = readInt32LE(input);
            record.textureCoordinateIndex = readInt32LE(input);
            readColor(input, &record.color);
            record.backIndex = readInt32LE(input);
            record.tmp = readInt32LE(input);
            record.hasRadianceData = readBool(input);
            if ( record.hasRadianceData ) {
                throw std::runtime_error("Vertex radianceData is not supported in binary reader");
            }
            record.patchIndices = readIndexList(input, "vertex.patches");

            Vertex *vertex = new Vertex(
                pointerFromIndex(vectors, record.pointIndex, "vertex.point"),
                pointerFromIndex(vectors, record.normalIndex, "vertex.normal"),
                pointerFromIndex(vectors, record.textureCoordinateIndex, "vertex.textureCoordinates"),
                new java::ArrayList<Patch *>());
            vertex->id = record.id;
            vertex->color = record.color;
            vertex->tmp = record.tmp;
            vertex->radianceData = nullptr;
            vertices.set(static_cast<long int>(i), vertex);
        }

        for ( int32_t i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices.get(static_cast<long int>(i));
            const VertexRecord &record = vertexRecords[static_cast<long int>(i)];
            vertex->back = pointerFromIndex(vertices, record.backIndex, "vertex.back");
        }

        expectTag(input, "PTCH");
        for ( int32_t i = 0; i < patchCount; i++ ) {
            PatchRecord &record = patchRecords[static_cast<long int>(i)];
            record.id = readInt32LE(input);
            record.twinIndex = readInt32LE(input);
            record.numberOfVertices = readInt32LE(input);
            if ( record.numberOfVertices != 3 && record.numberOfVertices != 4 ) {
                throw std::runtime_error("Invalid patch vertex count while loading binary model");
            }
            for ( int j = 0; j < MAXIMUM_VERTICES_PER_PATCH; j++ ) {
                record.vertexIndices[j] = readInt32LE(input);
            }

            record.hasBoundingBox = readBool(input);
            if ( record.hasBoundingBox ) {
                readBoundingBoxCoordinates(input, record.boundingBoxCoordinates);
            }

            readVector(input, &record.normal);
            record.planeConstant = readFloatLE(input);
            record.tolerance = readFloatLE(input);
            record.area = readFloatLE(input);
            readVector(input, &record.midPoint);

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
            readColor(input, &record.color);
            record.materialIndex = readInt32LE(input);
            record.hasRadianceData = readBool(input);
            if ( record.hasRadianceData ) {
                throw std::runtime_error("Patch radianceData is not supported in binary reader");
            }

            Vertex *v1 = pointerFromIndex(vertices, record.vertexIndices[0], "patch.vertex[0]");
            Vertex *v2 = pointerFromIndex(vertices, record.vertexIndices[1], "patch.vertex[1]");
            Vertex *v3 = pointerFromIndex(vertices, record.vertexIndices[2], "patch.vertex[2]");
            Vertex *v4 = nullptr;
            if ( record.numberOfVertices == 4 ) {
                v4 = pointerFromIndex(vertices, record.vertexIndices[3], "patch.vertex[3]");
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
            patch->material = pointerFromIndex(materials, record.materialIndex, "patch.material");
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
                setBoundingBoxFromCoordinates(patch->boundingBox, record.boundingBoxCoordinates);
            }

            patches.set(static_cast<long int>(i), patch);
        }

        for ( int32_t i = 0; i < patchCount; i++ ) {
            Patch *patch = patches.get(static_cast<long int>(i));
            const PatchRecord &record = patchRecords[static_cast<long int>(i)];
            patch->twin = pointerFromIndex(patches, record.twinIndex, "patch.twin");
        }

        for ( int32_t i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices.get(static_cast<long int>(i));
            delete vertex->patches;
            vertex->patches = arrayListFromIndices(
                vertexRecords[static_cast<long int>(i)].patchIndices,
                patches,
                "vertex.patches");
        }

        expectTag(input, "GEOM");
        for ( int32_t i = 0; i < geometryCount; i++ ) {
            GeometryRecord &record = geometryRecords[static_cast<long int>(i)];
            record.classId = readInt32LE(input);
            record.id = readInt32LE(input);
            record.itemCount = readInt32LE(input);
            record.bounded = readBool(input);
            record.shaftCullGeometry = readBool(input);
            record.omit = readBool(input);
            record.isDuplicate = readBool(input);
            readBoundingBoxCoordinates(input, record.boundingBoxCoordinates);
            record.hasRayIntersectionBox = readBool(input);
            record.hasRadianceData = readBool(input);
            if ( record.hasRadianceData ) {
                throw std::runtime_error("Geometry radianceData is not supported in binary reader");
            }

            record.hasObjectName = false;
            if ( record.objectName != nullptr ) {
                delete[] record.objectName;
                record.objectName = nullptr;
            }
            record.meshId = 0;
            record.materialIndex = -1;

            if ( record.classId == static_cast<int32_t>(GeometryClassId::SURFACE_MESH) ) {
                record.hasObjectName = readNullableString(input, &record.objectName);
                record.meshId = readInt32LE(input);
                record.materialIndex = readInt32LE(input);
                record.positions = readIndexList(input, "surface.positions");
                record.normals = readIndexList(input, "surface.normals");
                record.vertices = readIndexList(input, "surface.vertices");
                record.faces = readIndexList(input, "surface.faces");
            } else if ( record.classId == static_cast<int32_t>(GeometryClassId::COMPOUND) ) {
                record.children = readIndexList(input, "compound.children");
            } else if ( record.classId == static_cast<int32_t>(GeometryClassId::PATCH_SET) ) {
                record.patchSetPatches = readIndexList(input, "patchSet.patchList");
            } else {
                throw std::runtime_error("Unsupported geometry type in binary model");
            }
        }

        for ( int32_t i = 0; i < geometryCount; i++ ) {
            const GeometryRecord &record = geometryRecords[static_cast<long int>(i)];
            Geometry *geometry = nullptr;

            if ( record.classId == static_cast<int32_t>(GeometryClassId::SURFACE_MESH) ) {
                char *objectName = duplicateNullableString(record.hasObjectName, record.objectName);
                java::ArrayList<Vector3D *> *positions = arrayListFromIndices(record.positions, vectors, "surface.positions");
                java::ArrayList<Vector3D *> *normals = arrayListFromIndices(record.normals, vectors, "surface.normals");
                java::ArrayList<Vertex *> *surfaceVertices = arrayListFromIndices(record.vertices, vertices, "surface.vertices");
                java::ArrayList<Patch *> *faces = arrayListFromIndices(record.faces, patches, "surface.faces");
                Material *material = pointerFromIndex(materials, record.materialIndex, "surface.material");

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
            } else if ( record.classId == static_cast<int32_t>(GeometryClassId::COMPOUND) ) {
                geometry = new Compound(new java::ArrayList<Geometry *>());
            } else if ( record.classId == static_cast<int32_t>(GeometryClassId::PATCH_SET) ) {
                java::ArrayList<Patch *> *patchList = arrayListFromIndices(
                    record.patchSetPatches,
                    patches,
                    "patchSet.patchList");
                geometry = new PatchSet(patchList);
                delete patchList;
            }

            if ( geometry == nullptr ) {
                throw std::runtime_error("Could not instantiate geometry while loading binary model");
            }

            geometry->className = static_cast<GeometryClassId>(record.classId);
            geometry->id = record.id;
            geometry->itemCount = record.itemCount;
            geometry->bounded = static_cast<char>(record.bounded ? 1 : 0);
            geometry->shaftCullGeometry = static_cast<char>(record.shaftCullGeometry ? 1 : 0);
            geometry->omit = static_cast<char>(record.omit ? 1 : 0);
            geometry->isDuplicate = record.isDuplicate;
            setBoundingBoxFromCoordinates(&geometry->boundingBox, record.boundingBoxCoordinates);

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

        for ( int32_t i = 0; i < geometryCount; i++ ) {
            const GeometryRecord &record = geometryRecords[static_cast<long int>(i)];
            if ( record.classId == static_cast<int32_t>(GeometryClassId::COMPOUND) ) {
                Compound *compound = static_cast<Compound *>(geometries.get(static_cast<long int>(i)));
                delete compound->children;
                compound->children = arrayListFromIndices(record.children, geometries, "compound.children");
            }
        }

        expectTag(input, "MODL");
        modelRecord.currentColorIndex = readInt32LE(input);
        modelRecord.hasCurrentMaterialName = readNullableString(input, &modelRecord.currentMaterialName);
        modelRecord.hasCurrentObjectName = readNullableString(input, &modelRecord.currentObjectName);
        modelRecord.hasCurrentVertexName = readNullableString(input, &modelRecord.currentVertexName);
        modelRecord.geometryStackHeadIndex = readInt32LE(input);
        modelRecord.inComplex = readBool(input);
        modelRecord.inSurface = readBool(input);
        modelRecord.monochrome = readBool(input);
        modelRecord.readerContextIndex = readInt32LE(input);
        modelRecord.transformContextIndex = readInt32LE(input);

        modelRecord.currentFaceList = readIndexList(input, "model.currentFaceList");
        modelRecord.currentGeometryList = readIndexList(input, "model.currentGeometryList");
        modelRecord.currentNormalList = readIndexList(input, "model.currentNormalList");
        modelRecord.currentPointList = readIndexList(input, "model.currentPointList");
        modelRecord.currentVertexList = readIndexList(input, "model.currentVertexList");
        modelRecord.geometries = readIndexList(input, "model.geometries");
        modelRecord.materials = readIndexList(input, "model.materials");

        model = new PersistedSceneModel();
        model->currentColor = pointerFromIndex(colorContexts, modelRecord.currentColorIndex, "model.currentColor");
        model->geometryStackHeadIndex = modelRecord.geometryStackHeadIndex;
        model->inComplex = modelRecord.inComplex;
        model->inSurface = modelRecord.inSurface;
        model->monochrome = modelRecord.monochrome;
        model->readerContext = pointerFromIndex(readerContexts, modelRecord.readerContextIndex, "model.readerContext");
        model->transformContext = pointerFromIndex(transformContexts, modelRecord.transformContextIndex, "model.transformContext");

        populateModelStrings(model, modelRecord);

        model->currentFaceList = arrayListFromIndices(modelRecord.currentFaceList, patches, "model.currentFaceList");
        model->currentGeometryList = arrayListFromIndices(modelRecord.currentGeometryList, geometries, "model.currentGeometryList");
        model->currentNormalList = arrayListFromIndices(modelRecord.currentNormalList, vectors, "model.currentNormalList");
        model->currentPointList = arrayListFromIndices(modelRecord.currentPointList, vectors, "model.currentPointList");
        model->currentVertexList = arrayListFromIndices(modelRecord.currentVertexList, vertices, "model.currentVertexList");
        model->geometries = arrayListFromIndices(modelRecord.geometries, geometries, "model.geometries");
        model->materials = arrayListFromIndices(modelRecord.materials, materials, "model.materials");

        int maxPatchId = 0;
        for ( long int i = 0; i < patches.size(); i++ ) {
            Patch *patch = patches.get(i);
            if ( patch != nullptr ) {
                maxPatchId = std::max(maxPatchId, static_cast<int>(patch->id));
            }
        }
        Patch::setNextId(maxPatchId + 1);

        int maxGeometryId = -1;
        for ( long int i = 0; i < geometries.size(); i++ ) {
            Geometry *geometry = geometries.get(i);
            if ( geometry != nullptr ) {
                maxGeometryId = std::max(maxGeometryId, geometry->id);
            }
        }
        Geometry::nextGeometryId = maxGeometryId + 1;

        releaseVertexRecordIndexLists(vertexRecords);
        releaseGeometryRecordIndexLists(geometryRecords);
        releaseModelRecordIndexLists(&modelRecord);

        input.dispose();
        return model;
    } catch ( const std::exception &e ) {
        logError("BinaryModelReader::read", "%s", e.what());
        input.dispose();
        releaseVertexRecordIndexLists(vertexRecords);
        releaseGeometryRecordIndexLists(geometryRecords);
        releaseModelRecordIndexLists(&modelRecord);
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
}
