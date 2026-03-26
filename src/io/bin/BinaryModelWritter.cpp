#include "io/bin/BinaryModelWritter.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/error.h"
#include "java/util/ArrayList.txx"
#include "common/ColorRgb.h"
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
#include "skin/MeshSurface.h"
#include "skin/Patch.h"
#include "skin/PatchSet.h"
#include "skin/Vertex.h"

namespace {

static const unsigned char BINARY_MODEL_MAGIC[16] = {
    'R', 'P', 'K', 'M', 'G', 'F', 'B', 'I',
    'N', 'W', 'R', 'T', '1', 0, 0, 0
};
static const int32_t BINARY_MODEL_VERSION = 1;

void
writeBytesChunked(FILE *output, const unsigned char *data, int64_t length) {
    if ( length < 0 ) {
        throw std::runtime_error("Negative block length");
    }
    int64_t offset = 0;
    const int64_t maxChunk = static_cast<int64_t>(std::numeric_limits<int>::max());
    while ( offset < length ) {
        const int chunk = static_cast<int>(std::min(maxChunk, length - offset));
        vsdk::PersistenceElement::writeBytes(output, data + offset, chunk);
        offset += static_cast<int64_t>(chunk);
    }
}

void
writeTag(FILE *output, const char tag[4]) {
    vsdk::PersistenceElement::writeBytes(
        output,
        reinterpret_cast<const unsigned char *>(tag),
        4);
}

void
writeByte(FILE *output, unsigned char value) {
    vsdk::PersistenceElement::writeBytes(output, &value, 1);
}

void
writeBool(FILE *output, bool value) {
    writeByte(output, static_cast<unsigned char>(value ? 1 : 0));
}

void
writeInt16LE(FILE *output, int16_t value) {
    vsdk::PersistenceElement::writeSignedShortLE(output, value);
}

void
writeInt32LE(FILE *output, int32_t value) {
    vsdk::PersistenceElement::writeLongLE(output, static_cast<long>(value));
}

void
writeInt64LE(FILE *output, int64_t value) {
    uint64_t bits = static_cast<uint64_t>(value);
    unsigned char bytes[8];
    for ( int i = 0; i < 8; i++ ) {
        bytes[i] = static_cast<unsigned char>((bits >> (8 * i)) & 0xFFULL);
    }
    vsdk::PersistenceElement::writeBytes(output, bytes, 8);
}

void
writeDoubleLE(FILE *output, double value) {
    uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(double));
    unsigned char bytes[8];
    for ( int i = 0; i < 8; i++ ) {
        bytes[i] = static_cast<unsigned char>((bits >> (8 * i)) & 0xFFULL);
    }
    vsdk::PersistenceElement::writeBytes(output, bytes, 8);
}

void
writeFloatLE(FILE *output, float value) {
    vsdk::PersistenceElement::writeFloatLE(output, value);
}

int32_t
checkedLongToInt32(long value, const char *what) {
    if ( value > static_cast<long>(std::numeric_limits<int32_t>::max())
         || value < static_cast<long>(std::numeric_limits<int32_t>::min()) ) {
        throw std::runtime_error(std::string("Overflow converting to int32 for ") + what);
    }
    return static_cast<int32_t>(value);
}

void
writeString(FILE *output, const char *text) {
    if ( text == nullptr ) {
        writeInt32LE(output, -1);
        return;
    }
    const long size = static_cast<long>(std::strlen(text));
    writeInt32LE(output, checkedLongToInt32(size, "string length"));
    if ( size > 0 ) {
        vsdk::PersistenceElement::writeBytes(
            output,
            reinterpret_cast<const unsigned char *>(text),
            static_cast<int>(size));
    }
}

void
writeColor(FILE *output, const ColorRgb &color) {
    writeFloatLE(output, color.r);
    writeFloatLE(output, color.g);
    writeFloatLE(output, color.b);
}

void
writeVector(FILE *output, const Vector3D &vector) {
    writeFloatLE(output, vector.x);
    writeFloatLE(output, vector.y);
    writeFloatLE(output, vector.z);
}

void
writeBoundingBox(FILE *output, const BoundingBox &boundingBox) {
    for ( int i = 0; i < 6; i++ ) {
        writeFloatLE(output, boundingBox.valueAt(i));
    }
}

template <typename T>
int32_t
indexOfPointer(const T *ptr, const std::unordered_map<const T *, int> &indices, const char *what) {
    if ( ptr == nullptr ) {
        return -1;
    }
    typename std::unordered_map<const T *, int>::const_iterator it = indices.find(ptr);
    if ( it == indices.end() ) {
        throw std::runtime_error(std::string("Missing pointer index for ") + what);
    }
    return static_cast<int32_t>(it->second);
}

template <typename T>
void
writeIndexList(
    FILE *output,
    const java::ArrayList<T *> *list,
    const std::unordered_map<const T *, int> &indices,
    const char *what)
{
    if ( list == nullptr ) {
        writeInt32LE(output, -1);
        return;
    }

    const int32_t size = checkedLongToInt32(list->size(), what);
    writeInt32LE(output, size);
    for ( int32_t i = 0; i < size; i++ ) {
        const T *element = list->get(i);
        writeInt32LE(output, indexOfPointer(element, indices, what));
    }
}

struct SerializationContext {
    std::unordered_map<const Vector3D *, int> vectorIndices;
    std::vector<const Vector3D *> vectors;

    std::unordered_map<const Vertex *, int> vertexIndices;
    std::vector<const Vertex *> vertices;

    std::unordered_map<const Patch *, int> patchIndices;
    std::vector<const Patch *> patches;

    std::unordered_map<const Material *, int> materialIndices;
    std::vector<const Material *> materials;

    std::unordered_map<const Geometry *, int> geometryIndices;
    std::vector<const Geometry *> geometries;

    std::unordered_map<const MgfColorContext *, int> colorContextIndices;
    std::vector<const MgfColorContext *> colorContexts;

    std::unordered_map<const MgfReaderContext *, int> readerContextIndices;
    std::vector<const MgfReaderContext *> readerContexts;

    std::unordered_map<const MgfTransformArray *, int> transformArrayIndices;
    std::vector<const MgfTransformArray *> transformArrays;

    std::unordered_map<const MgfTransformContext *, int> transformContextIndices;
    std::vector<const MgfTransformContext *> transformContexts;

    int ensureVector(const Vector3D *value);
    int ensureMaterial(const Material *value);
    int ensureVertex(const Vertex *value);
    int ensurePatch(const Patch *value);
    int ensureGeometry(const Geometry *value);
    int ensureColorContext(const MgfColorContext *value);
    int ensureReaderContext(const MgfReaderContext *value);
    int ensureTransformArray(const MgfTransformArray *value);
    int ensureTransformContext(const MgfTransformContext *value);

    void collectVectorList(const java::ArrayList<Vector3D *> *list);
    void collectVertexList(const java::ArrayList<Vertex *> *list);
    void collectPatchList(const java::ArrayList<Patch *> *list);
    void collectMaterialList(const java::ArrayList<Material *> *list);
    void collectGeometryList(const java::ArrayList<Geometry *> *list);
    void collectModel(const MgfModel *model);
};

int
SerializationContext::ensureVector(const Vector3D *value) {
    if ( value == nullptr ) {
        return -1;
    }
    std::unordered_map<const Vector3D *, int>::const_iterator it = vectorIndices.find(value);
    if ( it != vectorIndices.end() ) {
        return it->second;
    }
    const int index = static_cast<int>(vectors.size());
    vectorIndices[value] = index;
    vectors.push_back(value);
    return index;
}

int
SerializationContext::ensureMaterial(const Material *value) {
    if ( value == nullptr ) {
        return -1;
    }
    std::unordered_map<const Material *, int>::const_iterator it = materialIndices.find(value);
    if ( it != materialIndices.end() ) {
        return it->second;
    }
    const int index = static_cast<int>(materials.size());
    materialIndices[value] = index;
    materials.push_back(value);
    return index;
}

int
SerializationContext::ensureVertex(const Vertex *value) {
    if ( value == nullptr ) {
        return -1;
    }
    std::unordered_map<const Vertex *, int>::const_iterator it = vertexIndices.find(value);
    if ( it != vertexIndices.end() ) {
        return it->second;
    }

    if ( value->radianceData != nullptr ) {
        throw std::runtime_error("Vertex radianceData is not supported by BinaryModelWritter");
    }

    const int index = static_cast<int>(vertices.size());
    vertexIndices[value] = index;
    vertices.push_back(value);

    ensureVector(value->point);
    ensureVector(value->normal);
    ensureVector(value->textureCoordinates);
    ensureVertex(value->back);

    if ( value->patches != nullptr ) {
        for ( int i = 0; i < value->patches->size(); i++ ) {
            ensurePatch(value->patches->get(i));
        }
    }

    return index;
}

int
SerializationContext::ensurePatch(const Patch *value) {
    if ( value == nullptr ) {
        return -1;
    }
    std::unordered_map<const Patch *, int>::const_iterator it = patchIndices.find(value);
    if ( it != patchIndices.end() ) {
        return it->second;
    }

    if ( value->radianceData != nullptr ) {
        throw std::runtime_error("Patch radianceData is not supported by BinaryModelWritter");
    }

    const int index = static_cast<int>(patches.size());
    patchIndices[value] = index;
    patches.push_back(value);

    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        ensureVertex(value->vertex[i]);
    }
    ensurePatch(value->twin);
    ensureMaterial(value->material);

    return index;
}

int
SerializationContext::ensureGeometry(const Geometry *value) {
    if ( value == nullptr ) {
        return -1;
    }
    std::unordered_map<const Geometry *, int>::const_iterator it = geometryIndices.find(value);
    if ( it != geometryIndices.end() ) {
        return it->second;
    }

    if ( value->radianceData != nullptr ) {
        throw std::runtime_error("Geometry radianceData is not supported by BinaryModelWritter");
    }

    const int index = static_cast<int>(geometries.size());
    geometryIndices[value] = index;
    geometries.push_back(value);

    if ( value->className == GeometryClassId::SURFACE_MESH ) {
        const MeshSurface *surface = static_cast<const MeshSurface *>(value);
        ensureMaterial(surface->material);
        collectVectorList(surface->positions);
        collectVectorList(surface->normals);
        collectVertexList(surface->vertices);
        collectPatchList(surface->faces);
    } else if ( value->className == GeometryClassId::COMPOUND ) {
        const Compound *compound = static_cast<const Compound *>(value);
        collectGeometryList(compound->children);
    } else if ( value->className == GeometryClassId::PATCH_SET ) {
        const PatchSet *patchSet = static_cast<const PatchSet *>(value);
        collectPatchList(patchSet->getPatchList());
    } else {
        throw std::runtime_error("Unsupported geometry class for BinaryModelWritter");
    }

    return index;
}

int
SerializationContext::ensureColorContext(const MgfColorContext *value) {
    if ( value == nullptr ) {
        return -1;
    }
    std::unordered_map<const MgfColorContext *, int>::const_iterator it = colorContextIndices.find(value);
    if ( it != colorContextIndices.end() ) {
        return it->second;
    }
    const int index = static_cast<int>(colorContexts.size());
    colorContextIndices[value] = index;
    colorContexts.push_back(value);
    return index;
}

int
SerializationContext::ensureReaderContext(const MgfReaderContext *value) {
    if ( value == nullptr ) {
        return -1;
    }
    std::unordered_map<const MgfReaderContext *, int>::const_iterator it = readerContextIndices.find(value);
    if ( it != readerContextIndices.end() ) {
        return it->second;
    }
    const int index = static_cast<int>(readerContexts.size());
    readerContextIndices[value] = index;
    readerContexts.push_back(value);

    ensureReaderContext(value->prev);
    return index;
}

int
SerializationContext::ensureTransformArray(const MgfTransformArray *value) {
    if ( value == nullptr ) {
        return -1;
    }
    std::unordered_map<const MgfTransformArray *, int>::const_iterator it = transformArrayIndices.find(value);
    if ( it != transformArrayIndices.end() ) {
        return it->second;
    }
    const int index = static_cast<int>(transformArrays.size());
    transformArrayIndices[value] = index;
    transformArrays.push_back(value);
    return index;
}

int
SerializationContext::ensureTransformContext(const MgfTransformContext *value) {
    if ( value == nullptr ) {
        return -1;
    }
    std::unordered_map<const MgfTransformContext *, int>::const_iterator it = transformContextIndices.find(value);
    if ( it != transformContextIndices.end() ) {
        return it->second;
    }
    const int index = static_cast<int>(transformContexts.size());
    transformContextIndices[value] = index;
    transformContexts.push_back(value);

    ensureTransformArray(value->transformationArray);
    ensureTransformContext(value->prev);
    return index;
}

void
SerializationContext::collectVectorList(const java::ArrayList<Vector3D *> *list) {
    if ( list == nullptr ) {
        return;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        ensureVector(list->get(i));
    }
}

void
SerializationContext::collectVertexList(const java::ArrayList<Vertex *> *list) {
    if ( list == nullptr ) {
        return;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        ensureVertex(list->get(i));
    }
}

void
SerializationContext::collectPatchList(const java::ArrayList<Patch *> *list) {
    if ( list == nullptr ) {
        return;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        ensurePatch(list->get(i));
    }
}

void
SerializationContext::collectMaterialList(const java::ArrayList<Material *> *list) {
    if ( list == nullptr ) {
        return;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        ensureMaterial(list->get(i));
    }
}

void
SerializationContext::collectGeometryList(const java::ArrayList<Geometry *> *list) {
    if ( list == nullptr ) {
        return;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        ensureGeometry(list->get(i));
    }
}

void
SerializationContext::collectModel(const MgfModel *model) {
    if ( model == nullptr ) {
        return;
    }

    ensureColorContext(model->currentColor);
    collectPatchList(model->currentFaceList);
    collectGeometryList(model->currentGeometryList);
    collectMaterialList(model->materials);
    collectVectorList(model->currentNormalList);
    collectVectorList(model->currentPointList);
    collectVertexList(model->currentVertexList);
    collectGeometryList(model->geometries);
    ensureReaderContext(model->readerContext);
    ensureTransformContext(model->transformContext);
}

void
writeMaterialRecord(FILE *output, const Material *material) {
    writeString(output, material->getName());
    writeBool(output, material->isSided());

    const PhongEmittanceDistributionFunction *edf = material->getEdf();
    writeBool(output, edf != nullptr);
    if ( edf != nullptr ) {
        writeColor(output, edf->getKd());
        writeColor(output, edf->getKs());
        writeFloatLE(output, edf->getNs());
    }

    const PhongBidirectionalScatteringDistributionFunction *bsdf = material->getBsdf();
    writeBool(output, bsdf != nullptr);
    if ( bsdf == nullptr ) {
        return;
    }

    const PhongBidirectionalReflectanceDistributionFunction *brdf = bsdf->getBrdf();
    writeBool(output, brdf != nullptr);
    if ( brdf != nullptr ) {
        writeColor(output, brdf->getKd());
        writeColor(output, brdf->getKs());
        writeFloatLE(output, brdf->getNs());
    }

    const PhongBidirectionalTransmittanceDistributionFunction *btdf = bsdf->getBtdf();
    writeBool(output, btdf != nullptr);
    if ( btdf != nullptr ) {
        writeColor(output, btdf->getKd());
        writeColor(output, btdf->getKs());
        writeFloatLE(output, btdf->getNs());
        writeFloatLE(output, btdf->getRefractionIndex().getNr());
        writeFloatLE(output, btdf->getRefractionIndex().getNi());
    }

    const Texture *texture = bsdf->getTexture();
    writeBool(output, texture != nullptr);
    if ( texture != nullptr ) {
        const int width = texture->getWidth();
        const int height = texture->getHeight();
        const int channels = texture->getChannels();
        if ( width < 0 || height < 0 || channels < 0 ) {
            throw std::runtime_error("Invalid texture dimensions");
        }

        writeInt32LE(output, width);
        writeInt32LE(output, height);
        writeInt32LE(output, channels);

        const int64_t dataBytes = static_cast<int64_t>(width)
                                  * static_cast<int64_t>(height)
                                  * static_cast<int64_t>(channels);
        writeInt64LE(output, dataBytes);

        if ( dataBytes > 0 ) {
            const unsigned char *data = texture->getData();
            if ( data == nullptr ) {
                throw std::runtime_error("Texture data is null with non-zero size");
            }
            writeBytesChunked(output, data, dataBytes);
        }
    }
}

void
writeColorContextRecord(FILE *output, const MgfColorContext *colorContext) {
    writeInt32LE(output, colorContext->clock);
    writeInt16LE(output, colorContext->flags);
    for ( int i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
        writeInt16LE(output, colorContext->straightSamples[i]);
    }
    writeInt64LE(output, static_cast<int64_t>(colorContext->spectralStraightSum));
    writeFloatLE(output, colorContext->cx);
    writeFloatLE(output, colorContext->cy);
    writeFloatLE(output, colorContext->eff);
}

void
writeReaderContextRecord(
    FILE *output,
    const MgfReaderContext *readerContext,
    const SerializationContext &context)
{
    vsdk::PersistenceElement::writeBytes(
        output,
        reinterpret_cast<const unsigned char *>(readerContext->fileName),
        96);
    writeBool(output, readerContext->inputStream != nullptr);
    writeInt32LE(output, readerContext->fileContextId);
    vsdk::PersistenceElement::writeBytes(
        output,
        reinterpret_cast<const unsigned char *>(readerContext->inputLine),
        MGF_MAXIMUM_INPUT_LINE_LENGTH);
    writeInt32LE(output, readerContext->lineNumber);
    writeByte(output, static_cast<unsigned char>(readerContext->isPipe));
    writeInt32LE(
        output,
        indexOfPointer(readerContext->prev, context.readerContextIndices, "readerContext.prev"));
}

void
writeTransformArrayRecord(FILE *output, const MgfTransformArray *transformArray) {
    writeInt32LE(output, transformArray->startingPosition.fid);
    writeInt32LE(output, transformArray->startingPosition.lineno);
    writeInt64LE(output, static_cast<int64_t>(transformArray->startingPosition.offset));
    writeInt32LE(output, transformArray->numberOfDimensions);
    for ( int i = 0; i < TRANSFORM_MAXIMUM_DIMENSIONS; i++ ) {
        writeInt16LE(output, transformArray->transformArguments[i].i);
        writeInt16LE(output, transformArray->transformArguments[i].n);
        vsdk::PersistenceElement::writeBytes(
            output,
            reinterpret_cast<const unsigned char *>(transformArray->transformArguments[i].arg),
            8);
    }
}

void
writeTransformContextRecord(
    FILE *output,
    const MgfTransformContext *transformContext,
    const SerializationContext &context)
{
    writeInt64LE(output, static_cast<int64_t>(transformContext->xid));
    writeInt16LE(output, transformContext->xac);
    writeInt16LE(output, transformContext->rev);

    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            writeDoubleLE(output, transformContext->xf.transformMatrix.m[i][j]);
        }
    }
    writeDoubleLE(output, transformContext->xf.scaleFactor);

    writeInt32LE(
        output,
        indexOfPointer(
            transformContext->transformationArray,
            context.transformArrayIndices,
            "transformContext.transformationArray"));
    writeInt32LE(
        output,
        indexOfPointer(
            transformContext->prev,
            context.transformContextIndices,
            "transformContext.prev"));
}

void
writeVertexRecord(FILE *output, const Vertex *vertex, const SerializationContext &context) {
    writeInt32LE(output, vertex->id);
    writeInt32LE(output, indexOfPointer(vertex->point, context.vectorIndices, "vertex.point"));
    writeInt32LE(output, indexOfPointer(vertex->normal, context.vectorIndices, "vertex.normal"));
    writeInt32LE(output, indexOfPointer(vertex->textureCoordinates, context.vectorIndices, "vertex.textureCoordinates"));
    writeColor(output, vertex->color);
    writeInt32LE(output, indexOfPointer(vertex->back, context.vertexIndices, "vertex.back"));
    writeInt32LE(output, vertex->tmp);
    writeBool(output, vertex->radianceData != nullptr);
    writeIndexList(output, vertex->patches, context.patchIndices, "vertex.patches");
}

void
writePatchRecord(FILE *output, const Patch *patch, const SerializationContext &context) {
    writeInt32LE(output, static_cast<int32_t>(patch->id));
    writeInt32LE(output, indexOfPointer(patch->twin, context.patchIndices, "patch.twin"));
    writeInt32LE(output, static_cast<int32_t>(patch->numberOfVertices));
    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        writeInt32LE(output, indexOfPointer(patch->vertex[i], context.vertexIndices, "patch.vertex"));
    }

    writeBool(output, patch->boundingBox != nullptr);
    if ( patch->boundingBox != nullptr ) {
        writeBoundingBox(output, *patch->boundingBox);
    }

    writeVector(output, patch->normal);
    writeFloatLE(output, patch->planeConstant);
    writeFloatLE(output, patch->tolerance);
    writeFloatLE(output, patch->area);
    writeVector(output, patch->midPoint);

    writeBool(output, patch->jacobian != nullptr);
    if ( patch->jacobian != nullptr ) {
        writeFloatLE(output, patch->jacobian->A);
        writeFloatLE(output, patch->jacobian->B);
        writeFloatLE(output, patch->jacobian->C);
    }

    writeFloatLE(output, patch->directPotential);
    writeInt32LE(output, static_cast<int32_t>(patch->index));
    writeBool(output, patch->omit != 0);
    writeByte(output, patch->getFlags());
    writeColor(output, patch->color);
    writeInt32LE(output, indexOfPointer(patch->material, context.materialIndices, "patch.material"));
    writeBool(output, patch->radianceData != nullptr);
}

void
writeGeometryRecord(FILE *output, const Geometry *geometry, const SerializationContext &context) {
    writeInt32LE(output, static_cast<int32_t>(geometry->className));
    writeInt32LE(output, geometry->id);
    writeInt32LE(output, geometry->itemCount);
    writeBool(output, geometry->bounded != 0);
    writeBool(output, geometry->shaftCullGeometry != 0);
    writeBool(output, geometry->omit != 0);
    writeBool(output, geometry->isDuplicate);
    writeBoundingBox(output, geometry->boundingBox);
    writeBool(output, geometry->rayIntersectionBox != nullptr);
    writeBool(output, geometry->radianceData != nullptr);

    if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
        const MeshSurface *surface = static_cast<const MeshSurface *>(geometry);
        writeString(output, surface->objectName);
        writeInt32LE(output, surface->meshId);
        writeInt32LE(output, indexOfPointer(surface->material, context.materialIndices, "surface.material"));
        writeIndexList(output, surface->positions, context.vectorIndices, "surface.positions");
        writeIndexList(output, surface->normals, context.vectorIndices, "surface.normals");
        writeIndexList(output, surface->vertices, context.vertexIndices, "surface.vertices");
        writeIndexList(output, surface->faces, context.patchIndices, "surface.faces");
    } else if ( geometry->className == GeometryClassId::COMPOUND ) {
        const Compound *compound = static_cast<const Compound *>(geometry);
        writeIndexList(output, compound->children, context.geometryIndices, "compound.children");
    } else if ( geometry->className == GeometryClassId::PATCH_SET ) {
        const PatchSet *patchSet = static_cast<const PatchSet *>(geometry);
        writeIndexList(output, patchSet->getPatchList(), context.patchIndices, "patchSet.patchList");
    } else {
        throw std::runtime_error("Unsupported geometry class while writing");
    }
}

void
writeModelRecord(FILE *output, const MgfModel *model, const SerializationContext &context) {
    writeInt32LE(output, indexOfPointer(model->currentColor, context.colorContextIndices, "model.currentColor"));
    writeString(output, model->currentMaterialName);
    writeString(output, model->currentObjectName);
    writeString(output, model->currentVertexName);
    writeInt32LE(output, model->geometryStackHeadIndex);
    writeBool(output, model->inComplex);
    writeBool(output, model->inSurface);
    writeBool(output, model->monochrome);
    writeInt32LE(output, indexOfPointer(model->readerContext, context.readerContextIndices, "model.readerContext"));
    writeInt32LE(output, indexOfPointer(model->transformContext, context.transformContextIndices, "model.transformContext"));

    writeIndexList(output, model->currentFaceList, context.patchIndices, "model.currentFaceList");
    writeIndexList(output, model->currentGeometryList, context.geometryIndices, "model.currentGeometryList");
    writeIndexList(output, model->currentNormalList, context.vectorIndices, "model.currentNormalList");
    writeIndexList(output, model->currentPointList, context.vectorIndices, "model.currentPointList");
    writeIndexList(output, model->currentVertexList, context.vertexIndices, "model.currentVertexList");
    writeIndexList(output, model->geometries, context.geometryIndices, "model.geometries");
    writeIndexList(output, model->materials, context.materialIndices, "model.materials");
}

} // namespace

bool
BinaryModelWritter::write(const MgfModel *model, const char *fileName) {
    if ( model == nullptr || fileName == nullptr || fileName[0] == '\0' ) {
        return false;
    }

    FILE *output = fopen(fileName, "wb");
    if ( output == nullptr ) {
        return false;
    }

    bool ok = false;
    try {
        SerializationContext context;
        context.collectModel(model);

        vsdk::PersistenceElement::writeBytes(output, BINARY_MODEL_MAGIC, 16);
        writeInt32LE(output, BINARY_MODEL_VERSION);
        writeInt32LE(output, static_cast<int32_t>(sizeof(void *)));
        writeInt32LE(output, static_cast<int32_t>(sizeof(long)));
        writeInt32LE(output, static_cast<int32_t>(sizeof(MgfModel)));

        writeInt32LE(output, static_cast<int32_t>(context.vectors.size()));
        writeInt32LE(output, static_cast<int32_t>(context.vertices.size()));
        writeInt32LE(output, static_cast<int32_t>(context.patches.size()));
        writeInt32LE(output, static_cast<int32_t>(context.materials.size()));
        writeInt32LE(output, static_cast<int32_t>(context.geometries.size()));
        writeInt32LE(output, static_cast<int32_t>(context.colorContexts.size()));
        writeInt32LE(output, static_cast<int32_t>(context.readerContexts.size()));
        writeInt32LE(output, static_cast<int32_t>(context.transformArrays.size()));
        writeInt32LE(output, static_cast<int32_t>(context.transformContexts.size()));

        writeTag(output, "VEC3");
        for ( size_t i = 0; i < context.vectors.size(); i++ ) {
            writeVector(output, *context.vectors[i]);
        }

        writeTag(output, "MTLS");
        for ( size_t i = 0; i < context.materials.size(); i++ ) {
            writeMaterialRecord(output, context.materials[i]);
        }

        writeTag(output, "COLR");
        for ( size_t i = 0; i < context.colorContexts.size(); i++ ) {
            writeColorContextRecord(output, context.colorContexts[i]);
        }

        writeTag(output, "RCTX");
        for ( size_t i = 0; i < context.readerContexts.size(); i++ ) {
            writeReaderContextRecord(output, context.readerContexts[i], context);
        }

        writeTag(output, "XFAR");
        for ( size_t i = 0; i < context.transformArrays.size(); i++ ) {
            writeTransformArrayRecord(output, context.transformArrays[i]);
        }

        writeTag(output, "XFCT");
        for ( size_t i = 0; i < context.transformContexts.size(); i++ ) {
            writeTransformContextRecord(output, context.transformContexts[i], context);
        }

        writeTag(output, "VRTX");
        for ( size_t i = 0; i < context.vertices.size(); i++ ) {
            writeVertexRecord(output, context.vertices[i], context);
        }

        writeTag(output, "PTCH");
        for ( size_t i = 0; i < context.patches.size(); i++ ) {
            writePatchRecord(output, context.patches[i], context);
        }

        writeTag(output, "GEOM");
        for ( size_t i = 0; i < context.geometries.size(); i++ ) {
            writeGeometryRecord(output, context.geometries[i], context);
        }

        writeTag(output, "MODL");
        writeModelRecord(output, model, context);

        ok = true;
    } catch ( const std::exception &e ) {
        logError("BinaryModelWritter::write", "%s", e.what());
        ok = false;
    }

    fclose(output);
    return ok;
}
