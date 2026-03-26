#include "io/bin/BinaryModelReader.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/error.h"
#include "java/io/BufferedInputStream.h"
#include "java/util/ArrayList.txx"
#include "common/ColorRgb.h"
#include "common/linealAlgebra/Jacobian.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/PersistenceElement.h"
#include "io/mgf/MgfColorContext.h"
#include "io/mgf/MgfModel.h"
#include "io/mgf/MgfReaderContext.h"
#include "io/mgf/MgfTransformArray.h"
#include "io/mgf/MgfTransformContext.h"
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

namespace {

static const unsigned char BINARY_MODEL_MAGIC[16] = {
    'R', 'P', 'K', 'M', 'G', 'F', 'B', 'I',
    'N', 'W', 'R', 'T', '1', 0, 0, 0
};
static const int32_t BINARY_MODEL_VERSION = 1;

struct IndexListRecord {
    bool isNull;
    std::vector<int32_t> indices;
};

struct VertexRecord {
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

struct PatchRecord {
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

struct GeometryRecord {
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
    std::string objectName;
    int32_t meshId;
    int32_t materialIndex;
    IndexListRecord positions;
    IndexListRecord normals;
    IndexListRecord vertices;
    IndexListRecord faces;

    IndexListRecord children;
    IndexListRecord patchSetPatches;
};

struct ModelRecord {
    int32_t currentColorIndex;
    bool hasCurrentMaterialName;
    std::string currentMaterialName;
    bool hasCurrentObjectName;
    std::string currentObjectName;
    bool hasCurrentVertexName;
    std::string currentVertexName;
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
};

void
readBytes(java::io::InputStream &input, unsigned char *buffer, int length) {
    if ( length <= 0 ) {
        return;
    }
    vsdk::PersistenceElement::readBytes(input, buffer, length);
}

void
readBytesChunked(java::io::InputStream &input, unsigned char *buffer, int64_t length) {
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
readByte(java::io::InputStream &input) {
    return static_cast<unsigned char>(vsdk::PersistenceElement::readByteUnsignedInt(input));
}

bool
readBool(java::io::InputStream &input) {
    return readByte(input) != 0;
}

int16_t
readInt16LE(java::io::InputStream &input) {
    return static_cast<int16_t>(vsdk::PersistenceElement::readSignedShortLE(input));
}

int32_t
readInt32LE(java::io::InputStream &input) {
    const long value = vsdk::PersistenceElement::readLongLE(input);
    return static_cast<int32_t>(value);
}

int64_t
readInt64LE(java::io::InputStream &input) {
    unsigned char bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    readBytes(input, bytes, 8);
    uint64_t value = 0;
    for ( int i = 0; i < 8; i++ ) {
        value |= static_cast<uint64_t>(bytes[i]) << (8 * i);
    }
    return static_cast<int64_t>(value);
}

float
readFloatLE(java::io::InputStream &input) {
    return vsdk::PersistenceElement::readFloatLE(input);
}

double
readDoubleLE(java::io::InputStream &input) {
    return vsdk::PersistenceElement::readDoubleLE(input);
}

void
expectTag(java::io::InputStream &input, const char expected[4]) {
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
readNonNegativeCount(java::io::InputStream &input, const char *what) {
    const int32_t count = readInt32LE(input);
    if ( count < 0 ) {
        throw std::runtime_error(std::string("Negative count for ") + what);
    }
    return count;
}

bool
readNullableString(java::io::InputStream &input, std::string *value) {
    if ( value == nullptr ) {
        throw std::runtime_error("Null string output pointer");
    }

    const int32_t size = readInt32LE(input);
    if ( size == -1 ) {
        value->clear();
        return false;
    }
    if ( size < -1 ) {
        throw std::runtime_error("Invalid negative string size");
    }

    value->clear();
    value->resize(static_cast<size_t>(size));
    if ( size > 0 ) {
        readBytes(
            input,
            reinterpret_cast<unsigned char *>(&(*value)[0]),
            size);
    }
    return true;
}

char *
duplicateNullableString(bool hasValue, const std::string &value) {
    if ( !hasValue ) {
        return nullptr;
    }
    char *text = new char[value.size() + 1];
    std::memcpy(text, value.c_str(), value.size() + 1);
    return text;
}

void
readColor(java::io::InputStream &input, ColorRgb *color) {
    if ( color == nullptr ) {
        throw std::runtime_error("Null color output pointer");
    }
    color->r = readFloatLE(input);
    color->g = readFloatLE(input);
    color->b = readFloatLE(input);
}

void
readVector(java::io::InputStream &input, Vector3D *vector) {
    if ( vector == nullptr ) {
        throw std::runtime_error("Null vector output pointer");
    }
    vector->x = readFloatLE(input);
    vector->y = readFloatLE(input);
    vector->z = readFloatLE(input);
}

void
readBoundingBoxCoordinates(java::io::InputStream &input, float coordinates[6]) {
    if ( coordinates == nullptr ) {
        throw std::runtime_error("Null bounding box coordinate buffer");
    }
    for ( int i = 0; i < 6; i++ ) {
        coordinates[i] = readFloatLE(input);
    }
}

void
setBoundingBoxFromCoordinates(BoundingBox *boundingBox, const float coordinates[6]) {
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

IndexListRecord
readIndexList(java::io::InputStream &input, const char *what) {
    IndexListRecord record;
    record.isNull = false;

    const int32_t count = readInt32LE(input);
    if ( count == -1 ) {
        record.isNull = true;
        return record;
    }
    if ( count < -1 ) {
        throw std::runtime_error(std::string("Negative index list count for ") + what);
    }

    record.indices.reserve(static_cast<size_t>(count));
    for ( int32_t i = 0; i < count; i++ ) {
        record.indices.push_back(readInt32LE(input));
    }

    return record;
}

template <typename T>
T *
pointerFromIndex(const std::vector<T *> &values, int32_t index, const char *what) {
    if ( index == -1 ) {
        return nullptr;
    }
    if ( index < 0 || static_cast<size_t>(index) >= values.size() ) {
        throw std::runtime_error(std::string("Out of range index for ") + what);
    }
    return values[static_cast<size_t>(index)];
}

template <typename T>
java::ArrayList<T *> *
arrayListFromIndices(
    const IndexListRecord &record,
    const std::vector<T *> &values,
    const char *what)
{
    if ( record.isNull ) {
        return nullptr;
    }
    java::ArrayList<T *> *list = new java::ArrayList<T *>();
    for ( size_t i = 0; i < record.indices.size(); i++ ) {
        list->add(pointerFromIndex(values, record.indices[i], what));
    }
    return list;
}

void
validateBinaryHeader(java::io::InputStream &input) {
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
         || modelSize != static_cast<int32_t>(sizeof(MgfModel)) ) {
        throw std::runtime_error("Incompatible binary model platform/type sizes");
    }
}

void
populateModelStrings(MgfModel *model, const ModelRecord &record) {
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
cleanupPartialModel(
    std::vector<Vector3D *> &vectors,
    std::vector<Vertex *> &vertices,
    std::vector<Patch *> &patches,
    std::vector<Material *> &materials,
    std::vector<Geometry *> &geometries,
    std::vector<MgfColorContext *> &colorContexts,
    std::vector<MgfReaderContext *> &readerContexts,
    std::vector<MgfTransformArray *> &transformArrays,
    std::vector<MgfTransformContext *> &transformContexts,
    MgfModel *model)
{
    const bool hasGeometry = !geometries.empty();
    bool hasSurfaceGeometry = false;
    for ( size_t i = 0; i < geometries.size(); i++ ) {
        if ( geometries[i] != nullptr && geometries[i]->className == GeometryClassId::SURFACE_MESH ) {
            hasSurfaceGeometry = true;
            break;
        }
    }

    for ( size_t i = 0; i < geometries.size(); i++ ) {
        delete geometries[i];
    }

    if ( !hasGeometry || !hasSurfaceGeometry ) {
        for ( size_t i = 0; i < patches.size(); i++ ) {
            delete patches[i];
        }
        for ( size_t i = 0; i < vertices.size(); i++ ) {
            delete vertices[i];
        }
        for ( size_t i = 0; i < vectors.size(); i++ ) {
            delete vectors[i];
        }
    }

    for ( size_t i = 0; i < materials.size(); i++ ) {
        delete materials[i];
    }
    for ( size_t i = 0; i < colorContexts.size(); i++ ) {
        delete colorContexts[i];
    }
    for ( size_t i = 0; i < readerContexts.size(); i++ ) {
        delete readerContexts[i];
    }
    for ( size_t i = 0; i < transformArrays.size(); i++ ) {
        delete transformArrays[i];
    }
    for ( size_t i = 0; i < transformContexts.size(); i++ ) {
        delete transformContexts[i];
    }

    if ( model != nullptr ) {
        delete[] model->currentMaterialName;
        delete[] model->currentObjectName;
        delete[] model->currentVertexName;
        delete model;
    }
}

} // namespace

MgfModel *
BinaryModelReader::read(const char *fileName) {
    if ( fileName == nullptr || fileName[0] == '\0' ) {
        return nullptr;
    }

    java::io::BufferedInputStream input;
    if ( !input.open(fileName) ) {
        return nullptr;
    }

    std::vector<Vector3D *> vectors;
    std::vector<Vertex *> vertices;
    std::vector<Patch *> patches;
    std::vector<Material *> materials;
    std::vector<Geometry *> geometries;
    std::vector<MgfColorContext *> colorContexts;
    std::vector<MgfReaderContext *> readerContexts;
    std::vector<MgfTransformArray *> transformArrays;
    std::vector<MgfTransformContext *> transformContexts;
    MgfModel *model = nullptr;

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

        vectors.resize(static_cast<size_t>(vectorCount), nullptr);
        vertices.resize(static_cast<size_t>(vertexCount), nullptr);
        patches.resize(static_cast<size_t>(patchCount), nullptr);
        materials.resize(static_cast<size_t>(materialCount), nullptr);
        geometries.resize(static_cast<size_t>(geometryCount), nullptr);
        colorContexts.resize(static_cast<size_t>(colorContextCount), nullptr);
        readerContexts.resize(static_cast<size_t>(readerContextCount), nullptr);
        transformArrays.resize(static_cast<size_t>(transformArrayCount), nullptr);
        transformContexts.resize(static_cast<size_t>(transformContextCount), nullptr);

        expectTag(input, "VEC3");
        for ( int32_t i = 0; i < vectorCount; i++ ) {
            Vector3D *vector = new Vector3D();
            readVector(input, vector);
            vectors[static_cast<size_t>(i)] = vector;
        }

        expectTag(input, "MTLS");
        for ( int32_t i = 0; i < materialCount; i++ ) {
            std::string materialName;
            const bool hasMaterialName = readNullableString(input, &materialName);
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

                    std::vector<unsigned char> textureData;
                    if ( dataBytes > 0 ) {
                        if ( dataBytes > static_cast<int64_t>(std::numeric_limits<size_t>::max()) ) {
                            throw std::runtime_error("Texture data too large for current platform");
                        }
                        textureData.resize(static_cast<size_t>(dataBytes));
                        readBytesChunked(input, &textureData[0], dataBytes);
                    }
                    texture = new Texture(
                        width,
                        height,
                        channels,
                        textureData.empty() ? nullptr : &textureData[0]);
                }

                bsdf = new PhongBidirectionalScatteringDistributionFunction(brdf, btdf, texture);
            }

            const char *materialNameCstr = hasMaterialName ? materialName.c_str() : "";
            materials[static_cast<size_t>(i)] = new Material(materialNameCstr, edf, bsdf, sided);
        }

        expectTag(input, "COLR");
        for ( int32_t i = 0; i < colorContextCount; i++ ) {
            MgfColorContext *colorContext = new MgfColorContext();
            colorContext->clock = readInt32LE(input);
            colorContext->flags = readInt16LE(input);
            for ( int j = 0; j < NUMBER_OF_SPECTRAL_SAMPLES; j++ ) {
                colorContext->straightSamples[j] = readInt16LE(input);
            }
            colorContext->spectralStraightSum = static_cast<long>(readInt64LE(input));
            colorContext->cx = readFloatLE(input);
            colorContext->cy = readFloatLE(input);
            colorContext->eff = readFloatLE(input);
            colorContexts[static_cast<size_t>(i)] = colorContext;
        }

        expectTag(input, "RCTX");
        std::vector<int32_t> readerContextPrevIndex(static_cast<size_t>(readerContextCount), -1);
        for ( int32_t i = 0; i < readerContextCount; i++ ) {
            MgfReaderContext *readerContext = new MgfReaderContext();

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
            readerContextPrevIndex[static_cast<size_t>(i)] = readInt32LE(input);
            readerContext->prev = nullptr;
            readerContexts[static_cast<size_t>(i)] = readerContext;
        }
        for ( int32_t i = 0; i < readerContextCount; i++ ) {
            readerContexts[static_cast<size_t>(i)]->prev = pointerFromIndex(
                readerContexts,
                readerContextPrevIndex[static_cast<size_t>(i)],
                "readerContext.prev");
        }

        expectTag(input, "XFAR");
        for ( int32_t i = 0; i < transformArrayCount; i++ ) {
            MgfTransformArray *transformArray = new MgfTransformArray();
            transformArray->startingPosition.fid = readInt32LE(input);
            transformArray->startingPosition.lineno = readInt32LE(input);
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
            transformArrays[static_cast<size_t>(i)] = transformArray;
        }

        expectTag(input, "XFCT");
        std::vector<int32_t> transformContextArrayIndex(static_cast<size_t>(transformContextCount), -1);
        std::vector<int32_t> transformContextPrevIndex(static_cast<size_t>(transformContextCount), -1);
        for ( int32_t i = 0; i < transformContextCount; i++ ) {
            MgfTransformContext *transformContext = new MgfTransformContext();
            transformContext->xid = static_cast<long>(readInt64LE(input));
            transformContext->xac = readInt16LE(input);
            transformContext->rev = readInt16LE(input);

            for ( int row = 0; row < 4; row++ ) {
                for ( int col = 0; col < 4; col++ ) {
                    transformContext->xf.transformMatrix.m[row][col] = readDoubleLE(input);
                }
            }
            transformContext->xf.scaleFactor = readDoubleLE(input);
            transformContextArrayIndex[static_cast<size_t>(i)] = readInt32LE(input);
            transformContextPrevIndex[static_cast<size_t>(i)] = readInt32LE(input);
            transformContext->transformationArray = nullptr;
            transformContext->prev = nullptr;
            transformContexts[static_cast<size_t>(i)] = transformContext;
        }
        for ( int32_t i = 0; i < transformContextCount; i++ ) {
            MgfTransformContext *transformContext = transformContexts[static_cast<size_t>(i)];
            transformContext->transformationArray = pointerFromIndex(
                transformArrays,
                transformContextArrayIndex[static_cast<size_t>(i)],
                "transformContext.transformationArray");
            transformContext->prev = pointerFromIndex(
                transformContexts,
                transformContextPrevIndex[static_cast<size_t>(i)],
                "transformContext.prev");
        }

        expectTag(input, "VRTX");
        std::vector<VertexRecord> vertexRecords(static_cast<size_t>(vertexCount));
        for ( int32_t i = 0; i < vertexCount; i++ ) {
            VertexRecord &record = vertexRecords[static_cast<size_t>(i)];
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
            vertices[static_cast<size_t>(i)] = vertex;
        }

        for ( int32_t i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices[static_cast<size_t>(i)];
            const VertexRecord &record = vertexRecords[static_cast<size_t>(i)];
            vertex->back = pointerFromIndex(vertices, record.backIndex, "vertex.back");
        }

        expectTag(input, "PTCH");
        std::vector<PatchRecord> patchRecords(static_cast<size_t>(patchCount));
        for ( int32_t i = 0; i < patchCount; i++ ) {
            PatchRecord &record = patchRecords[static_cast<size_t>(i)];
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

            patches[static_cast<size_t>(i)] = patch;
        }

        for ( int32_t i = 0; i < patchCount; i++ ) {
            Patch *patch = patches[static_cast<size_t>(i)];
            const PatchRecord &record = patchRecords[static_cast<size_t>(i)];
            patch->twin = pointerFromIndex(patches, record.twinIndex, "patch.twin");
        }

        for ( int32_t i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices[static_cast<size_t>(i)];
            delete vertex->patches;
            vertex->patches = arrayListFromIndices(
                vertexRecords[static_cast<size_t>(i)].patchIndices,
                patches,
                "vertex.patches");
        }

        expectTag(input, "GEOM");
        std::vector<GeometryRecord> geometryRecords(static_cast<size_t>(geometryCount));
        for ( int32_t i = 0; i < geometryCount; i++ ) {
            GeometryRecord &record = geometryRecords[static_cast<size_t>(i)];
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
            record.objectName.clear();
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
            const GeometryRecord &record = geometryRecords[static_cast<size_t>(i)];
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
            geometries[static_cast<size_t>(i)] = geometry;
        }

        for ( int32_t i = 0; i < geometryCount; i++ ) {
            const GeometryRecord &record = geometryRecords[static_cast<size_t>(i)];
            if ( record.classId == static_cast<int32_t>(GeometryClassId::COMPOUND) ) {
                Compound *compound = static_cast<Compound *>(geometries[static_cast<size_t>(i)]);
                delete compound->children;
                compound->children = arrayListFromIndices(record.children, geometries, "compound.children");
            }
        }

        expectTag(input, "MODL");
        ModelRecord modelRecord;
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

        model = new MgfModel();
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
        for ( size_t i = 0; i < patches.size(); i++ ) {
            if ( patches[i] != nullptr ) {
                maxPatchId = std::max(maxPatchId, static_cast<int>(patches[i]->id));
            }
        }
        Patch::setNextId(maxPatchId + 1);

        int maxGeometryId = -1;
        for ( size_t i = 0; i < geometries.size(); i++ ) {
            if ( geometries[i] != nullptr ) {
                maxGeometryId = std::max(maxGeometryId, geometries[i]->id);
            }
        }
        Geometry::nextGeometryId = maxGeometryId + 1;

        input.dispose();
        return model;
    } catch ( const std::exception &e ) {
        logError("BinaryModelReader::read", "%s", e.what());
        input.dispose();
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
