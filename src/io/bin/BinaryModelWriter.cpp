#include "io/bin/BinaryModelWriter.h"

#include "java/io/FileOutputStream.h"
#include "java/util/ArrayList.txx"
#include "java/util/HashMap.txx"
#include "common/error.h"
#include "common/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/PersistenceElement.h"
#include "io/context/ColorContext.h"
#include "io/PersistedSceneModel.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformArray.h"
#include "io/context/TransformContext.h"
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

const unsigned char BinaryModelWriter::BINARY_MODEL_MAGIC[16] = {
    'R', 'P', 'K', '_', 'M', 'G', 'F', '_',
    'B', 'I', 'N', '_', '1', 0, 0, 0
};
const int32_t BinaryModelWriter::BINARY_MODEL_VERSION = 1;

void
BinaryModelWriter::writeBytesChunked(java::io::OutputStream &output, const unsigned char *data, int64_t length) {
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
BinaryModelWriter::writeTag(java::io::OutputStream &output, const char tag[4]) {
    vsdk::PersistenceElement::writeBytes(
        output,
        reinterpret_cast<const unsigned char *>(tag),
        4);
}

int32_t
BinaryModelWriter::checkedLongToInt32(long value, const char *what) {
    if ( value > static_cast<long>(std::numeric_limits<int32_t>::max())
         || value < static_cast<long>(std::numeric_limits<int32_t>::min()) ) {
        throw std::runtime_error(std::string("Overflow converting to int32 for ") + what);
    }
    return static_cast<int32_t>(value);
}

void
BinaryModelWriter::writeString(java::io::OutputStream &output, const char *text) {
    if ( text == nullptr ) {
        vsdk::PersistenceElement::writeInt32LE(output, -1);
        return;
    }
    const long size = static_cast<long>(std::strlen(text));
    vsdk::PersistenceElement::writeInt32LE(output, checkedLongToInt32(size, "string length"));
    if ( size > 0 ) {
        vsdk::PersistenceElement::writeBytes(
            output,
            reinterpret_cast<const unsigned char *>(text),
            static_cast<int>(size));
    }
}

void
BinaryModelWriter::writeColor(java::io::OutputStream &output, const ColorRgb &color) {
    vsdk::PersistenceElement::writeFloatLE(output, color.r);
    vsdk::PersistenceElement::writeFloatLE(output, color.g);
    vsdk::PersistenceElement::writeFloatLE(output, color.b);
}

void
BinaryModelWriter::writeVector(java::io::OutputStream &output, const Vector3D &vector) {
    vsdk::PersistenceElement::writeFloatLE(output, vector.x);
    vsdk::PersistenceElement::writeFloatLE(output, vector.y);
    vsdk::PersistenceElement::writeFloatLE(output, vector.z);
}

void
BinaryModelWriter::writeBoundingBox(java::io::OutputStream &output, const BoundingBox &boundingBox) {
    for ( int i = 0; i < 6; i++ ) {
        vsdk::PersistenceElement::writeFloatLE(output, boundingBox.valueAt(i));
    }
}

template <typename T>
int32_t
BinaryModelWriter::indexOfPointer(const T *ptr, const java::HashMap<const T *, int> &indices, const char *what) {
    if ( ptr == nullptr ) {
        return -1;
    }
    int index = 0;
    if ( !indices.tryGet(ptr, &index) ) {
        throw std::runtime_error(std::string("Missing pointer index for ") + what);
    }
    return static_cast<int32_t>(index);
}

template <typename T>
void
BinaryModelWriter::writeIndexList(
    java::io::OutputStream &output,
    const java::ArrayList<T *> *list,
    const java::HashMap<const T *, int> &indices,
    const char *what)
{
    if ( list == nullptr ) {
        vsdk::PersistenceElement::writeInt32LE(output, -1);
        return;
    }

    const int32_t size = checkedLongToInt32(list->size(), what);
    vsdk::PersistenceElement::writeInt32LE(output, size);
    for ( int32_t i = 0; i < size; i++ ) {
        const T *element = list->get(i);
        vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(element, indices, what));
    }
}

class BinaryModelWriter::SerializationContext {
  public:
    java::HashMap<const Vector3D *, int> vectorIndices;
    java::ArrayList<const Vector3D *> vectors;

    java::HashMap<const Vertex *, int> vertexIndices;
    java::ArrayList<const Vertex *> vertices;

    java::HashMap<const Patch *, int> patchIndices;
    java::ArrayList<const Patch *> patches;

    java::HashMap<const Material *, int> materialIndices;
    java::ArrayList<const Material *> materials;

    java::HashMap<const Geometry *, int> geometryIndices;
    java::ArrayList<const Geometry *> geometries;

    java::HashMap<const ColorContext *, int> colorContextIndices;
    java::ArrayList<const ColorContext *> colorContexts;

    java::HashMap<const ReaderContext *, int> readerContextIndices;
    java::ArrayList<const ReaderContext *> readerContexts;

    java::HashMap<const TransformArray *, int> transformArrayIndices;
    java::ArrayList<const TransformArray *> transformArrays;

    java::HashMap<const TransformContext *, int> transformContextIndices;
    java::ArrayList<const TransformContext *> transformContexts;

    int ensureVector(const Vector3D *value);
    int ensureMaterial(const Material *value);
    int ensureVertex(const Vertex *value);
    int ensurePatch(const Patch *value);
    int ensureGeometry(const Geometry *value);
    int ensureColorContext(const ColorContext *value);
    int ensureReaderContext(const ReaderContext *value);
    int ensureTransformArray(const TransformArray *value);
    int ensureTransformContext(const TransformContext *value);

    void collectVectorList(const java::ArrayList<Vector3D *> *list);
    void collectVertexList(const java::ArrayList<Vertex *> *list);
    void collectPatchList(const java::ArrayList<Patch *> *list);
    void collectMaterialList(const java::ArrayList<Material *> *list);
    void collectGeometryList(const java::ArrayList<Geometry *> *list);
    void collectModel(const PersistedSceneModel *model);
};

int
BinaryModelWriter::SerializationContext::ensureVector(const Vector3D *value) {
    if ( value == nullptr ) {
        return -1;
    }
    int existingIndex = -1;
    if ( vectorIndices.tryGet(value, &existingIndex) ) {
        return existingIndex;
    }
    const int index = static_cast<int>(vectors.size());
    if ( !vectors.add(value) ) {
        throw std::runtime_error("Failed to append vector");
    }
    if ( !vectorIndices.put(value, index) ) {
        vectors.remove(vectors.size() - 1);
        throw std::runtime_error("Failed to index vector");
    }
    return index;
}

int
BinaryModelWriter::SerializationContext::ensureMaterial(const Material *value) {
    if ( value == nullptr ) {
        return -1;
    }
    int existingIndex = -1;
    if ( materialIndices.tryGet(value, &existingIndex) ) {
        return existingIndex;
    }
    const int index = static_cast<int>(materials.size());
    if ( !materials.add(value) ) {
        throw std::runtime_error("Failed to append material");
    }
    if ( !materialIndices.put(value, index) ) {
        materials.remove(materials.size() - 1);
        throw std::runtime_error("Failed to index material");
    }
    return index;
}

int
BinaryModelWriter::SerializationContext::ensureVertex(const Vertex *value) {
    if ( value == nullptr ) {
        return -1;
    }
    int existingIndex = -1;
    if ( vertexIndices.tryGet(value, &existingIndex) ) {
        return existingIndex;
    }

    if ( value->radianceData != nullptr ) {
        throw std::runtime_error("Vertex radianceData is not supported by BinaryModelWritter");
    }

    const int index = static_cast<int>(vertices.size());
    if ( !vertices.add(value) ) {
        throw std::runtime_error("Failed to append vertex");
    }
    if ( !vertexIndices.put(value, index) ) {
        vertices.remove(vertices.size() - 1);
        throw std::runtime_error("Failed to index vertex");
    }

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
BinaryModelWriter::SerializationContext::ensurePatch(const Patch *value) {
    if ( value == nullptr ) {
        return -1;
    }
    int existingIndex = -1;
    if ( patchIndices.tryGet(value, &existingIndex) ) {
        return existingIndex;
    }

    if ( value->radianceData != nullptr ) {
        throw std::runtime_error("Patch radianceData is not supported by BinaryModelWritter");
    }

    const int index = static_cast<int>(patches.size());
    if ( !patches.add(value) ) {
        throw std::runtime_error("Failed to append patch");
    }
    if ( !patchIndices.put(value, index) ) {
        patches.remove(patches.size() - 1);
        throw std::runtime_error("Failed to index patch");
    }

    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        ensureVertex(value->vertex[i]);
    }
    ensurePatch(value->twin);
    ensureMaterial(value->material);

    return index;
}

int
BinaryModelWriter::SerializationContext::ensureGeometry(const Geometry *value) {
    if ( value == nullptr ) {
        return -1;
    }
    int existingIndex = -1;
    if ( geometryIndices.tryGet(value, &existingIndex) ) {
        return existingIndex;
    }

    if ( value->radianceData != nullptr ) {
        throw std::runtime_error("Geometry radianceData is not supported by BinaryModelWritter");
    }

    const int index = static_cast<int>(geometries.size());
    if ( !geometries.add(value) ) {
        throw std::runtime_error("Failed to append geometry");
    }
    if ( !geometryIndices.put(value, index) ) {
        geometries.remove(geometries.size() - 1);
        throw std::runtime_error("Failed to index geometry");
    }

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
BinaryModelWriter::SerializationContext::ensureColorContext(const ColorContext *value) {
    if ( value == nullptr ) {
        return -1;
    }
    int existingIndex = -1;
    if ( colorContextIndices.tryGet(value, &existingIndex) ) {
        return existingIndex;
    }
    const int index = static_cast<int>(colorContexts.size());
    if ( !colorContexts.add(value) ) {
        throw std::runtime_error("Failed to append color context");
    }
    if ( !colorContextIndices.put(value, index) ) {
        colorContexts.remove(colorContexts.size() - 1);
        throw std::runtime_error("Failed to index color context");
    }
    return index;
}

int
BinaryModelWriter::SerializationContext::ensureReaderContext(const ReaderContext *value) {
    if ( value == nullptr ) {
        return -1;
    }
    int existingIndex = -1;
    if ( readerContextIndices.tryGet(value, &existingIndex) ) {
        return existingIndex;
    }
    const int index = static_cast<int>(readerContexts.size());
    if ( !readerContexts.add(value) ) {
        throw std::runtime_error("Failed to append reader context");
    }
    if ( !readerContextIndices.put(value, index) ) {
        readerContexts.remove(readerContexts.size() - 1);
        throw std::runtime_error("Failed to index reader context");
    }

    ensureReaderContext(value->prev);
    return index;
}

int
BinaryModelWriter::SerializationContext::ensureTransformArray(const TransformArray *value) {
    if ( value == nullptr ) {
        return -1;
    }
    int existingIndex = -1;
    if ( transformArrayIndices.tryGet(value, &existingIndex) ) {
        return existingIndex;
    }
    const int index = static_cast<int>(transformArrays.size());
    if ( !transformArrays.add(value) ) {
        throw std::runtime_error("Failed to append transform array");
    }
    if ( !transformArrayIndices.put(value, index) ) {
        transformArrays.remove(transformArrays.size() - 1);
        throw std::runtime_error("Failed to index transform array");
    }
    return index;
}

int
BinaryModelWriter::SerializationContext::ensureTransformContext(const TransformContext *value) {
    if ( value == nullptr ) {
        return -1;
    }
    int existingIndex = -1;
    if ( transformContextIndices.tryGet(value, &existingIndex) ) {
        return existingIndex;
    }
    const int index = static_cast<int>(transformContexts.size());
    if ( !transformContexts.add(value) ) {
        throw std::runtime_error("Failed to append transform context");
    }
    if ( !transformContextIndices.put(value, index) ) {
        transformContexts.remove(transformContexts.size() - 1);
        throw std::runtime_error("Failed to index transform context");
    }

    ensureTransformArray(value->transformationArray);
    ensureTransformContext(value->prev);
    return index;
}

void
BinaryModelWriter::SerializationContext::collectVectorList(const java::ArrayList<Vector3D *> *list) {
    if ( list == nullptr ) {
        return;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        ensureVector(list->get(i));
    }
}

void
BinaryModelWriter::SerializationContext::collectVertexList(const java::ArrayList<Vertex *> *list) {
    if ( list == nullptr ) {
        return;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        ensureVertex(list->get(i));
    }
}

void
BinaryModelWriter::SerializationContext::collectPatchList(const java::ArrayList<Patch *> *list) {
    if ( list == nullptr ) {
        return;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        ensurePatch(list->get(i));
    }
}

void
BinaryModelWriter::SerializationContext::collectMaterialList(const java::ArrayList<Material *> *list) {
    if ( list == nullptr ) {
        return;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        ensureMaterial(list->get(i));
    }
}

void
BinaryModelWriter::SerializationContext::collectGeometryList(const java::ArrayList<Geometry *> *list) {
    if ( list == nullptr ) {
        return;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        ensureGeometry(list->get(i));
    }
}

void
BinaryModelWriter::SerializationContext::collectModel(const PersistedSceneModel *model) {
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
BinaryModelWriter::writeMaterialRecord(java::io::OutputStream &output, const Material *material) {
    writeString(output, material->getName());
    vsdk::PersistenceElement::writeBool(output, material->isSided());

    const PhongEmittanceDistributionFunction *edf = material->getEdf();
    vsdk::PersistenceElement::writeBool(output, edf != nullptr);
    if ( edf != nullptr ) {
        writeColor(output, edf->getKd());
        writeColor(output, edf->getKs());
        vsdk::PersistenceElement::writeFloatLE(output, edf->getNs());
    }

    const PhongBidirectionalScatteringDistributionFunction *bsdf = material->getBsdf();
    vsdk::PersistenceElement::writeBool(output, bsdf != nullptr);
    if ( bsdf == nullptr ) {
        return;
    }

    const PhongBidirectionalReflectanceDistributionFunction *brdf = bsdf->getBrdf();
    vsdk::PersistenceElement::writeBool(output, brdf != nullptr);
    if ( brdf != nullptr ) {
        writeColor(output, brdf->getKd());
        writeColor(output, brdf->getKs());
        vsdk::PersistenceElement::writeFloatLE(output, brdf->getNs());
    }

    const PhongBidirectionalTransmittanceDistributionFunction *btdf = bsdf->getBtdf();
    vsdk::PersistenceElement::writeBool(output, btdf != nullptr);
    if ( btdf != nullptr ) {
        writeColor(output, btdf->getKd());
        writeColor(output, btdf->getKs());
        vsdk::PersistenceElement::writeFloatLE(output, btdf->getNs());
        vsdk::PersistenceElement::writeFloatLE(output, btdf->getRefractionIndex().getNr());
        vsdk::PersistenceElement::writeFloatLE(output, btdf->getRefractionIndex().getNi());
    }

    const Texture *texture = bsdf->getTexture();
    vsdk::PersistenceElement::writeBool(output, texture != nullptr);
    if ( texture != nullptr ) {
        const int width = texture->getWidth();
        const int height = texture->getHeight();
        const int channels = texture->getChannels();
        if ( width < 0 || height < 0 || channels < 0 ) {
            throw std::runtime_error("Invalid texture dimensions");
        }

        vsdk::PersistenceElement::writeInt32LE(output, width);
        vsdk::PersistenceElement::writeInt32LE(output, height);
        vsdk::PersistenceElement::writeInt32LE(output, channels);

        const int64_t dataBytes = static_cast<int64_t>(width)
                                  * static_cast<int64_t>(height)
                                  * static_cast<int64_t>(channels);
        vsdk::PersistenceElement::writeInt64LE(output, dataBytes);

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
BinaryModelWriter::writeColorContextRecord(java::io::OutputStream &output, const ColorContext *colorContext) {
    vsdk::PersistenceElement::writeInt32LE(output, colorContext->clock);
    vsdk::PersistenceElement::writeSignedShortLE(output, colorContext->flags);
    for ( int i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
        vsdk::PersistenceElement::writeSignedShortLE(output, colorContext->straightSamples[i]);
    }
    vsdk::PersistenceElement::writeInt64LE(output, static_cast<int64_t>(colorContext->spectralStraightSum));
    vsdk::PersistenceElement::writeFloatLE(output, colorContext->cx);
    vsdk::PersistenceElement::writeFloatLE(output, colorContext->cy);
    vsdk::PersistenceElement::writeFloatLE(output, colorContext->eff);
}

void
BinaryModelWriter::writeReaderContextRecord(
    java::io::OutputStream &output,
    const ReaderContext *readerContext,
    const BinaryModelWriter::SerializationContext &context)
{
    vsdk::PersistenceElement::writeBytes(
        output,
        reinterpret_cast<const unsigned char *>(readerContext->fileName),
        96);
    vsdk::PersistenceElement::writeBool(output, readerContext->inputStream != nullptr);
    vsdk::PersistenceElement::writeInt32LE(output, readerContext->fileContextId);
    vsdk::PersistenceElement::writeBytes(
        output,
        reinterpret_cast<const unsigned char *>(readerContext->inputLine),
        MGF_MAXIMUM_INPUT_LINE_LENGTH);
    vsdk::PersistenceElement::writeInt32LE(output, readerContext->lineNumber);
    vsdk::PersistenceElement::writeByte(output, static_cast<unsigned char>(readerContext->isPipe));
    vsdk::PersistenceElement::writeInt32LE(
        output,
        indexOfPointer(readerContext->prev, context.readerContextIndices, "readerContext.prev"));
}

void
BinaryModelWriter::writeTransformArrayRecord(java::io::OutputStream &output, const TransformArray *transformArray) {
    vsdk::PersistenceElement::writeInt32LE(output, transformArray->startingPosition.fid);
    vsdk::PersistenceElement::writeInt32LE(output, transformArray->startingPosition.lineno);
    vsdk::PersistenceElement::writeInt64LE(output, static_cast<int64_t>(transformArray->startingPosition.offset));
    vsdk::PersistenceElement::writeInt32LE(output, transformArray->numberOfDimensions);
    for ( int i = 0; i < TRANSFORM_MAXIMUM_DIMENSIONS; i++ ) {
        vsdk::PersistenceElement::writeSignedShortLE(output, transformArray->transformArguments[i].i);
        vsdk::PersistenceElement::writeSignedShortLE(output, transformArray->transformArguments[i].n);
        vsdk::PersistenceElement::writeBytes(
            output,
            reinterpret_cast<const unsigned char *>(transformArray->transformArguments[i].arg),
            8);
    }
}

void
BinaryModelWriter::writeTransformContextRecord(
    java::io::OutputStream &output,
    const TransformContext *transformContext,
    const BinaryModelWriter::SerializationContext &context)
{
    vsdk::PersistenceElement::writeInt64LE(output, static_cast<int64_t>(transformContext->xid));
    vsdk::PersistenceElement::writeSignedShortLE(output, transformContext->xac);
    vsdk::PersistenceElement::writeSignedShortLE(output, transformContext->rev);

    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            vsdk::PersistenceElement::writeDoubleLE(output, transformContext->xf.transformMatrix.m[i][j]);
        }
    }
    vsdk::PersistenceElement::writeDoubleLE(output, transformContext->xf.scaleFactor);

    vsdk::PersistenceElement::writeInt32LE(
        output,
        indexOfPointer(
            transformContext->transformationArray,
            context.transformArrayIndices,
            "transformContext.transformationArray"));
    vsdk::PersistenceElement::writeInt32LE(
        output,
        indexOfPointer(
            transformContext->prev,
            context.transformContextIndices,
            "transformContext.prev"));
}

void
BinaryModelWriter::writeVertexRecord(java::io::OutputStream &output, const Vertex *vertex, const BinaryModelWriter::SerializationContext &context) {
    vsdk::PersistenceElement::writeInt32LE(output, vertex->id);
    vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(vertex->point, context.vectorIndices, "vertex.point"));
    vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(vertex->normal, context.vectorIndices, "vertex.normal"));
    vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(vertex->textureCoordinates, context.vectorIndices, "vertex.textureCoordinates"));
    writeColor(output, vertex->color);
    vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(vertex->back, context.vertexIndices, "vertex.back"));
    vsdk::PersistenceElement::writeInt32LE(output, vertex->tmp);
    vsdk::PersistenceElement::writeBool(output, vertex->radianceData != nullptr);
    writeIndexList(output, vertex->patches, context.patchIndices, "vertex.patches");
}

void
BinaryModelWriter::writePatchRecord(java::io::OutputStream &output, const Patch *patch, const BinaryModelWriter::SerializationContext &context) {
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(patch->id));
    vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(patch->twin, context.patchIndices, "patch.twin"));
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(patch->numberOfVertices));
    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(patch->vertex[i], context.vertexIndices, "patch.vertex"));
    }

    vsdk::PersistenceElement::writeBool(output, patch->boundingBox != nullptr);
    if ( patch->boundingBox != nullptr ) {
        writeBoundingBox(output, *patch->boundingBox);
    }

    writeVector(output, patch->normal);
    vsdk::PersistenceElement::writeFloatLE(output, patch->planeConstant);
    vsdk::PersistenceElement::writeFloatLE(output, patch->tolerance);
    vsdk::PersistenceElement::writeFloatLE(output, patch->area);
    writeVector(output, patch->midPoint);

    vsdk::PersistenceElement::writeBool(output, patch->jacobian != nullptr);
    if ( patch->jacobian != nullptr ) {
        vsdk::PersistenceElement::writeFloatLE(output, patch->jacobian->A);
        vsdk::PersistenceElement::writeFloatLE(output, patch->jacobian->B);
        vsdk::PersistenceElement::writeFloatLE(output, patch->jacobian->C);
    }

    vsdk::PersistenceElement::writeFloatLE(output, patch->directPotential);
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(patch->index));
    vsdk::PersistenceElement::writeBool(output, patch->omit != 0);
    vsdk::PersistenceElement::writeByte(output, patch->getFlags());
    writeColor(output, patch->color);
    vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(patch->material, context.materialIndices, "patch.material"));
    vsdk::PersistenceElement::writeBool(output, patch->radianceData != nullptr);
}

void
BinaryModelWriter::writeGeometryRecord(java::io::OutputStream &output, const Geometry *geometry, const BinaryModelWriter::SerializationContext &context) {
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(geometry->className));
    vsdk::PersistenceElement::writeInt32LE(output, geometry->id);
    vsdk::PersistenceElement::writeInt32LE(output, geometry->itemCount);
    vsdk::PersistenceElement::writeBool(output, geometry->bounded != 0);
    vsdk::PersistenceElement::writeBool(output, geometry->shaftCullGeometry != 0);
    vsdk::PersistenceElement::writeBool(output, geometry->omit != 0);
    vsdk::PersistenceElement::writeBool(output, geometry->isDuplicate);
    writeBoundingBox(output, geometry->boundingBox);
    vsdk::PersistenceElement::writeBool(output, geometry->rayIntersectionBox != nullptr);
    vsdk::PersistenceElement::writeBool(output, geometry->radianceData != nullptr);

    if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
        const MeshSurface *surface = static_cast<const MeshSurface *>(geometry);
        writeString(output, surface->objectName);
        vsdk::PersistenceElement::writeInt32LE(output, surface->meshId);
        vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(surface->material, context.materialIndices, "surface.material"));
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
BinaryModelWriter::writeModelRecord(java::io::OutputStream &output, const PersistedSceneModel *model, const BinaryModelWriter::SerializationContext &context) {
    vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(model->currentColor, context.colorContextIndices, "model.currentColor"));
    writeString(output, model->currentMaterialName);
    writeString(output, model->currentObjectName);
    writeString(output, model->currentVertexName);
    vsdk::PersistenceElement::writeInt32LE(output, model->geometryStackHeadIndex);
    vsdk::PersistenceElement::writeBool(output, model->inComplex);
    vsdk::PersistenceElement::writeBool(output, model->inSurface);
    vsdk::PersistenceElement::writeBool(output, model->monochrome);
    vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(model->readerContext, context.readerContextIndices, "model.readerContext"));
    vsdk::PersistenceElement::writeInt32LE(output, indexOfPointer(model->transformContext, context.transformContextIndices, "model.transformContext"));

    writeIndexList(output, model->currentFaceList, context.patchIndices, "model.currentFaceList");
    writeIndexList(output, model->currentGeometryList, context.geometryIndices, "model.currentGeometryList");
    writeIndexList(output, model->currentNormalList, context.vectorIndices, "model.currentNormalList");
    writeIndexList(output, model->currentPointList, context.vectorIndices, "model.currentPointList");
    writeIndexList(output, model->currentVertexList, context.vertexIndices, "model.currentVertexList");
    writeIndexList(output, model->geometries, context.geometryIndices, "model.geometries");
    writeIndexList(output, model->materials, context.materialIndices, "model.materials");
}

bool
BinaryModelWriter::write(const PersistedSceneModel *model, const char *fileName) {
    if ( model == nullptr || fileName == nullptr || fileName[0] == '\0' ) {
        return false;
    }

    java::io::FileOutputStream output;
    if ( !output.open(fileName) ) {
        return false;
    }

    bool ok = false;
    try {
        SerializationContext context;
        context.collectModel(model);

        vsdk::PersistenceElement::writeBytes(output, BINARY_MODEL_MAGIC, 16);
        vsdk::PersistenceElement::writeInt32LE(output, BINARY_MODEL_VERSION);
        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(sizeof(void *)));
        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(sizeof(long)));
        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(sizeof(PersistedSceneModel)));

        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(context.vectors.size()));
        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(context.vertices.size()));
        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(context.patches.size()));
        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(context.materials.size()));
        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(context.geometries.size()));
        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(context.colorContexts.size()));
        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(context.readerContexts.size()));
        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(context.transformArrays.size()));
        vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(context.transformContexts.size()));

        writeTag(output, "VEC3");
        for ( long int i = 0; i < context.vectors.size(); i++ ) {
            writeVector(output, *context.vectors.get(i));
        }

        writeTag(output, "MTLS");
        for ( long int i = 0; i < context.materials.size(); i++ ) {
            writeMaterialRecord(output, context.materials.get(i));
        }

        writeTag(output, "COLR");
        for ( long int i = 0; i < context.colorContexts.size(); i++ ) {
            writeColorContextRecord(output, context.colorContexts.get(i));
        }

        writeTag(output, "RCTX");
        for ( long int i = 0; i < context.readerContexts.size(); i++ ) {
            writeReaderContextRecord(output, context.readerContexts.get(i), context);
        }

        writeTag(output, "XFAR");
        for ( long int i = 0; i < context.transformArrays.size(); i++ ) {
            writeTransformArrayRecord(output, context.transformArrays.get(i));
        }

        writeTag(output, "XFCT");
        for ( long int i = 0; i < context.transformContexts.size(); i++ ) {
            writeTransformContextRecord(output, context.transformContexts.get(i), context);
        }

        writeTag(output, "VRTX");
        for ( long int i = 0; i < context.vertices.size(); i++ ) {
            writeVertexRecord(output, context.vertices.get(i), context);
        }

        writeTag(output, "PTCH");
        for ( long int i = 0; i < context.patches.size(); i++ ) {
            writePatchRecord(output, context.patches.get(i), context);
        }

        writeTag(output, "GEOM");
        for ( long int i = 0; i < context.geometries.size(); i++ ) {
            writeGeometryRecord(output, context.geometries.get(i), context);
        }

        writeTag(output, "MODL");
        writeModelRecord(output, model, context);

        ok = true;
    } catch ( const std::exception &e ) {
        logError("BinaryModelWritter::write", "%s", e.what());
        ok = false;
    }
    output.close();
    return ok;
}
