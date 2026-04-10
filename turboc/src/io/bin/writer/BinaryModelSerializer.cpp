#include <string.h>

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
#include "io/context/ParseSnapshotContext.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformSequenceContext.h"
#include "io/context/TransformStackContext.h"
#include "io/bin/writer/BinaryModelSerializer.h"
#include "io/bin/writer/BinaryModelSerializationGraph.h"

const unsigned char BinaryModelSerializer::BINARY_MODEL_MAGIC[16] = {
    'R', 'P', 'K', '_', 'M', 'G', 'F', '_',
    'B', 'I', 'N', '_', '1', 0, 0, 0
};

const int BinaryModelSerializer::BINARY_MODEL_VERSION = 1;

const char *
BinaryModelSerializer::safeLabel(const char *text) {
    if ( text == NULL ) {
        return "(null)";
    }
    return text;
}

bool
BinaryModelSerializer::writeBytesChunked(OutputStream &output, const unsigned char *data, long length) {
    if ( length < 0 ) {
        Error::error("BinaryModelSerializer::writeBytesChunked", "Negative block length");
        return false;
    }
    long offset = 0;
    const long maxChunk = ((long)(Integer::MAX_VALUE));
    while ( offset < length ) {
        const long remaining = length - offset;
        const int chunk = ((int)(remaining < maxChunk ? remaining : maxChunk));
        PersistenceElement::writeBytes(output, data + offset, chunk);
        offset += ((long)(chunk));
    }
    return true;
}

void
BinaryModelSerializer::writeTag(OutputStream &output, const char tag[4]) {
    PersistenceElement::writeBytes(
        output,
        ((const unsigned char *)(tag)),
        4);
}

bool
BinaryModelSerializer::checkedLongToInt32(long value, const char *what, int &result) {
    if ( value > ((long)(Integer::MAX_VALUE))
         || value < ((long)(Integer::MIN_VALUE)) ) {
        Error::error("BinaryModelSerializer::checkedLongToInt32", "Overflow converting to int32 for %s", safeLabel(what));
        return false;
    }
    result = ((int)(value));
    return true;
}

void
BinaryModelSerializer::writeInt64LE(OutputStream &output, long value) {
    // Serialize a signed 64-bit integer from a 32-bit long by sign-extension.
    const int low = ((int)(value));
    const int high = (value < 0) ? -1 : 0;
    PersistenceElement::writeInt32LE(output, low);
    PersistenceElement::writeInt32LE(output, high);
}

void
BinaryModelSerializer::writeDoubleLE(OutputStream &output, double value) {
    unsigned char raw[8];
    memcpy(raw, &value, 8);

    const unsigned short endianProbe = 1;
    const bool littleEndian = (*((const unsigned char *)(&endianProbe)) == 1);

    if ( littleEndian ) {
        PersistenceElement::writeBytes(output, raw, 8);
    } else {
        unsigned char le[8];
        for ( int i = 0; i < 8; i++ ) {
            le[i] = raw[7 - i];
        }
        PersistenceElement::writeBytes(output, le, 8);
    }
}

bool
BinaryModelSerializer::writeString(OutputStream &output, const char *text) {
    if ( text == NULL ) {
        PersistenceElement::writeInt32LE(output, -1);
        return true;
    }
    const long size = ((long)(strlen(text)));
    int sizeAsInt32 = 0;
    if ( !checkedLongToInt32(size, "string length", sizeAsInt32) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, sizeAsInt32);
    if ( size > 0 ) {
        PersistenceElement::writeBytes(
            output,
            ((const unsigned char *)(text)),
            ((int)(size)));
    }
    return true;
}

void
BinaryModelSerializer::writeColor(OutputStream &output, const ColorRgb &color) {
    PersistenceElement::writeFloatLE(output, color.r);
    PersistenceElement::writeFloatLE(output, color.g);
    PersistenceElement::writeFloatLE(output, color.b);
}

void
BinaryModelSerializer::writeVector(OutputStream &output, const Vector3D &vector) {
    PersistenceElement::writeFloatLE(output, vector.x);
    PersistenceElement::writeFloatLE(output, vector.y);
    PersistenceElement::writeFloatLE(output, vector.z);
}

void
BinaryModelSerializer::writeBoundingBox(OutputStream &output, const BoundingBox &boundingBox) {
    for ( int i = 0; i < 6; i++ ) {
        PersistenceElement::writeFloatLE(output, boundingBox.valueAt(i));
    }
}

template <typename T>
bool
BinaryModelSerializer::indexOfPointer(
    const T *ptr,
    const HashMap<const T *, int> &indices,
    const char *what,
    int &result)
{
    if ( ptr == NULL ) {
        result = -1;
        return true;
    }
    int index = 0;
    if ( !indices.tryGet(ptr, &index) ) {
        Error::error("BinaryModelSerializer::indexOfPointer", "Missing pointer index for %s", safeLabel(what));
        return false;
    }
    result = ((int)(index));
    return true;
}

template <typename T>
bool
BinaryModelSerializer::writeIndexList(
    OutputStream &output,
    const ArrayList<T *> *list,
    const HashMap<const T *, int> &indices,
    const char *what)
{
    if ( list == NULL ) {
        PersistenceElement::writeInt32LE(output, -1);
        return true;
    }

    int size = 0;
    if ( !checkedLongToInt32(list->size(), what, size) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, size);
    for ( int i = 0; i < size; i++ ) {
        const T *element = list->get(i);
        int elementIndex = -1;
        if ( !indexOfPointer(element, indices, what, elementIndex) ) {
            return false;
        }
        PersistenceElement::writeInt32LE(output, elementIndex);
    }
    return true;
}

bool
BinaryModelSerializer::writeMaterialRecord(OutputStream &output, const Material *material) {
    if ( !writeString(output, material->getName()) ) {
        return false;
    }
    PersistenceElement::writeBool(output, material->isSided());

    const PhongEmitDistFunc *edf = material->getEdf();
    PersistenceElement::writeBool(output, edf != NULL);
    if ( edf != NULL ) {
        writeColor(output, edf->getKd());
        writeColor(output, edf->getKs());
        PersistenceElement::writeFloatLE(output, edf->getNs());
    }

    const PhongBidirScattDistFunc *bsdf = material->getBsdf();
    PersistenceElement::writeBool(output, bsdf != NULL);
    if ( bsdf == NULL ) {
        return true;
    }

    const PhongBidirReflDistFunc *brdf = bsdf->getBrdf();
    PersistenceElement::writeBool(output, brdf != NULL);
    if ( brdf != NULL ) {
        writeColor(output, brdf->getKd());
        writeColor(output, brdf->getKs());
        PersistenceElement::writeFloatLE(output, brdf->getNs());
    }

    const PhongBidirTransDistFunc *btdf = bsdf->getBtdf();
    PersistenceElement::writeBool(output, btdf != NULL);
    if ( btdf != NULL ) {
        writeColor(output, btdf->getKd());
        writeColor(output, btdf->getKs());
        PersistenceElement::writeFloatLE(output, btdf->getNs());
        PersistenceElement::writeFloatLE(output, btdf->getRefractionIndex().getNr());
        PersistenceElement::writeFloatLE(output, btdf->getRefractionIndex().getNi());
    }

    const Texture *texture = bsdf->getTexture();
    PersistenceElement::writeBool(output, texture != NULL);
    if ( texture != NULL ) {
        const int width = texture->getWidth();
        const int height = texture->getHeight();
        const int channels = texture->getChannels();
        if ( width < 0 || height < 0 || channels < 0 ) {
            Error::error("BinaryModelSerializer::writeMaterialRecord", "Invalid texture dimensions");
            return false;
        }

        PersistenceElement::writeInt32LE(output, width);
        PersistenceElement::writeInt32LE(output, height);
        PersistenceElement::writeInt32LE(output, channels);

        const long dataBytes = ((long)(width))
                                  * ((long)(height))
                                  * ((long)(channels));
        writeInt64LE(output, dataBytes);

        if ( dataBytes > 0 ) {
            const unsigned char *data = texture->getData();
            if ( data == NULL ) {
                Error::error("BinaryModelSerializer::writeMaterialRecord", "Texture data is null with non-zero size");
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
BinaryModelSerializer::writeColorContextRecord(OutputStream &output, const ColorContext *colorContext) {
    PersistenceElement::writeInt32LE(output, colorContext->clock);
    PersistenceElement::writeSignedShortLE(output, colorContext->flags);
    for ( int i = 0; i < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
        PersistenceElement::writeSignedShortLE(output, colorContext->straightSamples[i]);
    }
    writeInt64LE(output, ((long)(colorContext->spectralStraightSum)));
    PersistenceElement::writeFloatLE(output, colorContext->cx);
    PersistenceElement::writeFloatLE(output, colorContext->cy);
    PersistenceElement::writeFloatLE(output, colorContext->eff);
}

bool
BinaryModelSerializer::writeReaderContextRecord(
    OutputStream &output,
    const ReaderContext *readerContext,
    const BinaryModelSerializationGraph &context)
{
    PersistenceElement::writeBytes(
        output,
        ((const unsigned char *)(readerContext->fileName)),
        96);
    PersistenceElement::writeBool(output, readerContext->inputStream != NULL);
    PersistenceElement::writeInt32LE(output, readerContext->fileContextId);
    PersistenceElement::writeBytes(
        output,
        ((const unsigned char *)(readerContext->inputLine)),
        ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH);
    PersistenceElement::writeInt32LE(output, readerContext->lineNumber);
    PersistenceElement::writeByte(output, ((unsigned char)(readerContext->isPipe)));

    int previousIndex = -1;
    if ( !indexOfPointer(readerContext->prev, context.readerContextIndices, "readerContext.prev", previousIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, previousIndex);
    return true;
}

void
BinaryModelSerializer::writeTransformArrayRecord(OutputStream &output, const TransformSequenceContext *transformArray) {
    PersistenceElement::writeInt32LE(output, transformArray->startingPosition.fileId);
    PersistenceElement::writeInt32LE(output, transformArray->startingPosition.lineNumber);
    writeInt64LE(output, ((long)(transformArray->startingPosition.offset)));
    PersistenceElement::writeInt32LE(output, transformArray->numberOfDimensions);
    for ( int i = 0; i < TransformSequenceContext::TRANSFORM_MAXIMUM_DIMENSIONS; i++ ) {
        PersistenceElement::writeSignedShortLE(output, transformArray->transformArguments[i].i);
        PersistenceElement::writeSignedShortLE(output, transformArray->transformArguments[i].n);
        PersistenceElement::writeBytes(
            output,
            ((const unsigned char *)(transformArray->transformArguments[i].arg)),
            8);
    }
}

bool
BinaryModelSerializer::writeTransformContextRecord(
    OutputStream &output,
    const TransformStackContext *transformContext,
    const BinaryModelSerializationGraph &context)
{
    writeInt64LE(output, ((long)(transformContext->xid)));
    PersistenceElement::writeSignedShortLE(output, transformContext->xac);
    PersistenceElement::writeSignedShortLE(output, transformContext->rev);

    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            writeDoubleLE(output, transformContext->xf.transformMatrix.m[i][j]);
        }
    }
    writeDoubleLE(output, transformContext->xf.scaleFactor);

    int transformArrayIndex = -1;
    if ( !indexOfPointer(
             transformContext->transformationArray,
             context.transformArrayIndices,
             "transformContext.transformationArray",
             transformArrayIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, transformArrayIndex);

    int previousIndex = -1;
    if ( !indexOfPointer(transformContext->prev, context.transformContextIndices, "transformContext.prev", previousIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, previousIndex);
    return true;
}

bool
BinaryModelSerializer::writeVertexRecord(OutputStream &output, const Vertex *vertex, const BinaryModelSerializationGraph &context) {
    PersistenceElement::writeInt32LE(output, vertex->id);

    int pointIndex = -1;
    if ( !indexOfPointer(vertex->point, context.vectorIndices, "vertex.point", pointIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, pointIndex);

    int normalIndex = -1;
    if ( !indexOfPointer(vertex->normal, context.vectorIndices, "vertex.normal", normalIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, normalIndex);

    int textureIndex = -1;
    if ( !indexOfPointer(vertex->textureCoordinates, context.vectorIndices, "vertex.textureCoordinates", textureIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, textureIndex);

    writeColor(output, vertex->color);

    int backIndex = -1;
    if ( !indexOfPointer(vertex->back, context.vertexIndices, "vertex.back", backIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, backIndex);

    PersistenceElement::writeInt32LE(output, vertex->tmp);
    PersistenceElement::writeBool(output, vertex->radianceData != NULL);
    return writeIndexList(output, vertex->patches, context.patchIndices, "vertex.patches");
}

bool
BinaryModelSerializer::writePatchRecord(OutputStream &output, const Patch *patch, const BinaryModelSerializationGraph &context) {
    PersistenceElement::writeInt32LE(output, ((int)(patch->id)));

    int twinIndex = -1;
    if ( !indexOfPointer(patch->twin, context.patchIndices, "patch.twin", twinIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, twinIndex);

    PersistenceElement::writeInt32LE(output, ((int)(patch->numberOfVertices)));
    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        int vertexIndex = -1;
        if ( !indexOfPointer(patch->vertex[i], context.vertexIndices, "patch.vertex", vertexIndex) ) {
            return false;
        }
        PersistenceElement::writeInt32LE(output, vertexIndex);
    }

    PersistenceElement::writeBool(output, patch->boundingBox != NULL);
    if ( patch->boundingBox != NULL ) {
        writeBoundingBox(output, *patch->boundingBox);
    }

    writeVector(output, patch->normal);
    PersistenceElement::writeFloatLE(output, patch->planeConstant);
    PersistenceElement::writeFloatLE(output, patch->tolerance);
    PersistenceElement::writeFloatLE(output, patch->area);
    writeVector(output, patch->midPoint);

    PersistenceElement::writeBool(output, patch->jacobian != NULL);
    if ( patch->jacobian != NULL ) {
        PersistenceElement::writeFloatLE(output, patch->jacobian->A);
        PersistenceElement::writeFloatLE(output, patch->jacobian->B);
        PersistenceElement::writeFloatLE(output, patch->jacobian->C);
    }

    PersistenceElement::writeFloatLE(output, patch->directPotential);
    PersistenceElement::writeInt32LE(output, ((int)(patch->index)));
    PersistenceElement::writeBool(output, patch->omit != 0);
    PersistenceElement::writeByte(output, patch->getFlags());
    writeColor(output, patch->color);

    int materialIndex = -1;
    if ( !indexOfPointer(patch->material, context.materialIndices, "patch.material", materialIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, materialIndex);

    PersistenceElement::writeBool(output, patch->radianceData != NULL);
    return true;
}

bool
BinaryModelSerializer::writeGeometryRecord(OutputStream &output, const Geometry *geometry, const BinaryModelSerializationGraph &context) {
    PersistenceElement::writeInt32LE(output, ((int)(geometry->className)));
    PersistenceElement::writeInt32LE(output, geometry->id);
    PersistenceElement::writeInt32LE(output, geometry->itemCount);
    PersistenceElement::writeBool(output, geometry->bounded != 0);
    PersistenceElement::writeBool(output, geometry->shaftCullGeometry != 0);
    PersistenceElement::writeBool(output, geometry->omit != 0);
    PersistenceElement::writeBool(output, geometry->isDuplicate);
    writeBoundingBox(output, geometry->boundingBox);
    PersistenceElement::writeBool(output, geometry->rayIntersectionBox != NULL);
    PersistenceElement::writeBool(output, geometry->radianceData != NULL);

    if ( geometry->className == SURFACE_MESH ) {
        const MeshSurface *surface = ((const MeshSurface *)(geometry));
        if ( !writeString(output, surface->objectName) ) {
            return false;
        }
        PersistenceElement::writeInt32LE(output, surface->meshId);

        int materialIndex = -1;
        if ( !indexOfPointer(surface->material, context.materialIndices, "surface.material", materialIndex) ) {
            return false;
        }
        PersistenceElement::writeInt32LE(output, materialIndex);

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
    } else if ( geometry->className == COMPOUND ) {
        const Compound *compound = ((const Compound *)(geometry));
        if ( !writeIndexList(output, compound->children, context.geometryIndices, "compound.children") ) {
            return false;
        }
    } else if ( geometry->className == PATCH_SET ) {
        const PatchSet *patchSet = ((const PatchSet *)(geometry));
        if ( !writeIndexList(output, patchSet->getPatchList(), context.patchIndices, "patchSet.patchList") ) {
            return false;
        }
    } else {
        Error::error("BinaryModelSerializer::writeGeometryRecord", "Unsupported geometry class while writing");
        return false;
    }
    return true;
}

bool
BinaryModelSerializer::writeModelRecord(OutputStream &output, const ParseSnapshotContext *model, const BinaryModelSerializationGraph &context) {
    int currentColorIndex = -1;
    if ( !indexOfPointer(model->currentColor, context.colorContextIndices, "model.currentColor", currentColorIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, currentColorIndex);

    if ( !writeString(output, model->currentMaterialName) ) {
        return false;
    }
    if ( !writeString(output, model->currentObjectName) ) {
        return false;
    }
    if ( !writeString(output, model->currentVertexName) ) {
        return false;
    }

    PersistenceElement::writeInt32LE(output, model->geometryStackHeadIndex);
    PersistenceElement::writeBool(output, model->inComplex);
    PersistenceElement::writeBool(output, model->inSurface);
    PersistenceElement::writeBool(output, model->monochrome);

    int readerContextIndex = -1;
    if ( !indexOfPointer(model->readerContext, context.readerContextIndices, "model.readerContext", readerContextIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, readerContextIndex);

    int transformContextIndex = -1;
    if ( !indexOfPointer(model->transformContext, context.transformContextIndices, "model.transformContext", transformContextIndex) ) {
        return false;
    }
    PersistenceElement::writeInt32LE(output, transformContextIndex);

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
BinaryModelSerializer::write(const ParseSnapshotContext *model, const char *fileName) {
    if ( model == NULL || fileName == NULL || fileName[0] == '\0' ) {
        Error::error("BinaryModelSerializer::write", "Invalid model or fileName");
        return false;
    }
    File file(fileName);
    if ( !file.canWrite() || file.isDirectory() ) {
        Error::error("BinaryModelSerializer::write", "Could not open output file '%s'", fileName);
        return false;
    }

    FileOutputStream output(fileName);

    BinaryModelSerializationGraph context;
    if ( !context.collectModel(model) ) {
        output.close();
        return false;
    }

    PersistenceElement::writeBytes(output, BINARY_MODEL_MAGIC, 16);
    PersistenceElement::writeInt32LE(output, BINARY_MODEL_VERSION);
    PersistenceElement::writeInt32LE(output, ((int)(sizeof(void *))));
    PersistenceElement::writeInt32LE(output, ((int)(sizeof(long))));
    PersistenceElement::writeInt32LE(output, ((int)(sizeof(ParseSnapshotContext))));

    int vectorsCount = 0;
    if ( !checkedLongToInt32(context.vectors.size(), "vectors count", vectorsCount) ) {
        output.close();
        return false;
    }
    PersistenceElement::writeInt32LE(output, vectorsCount);

    int verticesCount = 0;
    if ( !checkedLongToInt32(context.vertices.size(), "vertices count", verticesCount) ) {
        output.close();
        return false;
    }
    PersistenceElement::writeInt32LE(output, verticesCount);

    int patchesCount = 0;
    if ( !checkedLongToInt32(context.patches.size(), "patches count", patchesCount) ) {
        output.close();
        return false;
    }
    PersistenceElement::writeInt32LE(output, patchesCount);

    int materialsCount = 0;
    if ( !checkedLongToInt32(context.materials.size(), "materials count", materialsCount) ) {
        output.close();
        return false;
    }
    PersistenceElement::writeInt32LE(output, materialsCount);

    int geometriesCount = 0;
    if ( !checkedLongToInt32(context.geometries.size(), "geometries count", geometriesCount) ) {
        output.close();
        return false;
    }
    PersistenceElement::writeInt32LE(output, geometriesCount);

    int colorContextsCount = 0;
    if ( !checkedLongToInt32(context.colorContexts.size(), "color contexts count", colorContextsCount) ) {
        output.close();
        return false;
    }
    PersistenceElement::writeInt32LE(output, colorContextsCount);

    int readerContextsCount = 0;
    if ( !checkedLongToInt32(context.readerContexts.size(), "reader contexts count", readerContextsCount) ) {
        output.close();
        return false;
    }
    PersistenceElement::writeInt32LE(output, readerContextsCount);

    int transformArraysCount = 0;
    if ( !checkedLongToInt32(context.transformArrays.size(), "transform arrays count", transformArraysCount) ) {
        output.close();
        return false;
    }
    PersistenceElement::writeInt32LE(output, transformArraysCount);

    int transformContextsCount = 0;
    if ( !checkedLongToInt32(context.transformContexts.size(), "transform contexts count", transformContextsCount) ) {
        output.close();
        return false;
    }
    PersistenceElement::writeInt32LE(output, transformContextsCount);

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
