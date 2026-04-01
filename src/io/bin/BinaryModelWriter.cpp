#include <cstring>

#include "java/io/FileOutputStream.h"
#include "java/lang/Integer.h"
#include "java/util/ArrayList.txx"
#include "java/util/HashMap.txx"
#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "common/Error.h"
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
#include "io/wrapper/PersistenceElement.h"
#include "io/context/ColorContext.h"
#include "io/context/PersistedSceneModel.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformArray.h"
#include "io/context/TransformStackContext.h"
#include "io/bin/BinaryModelWriter.h"
#include "io/bin/BinaryModelWriterSerializationContext.h"

const unsigned char BinaryModelWriter::BINARY_MODEL_MAGIC[16] = {
    'R', 'P', 'K', '_', 'M', 'G', 'F', '_',
    'B', 'I', 'N', '_', '1', 0, 0, 0
};

const int BinaryModelWriter::BINARY_MODEL_VERSION = 1;

const char *
BinaryModelWriter::safeLabel(const char *text) {
    if ( text == nullptr ) {
        return "(null)";
    }
    return text;
}

bool
BinaryModelWriter::writeBytesChunked(java::OutputStream &output, const unsigned char *data, long long length) {
    if ( length < 0 ) {
        Error::error("BinaryModelWriter::writeBytesChunked", "Negative block length");
        return false;
    }
    long long offset = 0;
    const long long maxChunk = static_cast<long long>(java::Integer::MAX_VALUE);
    while ( offset < length ) {
        const long long remaining = length - offset;
        const int chunk = static_cast<int>(remaining < maxChunk ? remaining : maxChunk);
        vsdk::PersistenceElement::writeBytes(output, data + offset, chunk);
        offset += static_cast<long long>(chunk);
    }
    return true;
}

void
BinaryModelWriter::writeTag(java::OutputStream &output, const char tag[4]) {
    vsdk::PersistenceElement::writeBytes(
        output,
        reinterpret_cast<const unsigned char *>(tag),
        4);
}

bool
BinaryModelWriter::checkedLongToInt32(long value, const char *what, int &result) {
    if ( value > static_cast<long>(java::Integer::MAX_VALUE)
         || value < static_cast<long>(java::Integer::MIN_VALUE) ) {
        Error::error("BinaryModelWriter::checkedLongToInt32", "Overflow converting to int32 for %s", safeLabel(what));
        return false;
    }
    result = static_cast<int>(value);
    return true;
}

bool
BinaryModelWriter::writeString(java::OutputStream &output, const char *text) {
    if ( text == nullptr ) {
        vsdk::PersistenceElement::writeInt32LE(output, -1);
        return true;
    }
    const long size = static_cast<long>(std::strlen(text));
    int sizeAsInt32 = 0;
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
BinaryModelWriter::writeColor(java::OutputStream &output, const ColorRgb &color) {
    vsdk::PersistenceElement::writeFloatLE(output, color.r);
    vsdk::PersistenceElement::writeFloatLE(output, color.g);
    vsdk::PersistenceElement::writeFloatLE(output, color.b);
}

void
BinaryModelWriter::writeVector(java::OutputStream &output, const Vector3D &vector) {
    vsdk::PersistenceElement::writeFloatLE(output, vector.x);
    vsdk::PersistenceElement::writeFloatLE(output, vector.y);
    vsdk::PersistenceElement::writeFloatLE(output, vector.z);
}

void
BinaryModelWriter::writeBoundingBox(java::OutputStream &output, const BoundingBox &boundingBox) {
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
    int &result)
{
    if ( ptr == nullptr ) {
        result = -1;
        return true;
    }
    int index = 0;
    if ( !indices.tryGet(ptr, &index) ) {
        Error::error("BinaryModelWriter::indexOfPointer", "Missing pointer index for %s", safeLabel(what));
        return false;
    }
    result = static_cast<int>(index);
    return true;
}

template <typename T>
bool
BinaryModelWriter::writeIndexList(
    java::OutputStream &output,
    const java::ArrayList<T *> *list,
    const java::HashMap<const T *, int> &indices,
    const char *what)
{
    if ( list == nullptr ) {
        vsdk::PersistenceElement::writeInt32LE(output, -1);
        return true;
    }

    int size = 0;
    if ( !checkedLongToInt32(list->size(), what, size) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, size);
    for ( int i = 0; i < size; i++ ) {
        const T *element = list->get(i);
        int elementIndex = -1;
        if ( !indexOfPointer(element, indices, what, elementIndex) ) {
            return false;
        }
        vsdk::PersistenceElement::writeInt32LE(output, elementIndex);
    }
    return true;
}

bool
BinaryModelWriter::writeMaterialRecord(java::OutputStream &output, const Material *material) {
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
            Error::error("BinaryModelWriter::writeMaterialRecord", "Invalid texture dimensions");
            return false;
        }

        vsdk::PersistenceElement::writeInt32LE(output, width);
        vsdk::PersistenceElement::writeInt32LE(output, height);
        vsdk::PersistenceElement::writeInt32LE(output, channels);

        const long long dataBytes = static_cast<long long>(width)
                                  * static_cast<long long>(height)
                                  * static_cast<long long>(channels);
        vsdk::PersistenceElement::writeInt64LE(output, dataBytes);

        if ( dataBytes > 0 ) {
            const unsigned char *data = texture->getData();
            if ( data == nullptr ) {
                Error::error("BinaryModelWriter::writeMaterialRecord", "Texture data is null with non-zero size");
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
BinaryModelWriter::writeColorContextRecord(java::OutputStream &output, const ColorContext *colorContext) {
    vsdk::PersistenceElement::writeInt32LE(output, colorContext->clock);
    vsdk::PersistenceElement::writeSignedShortLE(output, colorContext->flags);
    for ( int i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
        vsdk::PersistenceElement::writeSignedShortLE(output, colorContext->straightSamples[i]);
    }
    vsdk::PersistenceElement::writeInt64LE(output, static_cast<long long>(colorContext->spectralStraightSum));
    vsdk::PersistenceElement::writeFloatLE(output, colorContext->cx);
    vsdk::PersistenceElement::writeFloatLE(output, colorContext->cy);
    vsdk::PersistenceElement::writeFloatLE(output, colorContext->eff);
}

bool
BinaryModelWriter::writeReaderContextRecord(
    java::OutputStream &output,
    const ReaderContext *readerContext,
    const BinaryModelWriterSerializationContext &context)
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

    int previousIndex = -1;
    if ( !indexOfPointer(readerContext->prev, context.readerContextIndices, "readerContext.prev", previousIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, previousIndex);
    return true;
}

void
BinaryModelWriter::writeTransformArrayRecord(java::OutputStream &output, const TransformArray *transformArray) {
    vsdk::PersistenceElement::writeInt32LE(output, transformArray->startingPosition.fileId);
    vsdk::PersistenceElement::writeInt32LE(output, transformArray->startingPosition.lineNumber);
    vsdk::PersistenceElement::writeInt64LE(output, static_cast<long long>(transformArray->startingPosition.offset));
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
    java::OutputStream &output,
    const TransformStackContext *transformContext,
    const BinaryModelWriterSerializationContext &context)
{
    vsdk::PersistenceElement::writeInt64LE(output, static_cast<long long>(transformContext->xid));
    vsdk::PersistenceElement::writeSignedShortLE(output, transformContext->xac);
    vsdk::PersistenceElement::writeSignedShortLE(output, transformContext->rev);

    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            vsdk::PersistenceElement::writeDoubleLE(output, transformContext->xf.transformMatrix.m[i][j]);
        }
    }
    vsdk::PersistenceElement::writeDoubleLE(output, transformContext->xf.scaleFactor);

    int transformArrayIndex = -1;
    if ( !indexOfPointer(
             transformContext->transformationArray,
             context.transformArrayIndices,
             "transformContext.transformationArray",
             transformArrayIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, transformArrayIndex);

    int previousIndex = -1;
    if ( !indexOfPointer(transformContext->prev, context.transformContextIndices, "transformContext.prev", previousIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, previousIndex);
    return true;
}

bool
BinaryModelWriter::writeVertexRecord(java::OutputStream &output, const Vertex *vertex, const BinaryModelWriterSerializationContext &context) {
    vsdk::PersistenceElement::writeInt32LE(output, vertex->id);

    int pointIndex = -1;
    if ( !indexOfPointer(vertex->point, context.vectorIndices, "vertex.point", pointIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, pointIndex);

    int normalIndex = -1;
    if ( !indexOfPointer(vertex->normal, context.vectorIndices, "vertex.normal", normalIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, normalIndex);

    int textureIndex = -1;
    if ( !indexOfPointer(vertex->textureCoordinates, context.vectorIndices, "vertex.textureCoordinates", textureIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, textureIndex);

    writeColor(output, vertex->color);

    int backIndex = -1;
    if ( !indexOfPointer(vertex->back, context.vertexIndices, "vertex.back", backIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, backIndex);

    vsdk::PersistenceElement::writeInt32LE(output, vertex->tmp);
    vsdk::PersistenceElement::writeBool(output, vertex->radianceData != nullptr);
    return writeIndexList(output, vertex->patches, context.patchIndices, "vertex.patches");
}

bool
BinaryModelWriter::writePatchRecord(java::OutputStream &output, const Patch *patch, const BinaryModelWriterSerializationContext &context) {
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int>(patch->id));

    int twinIndex = -1;
    if ( !indexOfPointer(patch->twin, context.patchIndices, "patch.twin", twinIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, twinIndex);

    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int>(patch->numberOfVertices));
    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        int vertexIndex = -1;
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
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int>(patch->index));
    vsdk::PersistenceElement::writeBool(output, patch->omit != 0);
    vsdk::PersistenceElement::writeByte(output, patch->getFlags());
    writeColor(output, patch->color);

    int materialIndex = -1;
    if ( !indexOfPointer(patch->material, context.materialIndices, "patch.material", materialIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, materialIndex);

    vsdk::PersistenceElement::writeBool(output, patch->radianceData != nullptr);
    return true;
}

bool
BinaryModelWriter::writeGeometryRecord(java::OutputStream &output, const Geometry *geometry, const BinaryModelWriterSerializationContext &context) {
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int>(geometry->className));
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

        int materialIndex = -1;
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
        Error::error("BinaryModelWriter::writeGeometryRecord", "Unsupported geometry class while writing");
        return false;
    }
    return true;
}

bool
BinaryModelWriter::writeModelRecord(java::OutputStream &output, const PersistedSceneModel *model, const BinaryModelWriterSerializationContext &context) {
    int currentColorIndex = -1;
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

    int readerContextIndex = -1;
    if ( !indexOfPointer(model->readerContext, context.readerContextIndices, "model.readerContext", readerContextIndex) ) {
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, readerContextIndex);

    int transformContextIndex = -1;
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
        Error::error("BinaryModelWriter::write", "Invalid model or fileName");
        return false;
    }
    java::File file(fileName);
    if ( !file.canWrite() || file.isDirectory() ) {
        Error::error("BinaryModelWriter::write", "Could not open output file '%s'", fileName);
        return false;
    }

    java::FileOutputStream output(fileName);

    BinaryModelWriterSerializationContext context;
    if ( !context.collectModel(model) ) {
        output.close();
        return false;
    }

    vsdk::PersistenceElement::writeBytes(output, BINARY_MODEL_MAGIC, 16);
    vsdk::PersistenceElement::writeInt32LE(output, BINARY_MODEL_VERSION);
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int>(sizeof(void *)));
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int>(sizeof(long)));
    vsdk::PersistenceElement::writeInt32LE(output, static_cast<int>(sizeof(PersistedSceneModel)));

    int vectorsCount = 0;
    if ( !checkedLongToInt32(context.vectors.size(), "vectors count", vectorsCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, vectorsCount);

    int verticesCount = 0;
    if ( !checkedLongToInt32(context.vertices.size(), "vertices count", verticesCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, verticesCount);

    int patchesCount = 0;
    if ( !checkedLongToInt32(context.patches.size(), "patches count", patchesCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, patchesCount);

    int materialsCount = 0;
    if ( !checkedLongToInt32(context.materials.size(), "materials count", materialsCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, materialsCount);

    int geometriesCount = 0;
    if ( !checkedLongToInt32(context.geometries.size(), "geometries count", geometriesCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, geometriesCount);

    int colorContextsCount = 0;
    if ( !checkedLongToInt32(context.colorContexts.size(), "color contexts count", colorContextsCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, colorContextsCount);

    int readerContextsCount = 0;
    if ( !checkedLongToInt32(context.readerContexts.size(), "reader contexts count", readerContextsCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, readerContextsCount);

    int transformArraysCount = 0;
    if ( !checkedLongToInt32(context.transformArrays.size(), "transform arrays count", transformArraysCount) ) {
        output.close();
        return false;
    }
    vsdk::PersistenceElement::writeInt32LE(output, transformArraysCount);

    int transformContextsCount = 0;
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
