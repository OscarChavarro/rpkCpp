#include "io/bin/BinaryModelWriter.h"

#include <cstring>
#include <limits>

#include "java/io/FileOutputStream.h"
#include "java/util/ArrayList.txx"
#include "java/util/HashMap.txx"
#include "common/error.h"
#include "common/ColorRgb.h"
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
#include "skin/MeshSurface.h"
#include "skin/Patch.h"
#include "skin/PatchSet.h"
#include "skin/Vertex.h"

const unsigned char BinaryModelWriter::BINARY_MODEL_MAGIC[16] = {
    'R', 'P', 'K', '_', 'M', 'G', 'F', '_',
    'B', 'I', 'N', '_', '1', 0, 0, 0
};
const int32_t BinaryModelWriter::BINARY_MODEL_VERSION = 1;

namespace {
const char *
safeLabel(const char *text) {
    if ( text == nullptr ) {
        return "(null)";
    }
    return text;
}
}

bool
BinaryModelWriter::writeBytesChunked(java::io::OutputStream &output, const unsigned char *data, int64_t length) {
    if ( length < 0 ) {
        logError("BinaryModelWriter::writeBytesChunked", "Negative block length");
        return false;
    }
    int64_t offset = 0;
    const int64_t maxChunk = static_cast<int64_t>(std::numeric_limits<int>::max());
    while ( offset < length ) {
        const int64_t remaining = length - offset;
        const int chunk = static_cast<int>(remaining < maxChunk ? remaining : maxChunk);
        vsdk::PersistenceElement::writeBytes(output, data + offset, chunk);
        offset += static_cast<int64_t>(chunk);
    }
    return true;
}

void
BinaryModelWriter::writeTag(java::io::OutputStream &output, const char tag[4]) {
    vsdk::PersistenceElement::writeBytes(
        output,
        reinterpret_cast<const unsigned char *>(tag),
        4);
}

bool
BinaryModelWriter::checkedLongToInt32(long value, const char *what, int32_t &result) {
    if ( value > static_cast<long>(std::numeric_limits<int32_t>::max())
         || value < static_cast<long>(std::numeric_limits<int32_t>::min()) ) {
        logError("BinaryModelWriter::checkedLongToInt32", "Overflow converting to int32 for %s", safeLabel(what));
        return false;
    }
    result = static_cast<int32_t>(value);
    return true;
}

bool
BinaryModelWriter::writeString(java::io::OutputStream &output, const char *text) {
    if ( text == nullptr ) {
        vsdk::PersistenceElement::writeInt32LE(output, -1);
        return true;
    }
    const long size = static_cast<long>(std::strlen(text));
    int32_t sizeAsInt32 = 0;
    if ( !checkedLongToInt32(size, "string length", sizeAsInt32) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, sizeAsInt32);
    if ( size > 0 ) {
        vsdk::PersistenceElement::writeBytes(
            output,
            reinterpret_cast<const unsigned char *>(text),
            static_cast<int>(size));
    }
    return true;
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
bool
BinaryModelWriter::indexOfPointer(
    const T *ptr,
    const java::HashMap<const T *, int> &indices,
    const char *what,
    int32_t &result)
{
    if ( ptr == nullptr ) {
        result = -1;
        return true;
    }
    int index = 0;
    if ( !indices.tryGet(ptr, &index) ) {
        logError("BinaryModelWriter::indexOfPointer", "Missing pointer index for %s", safeLabel(what));
        return false;
    }
    result = static_cast<int32_t>(index);
    return true;
}

template <typename T>
bool
BinaryModelWriter::writeIndexList(
    java::io::OutputStream &output,
    const java::ArrayList<T *> *list,
    const java::HashMap<const T *, int> &indices,
    const char *what)
{
    if ( list == nullptr ) {
        vsdk::PersistenceElement::writeInt32LE(output, -1);
        return true;
    }

    int32_t size = 0;
    if ( !checkedLongToInt32(list->size(), what, size) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, size);
    for ( int32_t i = 0; i < size; i++ ) {
        const T *element = list->get(i);
        int32_t elementIndex = -1;
        if ( !indexOfPointer(element, indices, what, elementIndex) ) {
            return false;
        }
        vsdk::PersistenceElement::writeInt32LE(output, elementIndex);
    }
    return true;
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

    java::HashMap<const TransformStackContext *, int> transformContextIndices;
    java::ArrayList<const TransformStackContext *> transformContexts;

    bool ensureVector(const Vector3D *value);
    bool ensureMaterial(const Material *value);
    bool ensureVertex(const Vertex *value);
    bool ensurePatch(const Patch *value);
    bool ensureGeometry(const Geometry *value);
    bool ensureColorContext(const ColorContext *value);
    bool ensureReaderContext(const ReaderContext *value);
    bool ensureTransformArray(const TransformArray *value);
    bool ensureTransformContext(const TransformStackContext *value);

    bool collectVectorList(const java::ArrayList<Vector3D *> *list);
    bool collectVertexList(const java::ArrayList<Vertex *> *list);
    bool collectPatchList(const java::ArrayList<Patch *> *list);
    bool collectMaterialList(const java::ArrayList<Material *> *list);
    bool collectGeometryList(const java::ArrayList<Geometry *> *list);
    bool collectModel(const PersistedSceneModel *model);
};

bool
BinaryModelWriter::SerializationContext::ensureVector(const Vector3D *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( vectorIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(vectors.size());
    if ( !vectors.add(value) ) {
        logError("BinaryModelWriter::SerializationContext::ensureVector", "Failed to append vector");
        return false;
    }
    if ( !vectorIndices.put(value, index) ) {
        vectors.remove(vectors.size() - 1);
        logError("BinaryModelWriter::SerializationContext::ensureVector", "Failed to index vector");
        return false;
    }
    return true;
}

bool
BinaryModelWriter::SerializationContext::ensureMaterial(const Material *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( materialIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(materials.size());
    if ( !materials.add(value) ) {
        logError("BinaryModelWriter::SerializationContext::ensureMaterial", "Failed to append material");
        return false;
    }
    if ( !materialIndices.put(value, index) ) {
        materials.remove(materials.size() - 1);
        logError("BinaryModelWriter::SerializationContext::ensureMaterial", "Failed to index material");
        return false;
    }
    return true;
}

bool
BinaryModelWriter::SerializationContext::ensureVertex(const Vertex *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( vertexIndices.tryGet(value, &existingIndex) ) {
        return true;
    }

    if ( value->radianceData != nullptr ) {
        logError("BinaryModelWriter::SerializationContext::ensureVertex", "Vertex radianceData is not supported by BinaryModelWritter");
        return false;
    }

    const int index = static_cast<int>(vertices.size());
    if ( !vertices.add(value) ) {
        logError("BinaryModelWriter::SerializationContext::ensureVertex", "Failed to append vertex");
        return false;
    }
    if ( !vertexIndices.put(value, index) ) {
        vertices.remove(vertices.size() - 1);
        logError("BinaryModelWriter::SerializationContext::ensureVertex", "Failed to index vertex");
        return false;
    }

    if ( !ensureVector(value->point) ) {
        return false;
    }
    if ( !ensureVector(value->normal) ) {
        return false;
    }
    if ( !ensureVector(value->textureCoordinates) ) {
        return false;
    }
    if ( !ensureVertex(value->back) ) {
        return false;
    }

    if ( value->patches != nullptr ) {
        for ( int i = 0; i < value->patches->size(); i++ ) {
            if ( !ensurePatch(value->patches->get(i)) ) {
                return false;
            }
        }
    }

    return true;
}

bool
BinaryModelWriter::SerializationContext::ensurePatch(const Patch *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( patchIndices.tryGet(value, &existingIndex) ) {
        return true;
    }

    if ( value->radianceData != nullptr ) {
        logError("BinaryModelWriter::SerializationContext::ensurePatch", "Patch radianceData is not supported by BinaryModelWritter");
        return false;
    }

    const int index = static_cast<int>(patches.size());
    if ( !patches.add(value) ) {
        logError("BinaryModelWriter::SerializationContext::ensurePatch", "Failed to append patch");
        return false;
    }
    if ( !patchIndices.put(value, index) ) {
        patches.remove(patches.size() - 1);
        logError("BinaryModelWriter::SerializationContext::ensurePatch", "Failed to index patch");
        return false;
    }

    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        if ( !ensureVertex(value->vertex[i]) ) {
            return false;
        }
    }
    if ( !ensurePatch(value->twin) ) {
        return false;
    }
    if ( !ensureMaterial(value->material) ) {
        return false;
    }

    return true;
}

bool
BinaryModelWriter::SerializationContext::ensureGeometry(const Geometry *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( geometryIndices.tryGet(value, &existingIndex) ) {
        return true;
    }

    if ( value->radianceData != nullptr ) {
        logError("BinaryModelWriter::SerializationContext::ensureGeometry", "Geometry radianceData is not supported by BinaryModelWritter");
        return false;
    }

    const int index = static_cast<int>(geometries.size());
    if ( !geometries.add(value) ) {
        logError("BinaryModelWriter::SerializationContext::ensureGeometry", "Failed to append geometry");
        return false;
    }
    if ( !geometryIndices.put(value, index) ) {
        geometries.remove(geometries.size() - 1);
        logError("BinaryModelWriter::SerializationContext::ensureGeometry", "Failed to index geometry");
        return false;
    }

    if ( value->className == GeometryClassId::SURFACE_MESH ) {
        const MeshSurface *surface = static_cast<const MeshSurface *>(value);
        if ( !ensureMaterial(surface->material) ) {
            return false;
        }
        if ( !collectVectorList(surface->positions) ) {
            return false;
        }
        if ( !collectVectorList(surface->normals) ) {
            return false;
        }
        if ( !collectVertexList(surface->vertices) ) {
            return false;
        }
        if ( !collectPatchList(surface->faces) ) {
            return false;
        }
    } else if ( value->className == GeometryClassId::COMPOUND ) {
        const Compound *compound = static_cast<const Compound *>(value);
        if ( !collectGeometryList(compound->children) ) {
            return false;
        }
    } else if ( value->className == GeometryClassId::PATCH_SET ) {
        const PatchSet *patchSet = static_cast<const PatchSet *>(value);
        if ( !collectPatchList(patchSet->getPatchList()) ) {
            return false;
        }
    } else {
        logError("BinaryModelWriter::SerializationContext::ensureGeometry", "Unsupported geometry class for BinaryModelWritter");
        return false;
    }

    return true;
}

bool
BinaryModelWriter::SerializationContext::ensureColorContext(const ColorContext *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( colorContextIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(colorContexts.size());
    if ( !colorContexts.add(value) ) {
        logError("BinaryModelWriter::SerializationContext::ensureColorContext", "Failed to append color context");
        return false;
    }
    if ( !colorContextIndices.put(value, index) ) {
        colorContexts.remove(colorContexts.size() - 1);
        logError("BinaryModelWriter::SerializationContext::ensureColorContext", "Failed to index color context");
        return false;
    }
    return true;
}

bool
BinaryModelWriter::SerializationContext::ensureReaderContext(const ReaderContext *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( readerContextIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(readerContexts.size());
    if ( !readerContexts.add(value) ) {
        logError("BinaryModelWriter::SerializationContext::ensureReaderContext", "Failed to append reader context");
        return false;
    }
    if ( !readerContextIndices.put(value, index) ) {
        readerContexts.remove(readerContexts.size() - 1);
        logError("BinaryModelWriter::SerializationContext::ensureReaderContext", "Failed to index reader context");
        return false;
    }

    return ensureReaderContext(value->prev);
}

bool
BinaryModelWriter::SerializationContext::ensureTransformArray(const TransformArray *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( transformArrayIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(transformArrays.size());
    if ( !transformArrays.add(value) ) {
        logError("BinaryModelWriter::SerializationContext::ensureTransformArray", "Failed to append transform array");
        return false;
    }
    if ( !transformArrayIndices.put(value, index) ) {
        transformArrays.remove(transformArrays.size() - 1);
        logError("BinaryModelWriter::SerializationContext::ensureTransformArray", "Failed to index transform array");
        return false;
    }
    return true;
}

bool
BinaryModelWriter::SerializationContext::ensureTransformContext(const TransformStackContext *value) {
    if ( value == nullptr ) {
        return true;
    }
    int existingIndex = -1;
    if ( transformContextIndices.tryGet(value, &existingIndex) ) {
        return true;
    }
    const int index = static_cast<int>(transformContexts.size());
    if ( !transformContexts.add(value) ) {
        logError("BinaryModelWriter::SerializationContext::ensureTransformContext", "Failed to append transform context");
        return false;
    }
    if ( !transformContextIndices.put(value, index) ) {
        transformContexts.remove(transformContexts.size() - 1);
        logError("BinaryModelWriter::SerializationContext::ensureTransformContext", "Failed to index transform context");
        return false;
    }

    if ( !ensureTransformArray(value->transformationArray) ) {
        return false;
    }
    return ensureTransformContext(value->prev);
}

bool
BinaryModelWriter::SerializationContext::collectVectorList(const java::ArrayList<Vector3D *> *list) {
    if ( list == nullptr ) {
        return true;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        if ( !ensureVector(list->get(i)) ) {
            return false;
        }
    }
    return true;
}

bool
BinaryModelWriter::SerializationContext::collectVertexList(const java::ArrayList<Vertex *> *list) {
    if ( list == nullptr ) {
        return true;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        if ( !ensureVertex(list->get(i)) ) {
            return false;
        }
    }
    return true;
}

bool
BinaryModelWriter::SerializationContext::collectPatchList(const java::ArrayList<Patch *> *list) {
    if ( list == nullptr ) {
        return true;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        if ( !ensurePatch(list->get(i)) ) {
            return false;
        }
    }
    return true;
}

bool
BinaryModelWriter::SerializationContext::collectMaterialList(const java::ArrayList<Material *> *list) {
    if ( list == nullptr ) {
        return true;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        if ( !ensureMaterial(list->get(i)) ) {
            return false;
        }
    }
    return true;
}

bool
BinaryModelWriter::SerializationContext::collectGeometryList(const java::ArrayList<Geometry *> *list) {
    if ( list == nullptr ) {
        return true;
    }
    for ( int i = 0; i < list->size(); i++ ) {
        if ( !ensureGeometry(list->get(i)) ) {
            return false;
        }
    }
    return true;
}

bool
BinaryModelWriter::SerializationContext::collectModel(const PersistedSceneModel *model) {
    if ( model == nullptr ) {
        return true;
    }

    if ( !ensureColorContext(model->currentColor) ) {
        return false;
    }
    if ( !collectPatchList(model->currentFaceList) ) {
        return false;
    }
    if ( !collectGeometryList(model->currentGeometryList) ) {
        return false;
    }
    if ( !collectMaterialList(model->materials) ) {
        return false;
    }
    if ( !collectVectorList(model->currentNormalList) ) {
        return false;
    }
    if ( !collectVectorList(model->currentPointList) ) {
        return false;
    }
    if ( !collectVertexList(model->currentVertexList) ) {
        return false;
    }
    if ( !collectGeometryList(model->geometries) ) {
        return false;
    }
    if ( !ensureReaderContext(model->readerContext) ) {
        return false;
    }
    return ensureTransformContext(model->transformContext);
}

bool
BinaryModelWriter::writeMaterialRecord(java::io::OutputStream &output, const Material *material) {
    if ( !writeString(output, material->getName()) ) {
        return false;
    }
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
        return true;
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
            logError("BinaryModelWriter::writeMaterialRecord", "Invalid texture dimensions");
            return false;
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
                logError("BinaryModelWriter::writeMaterialRecord", "Texture data is null with non-zero size");
                return false;
            }
            if ( !writeBytesChunked(output, data, dataBytes) ) {
                return false;
            }
        }
    }
    return true;
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

bool
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

    int32_t previousIndex = -1;
    if ( !indexOfPointer(readerContext->prev, context.readerContextIndices, "readerContext.prev", previousIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, previousIndex);
    return true;
}

void
BinaryModelWriter::writeTransformArrayRecord(java::io::OutputStream &output, const TransformArray *transformArray) {
    vsdk::PersistenceElement::writeInt32LE(output, transformArray->startingPosition.fileId);
    vsdk::PersistenceElement::writeInt32LE(output, transformArray->startingPosition.lineNumber);
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

bool
BinaryModelWriter::writeTransformContextRecord(
    java::io::OutputStream &output,
    const TransformStackContext *transformContext,
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

    int32_t transformArrayIndex = -1;
    if ( !indexOfPointer(
             transformContext->transformationArray,
             context.transformArrayIndices,
             "transformContext.transformationArray",
             transformArrayIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, transformArrayIndex);

    int32_t previousIndex = -1;
    if ( !indexOfPointer(transformContext->prev, context.transformContextIndices, "transformContext.prev", previousIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, previousIndex);
    return true;
}

bool
BinaryModelWriter::writeVertexRecord(java::io::OutputStream &output, const Vertex *vertex, const BinaryModelWriter::SerializationContext &context) {
    vsdk::PersistenceElement::writeInt32LE(output, vertex->id);

    int32_t pointIndex = -1;
    if ( !indexOfPointer(vertex->point, context.vectorIndices, "vertex.point", pointIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, pointIndex);

    int32_t normalIndex = -1;
    if ( !indexOfPointer(vertex->normal, context.vectorIndices, "vertex.normal", normalIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, normalIndex);

    int32_t textureIndex = -1;
    if ( !indexOfPointer(vertex->textureCoordinates, context.vectorIndices, "vertex.textureCoordinates", textureIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, textureIndex);

    writeColor(output, vertex->color);

    int32_t backIndex = -1;
    if ( !indexOfPointer(vertex->back, context.vertexIndices, "vertex.back", backIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, backIndex);

    vsdk::PersistenceElement::writeInt32LE(output, vertex->tmp);
    vsdk::PersistenceElement::writeBool(output, vertex->radianceData != nullptr);
    return writeIndexList(output, vertex->patches, context.patchIndices, "vertex.patches");
}

bool
BinaryModelWriter::writePatchRecord(java::io::OutputStream &output, const Patch *patch, const BinaryModelWriter::SerializationContext &context) {
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(patch->id));

    int32_t twinIndex = -1;
    if ( !indexOfPointer(patch->twin, context.patchIndices, "patch.twin", twinIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, twinIndex);

    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(patch->numberOfVertices));
    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        int32_t vertexIndex = -1;
        if ( !indexOfPointer(patch->vertex[i], context.vertexIndices, "patch.vertex", vertexIndex) ) {
            return false;
        }
        vsdk::PersistenceElement::writeInt32LE(output, vertexIndex);
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

    int32_t materialIndex = -1;
    if ( !indexOfPointer(patch->material, context.materialIndices, "patch.material", materialIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, materialIndex);

    vsdk::PersistenceElement::writeBool(output, patch->radianceData != nullptr);
    return true;
}

bool
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
        if ( !writeString(output, surface->objectName) ) {
            return false;
        }
        vsdk::PersistenceElement::writeInt32LE(output, surface->meshId);

        int32_t materialIndex = -1;
        if ( !indexOfPointer(surface->material, context.materialIndices, "surface.material", materialIndex) ) {
            return false;
        }
        vsdk::PersistenceElement::writeInt32LE(output, materialIndex);

        if ( !writeIndexList(output, surface->positions, context.vectorIndices, "surface.positions") ) {
            return false;
        }
        if ( !writeIndexList(output, surface->normals, context.vectorIndices, "surface.normals") ) {
            return false;
        }
        if ( !writeIndexList(output, surface->vertices, context.vertexIndices, "surface.vertices") ) {
            return false;
        }
        if ( !writeIndexList(output, surface->faces, context.patchIndices, "surface.faces") ) {
            return false;
        }
    } else if ( geometry->className == GeometryClassId::COMPOUND ) {
        const Compound *compound = static_cast<const Compound *>(geometry);
        if ( !writeIndexList(output, compound->children, context.geometryIndices, "compound.children") ) {
            return false;
        }
    } else if ( geometry->className == GeometryClassId::PATCH_SET ) {
        const PatchSet *patchSet = static_cast<const PatchSet *>(geometry);
        if ( !writeIndexList(output, patchSet->getPatchList(), context.patchIndices, "patchSet.patchList") ) {
            return false;
        }
    } else {
        logError("BinaryModelWriter::writeGeometryRecord", "Unsupported geometry class while writing");
        return false;
    }
    return true;
}

bool
BinaryModelWriter::writeModelRecord(java::io::OutputStream &output, const PersistedSceneModel *model, const BinaryModelWriter::SerializationContext &context) {
    int32_t currentColorIndex = -1;
    if ( !indexOfPointer(model->currentColor, context.colorContextIndices, "model.currentColor", currentColorIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, currentColorIndex);

    if ( !writeString(output, model->currentMaterialName) ) {
        return false;
    }
    if ( !writeString(output, model->currentObjectName) ) {
        return false;
    }
    if ( !writeString(output, model->currentVertexName) ) {
        return false;
    }

    vsdk::PersistenceElement::writeInt32LE(output, model->geometryStackHeadIndex);
    vsdk::PersistenceElement::writeBool(output, model->inComplex);
    vsdk::PersistenceElement::writeBool(output, model->inSurface);
    vsdk::PersistenceElement::writeBool(output, model->monochrome);

    int32_t readerContextIndex = -1;
    if ( !indexOfPointer(model->readerContext, context.readerContextIndices, "model.readerContext", readerContextIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, readerContextIndex);

    int32_t transformContextIndex = -1;
    if ( !indexOfPointer(model->transformContext, context.transformContextIndices, "model.transformContext", transformContextIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, transformContextIndex);

    if ( !writeIndexList(output, model->currentFaceList, context.patchIndices, "model.currentFaceList") ) {
        return false;
    }
    if ( !writeIndexList(output, model->currentGeometryList, context.geometryIndices, "model.currentGeometryList") ) {
        return false;
    }
    if ( !writeIndexList(output, model->currentNormalList, context.vectorIndices, "model.currentNormalList") ) {
        return false;
    }
    if ( !writeIndexList(output, model->currentPointList, context.vectorIndices, "model.currentPointList") ) {
        return false;
    }
    if ( !writeIndexList(output, model->currentVertexList, context.vertexIndices, "model.currentVertexList") ) {
        return false;
    }
    if ( !writeIndexList(output, model->geometries, context.geometryIndices, "model.geometries") ) {
        return false;
    }
    if ( !writeIndexList(output, model->materials, context.materialIndices, "model.materials") ) {
        return false;
    }
    return true;
}

bool
BinaryModelWriter::write(const PersistedSceneModel *model, const char *fileName) {
    if ( model == nullptr || fileName == nullptr || fileName[0] == '\0' ) {
        logError("BinaryModelWriter::write", "Invalid model or fileName");
        return false;
    }

    java::io::FileOutputStream output;
    if ( !output.open(fileName) ) {
        logError("BinaryModelWriter::write", "Could not open output file '%s'", fileName);
        output.close();
        return false;
    }

    SerializationContext context;
    if ( !context.collectModel(model) ) {
        output.close();
        return false;
    }

    vsdk::PersistenceElement::writeBytes(output, BINARY_MODEL_MAGIC, 16);
    vsdk::PersistenceElement::writeInt32LE(output, BINARY_MODEL_VERSION);
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(sizeof(void *)));
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(sizeof(long)));
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int32_t>(sizeof(PersistedSceneModel)));

    int32_t vectorsCount = 0;
    if ( !checkedLongToInt32(context.vectors.size(), "vectors count", vectorsCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, vectorsCount);

    int32_t verticesCount = 0;
    if ( !checkedLongToInt32(context.vertices.size(), "vertices count", verticesCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, verticesCount);

    int32_t patchesCount = 0;
    if ( !checkedLongToInt32(context.patches.size(), "patches count", patchesCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, patchesCount);

    int32_t materialsCount = 0;
    if ( !checkedLongToInt32(context.materials.size(), "materials count", materialsCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, materialsCount);

    int32_t geometriesCount = 0;
    if ( !checkedLongToInt32(context.geometries.size(), "geometries count", geometriesCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, geometriesCount);

    int32_t colorContextsCount = 0;
    if ( !checkedLongToInt32(context.colorContexts.size(), "color contexts count", colorContextsCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, colorContextsCount);

    int32_t readerContextsCount = 0;
    if ( !checkedLongToInt32(context.readerContexts.size(), "reader contexts count", readerContextsCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, readerContextsCount);

    int32_t transformArraysCount = 0;
    if ( !checkedLongToInt32(context.transformArrays.size(), "transform arrays count", transformArraysCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, transformArraysCount);

    int32_t transformContextsCount = 0;
    if ( !checkedLongToInt32(context.transformContexts.size(), "transform contexts count", transformContextsCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, transformContextsCount);

    writeTag(output, "VEC3");
    for ( long int i = 0; i < context.vectors.size(); i++ ) {
        writeVector(output, *context.vectors.get(i));
    }

    writeTag(output, "MTLS");
    for ( long int i = 0; i < context.materials.size(); i++ ) {
        if ( !writeMaterialRecord(output, context.materials.get(i)) ) {
            output.close();
            return false;
        }
    }

    writeTag(output, "COLR");
    for ( long int i = 0; i < context.colorContexts.size(); i++ ) {
        writeColorContextRecord(output, context.colorContexts.get(i));
    }

    writeTag(output, "RCTX");
    for ( long int i = 0; i < context.readerContexts.size(); i++ ) {
        if ( !writeReaderContextRecord(output, context.readerContexts.get(i), context) ) {
            output.close();
            return false;
        }
    }

    writeTag(output, "XFAR");
    for ( long int i = 0; i < context.transformArrays.size(); i++ ) {
        writeTransformArrayRecord(output, context.transformArrays.get(i));
    }

    writeTag(output, "XFCT");
    for ( long int i = 0; i < context.transformContexts.size(); i++ ) {
        if ( !writeTransformContextRecord(output, context.transformContexts.get(i), context) ) {
            output.close();
            return false;
        }
    }

    writeTag(output, "VRTX");
    for ( long int i = 0; i < context.vertices.size(); i++ ) {
        if ( !writeVertexRecord(output, context.vertices.get(i), context) ) {
            output.close();
            return false;
        }
    }

    writeTag(output, "PTCH");
    for ( long int i = 0; i < context.patches.size(); i++ ) {
        if ( !writePatchRecord(output, context.patches.get(i), context) ) {
            output.close();
            return false;
        }
    }

    writeTag(output, "GEOM");
    for ( long int i = 0; i < context.geometries.size(); i++ ) {
        if ( !writeGeometryRecord(output, context.geometries.get(i), context) ) {
            output.close();
            return false;
        }
    }

    writeTag(output, "MODL");
    if ( !writeModelRecord(output, model, context) ) {
        output.close();
        return false;
    }

    output.close();
    return true;
}
