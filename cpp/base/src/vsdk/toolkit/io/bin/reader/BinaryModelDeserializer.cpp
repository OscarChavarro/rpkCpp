#include "vsdk/toolkit/java/io/BufferedInputStream.h"
#include "vsdk/toolkit/java/io/FileInputStream.h"
#include "vsdk/toolkit/java/lang/Integer.h"
#include "vsdk/toolkit/java/util/ArrayList.txx"

#include "vsdk/toolkit/common/linealAlgebra/Jacobian.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/material/Material.h"
#include "vsdk/toolkit/material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"
#include "vsdk/toolkit/material/PhongBidirectionalTransmittanceDistributionFunction.h"
#include "vsdk/toolkit/material/PhongEmittanceDistributionFunction.h"
#include "vsdk/toolkit/material/Texture.h"
#include "vsdk/toolkit/skin/Compound.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/material/MaterialColorFlags.h"
#include "vsdk/toolkit/skin/MeshSurface.h"
#include "vsdk/toolkit/skin/MinMaxBox.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/PatchSet.h"
#include "vsdk/toolkit/environment/geometry/elements/Vertex.h"
#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/io/context/ParseSnapshotContext.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/context/TransformSequenceContext.h"
#include "vsdk/toolkit/io/context/TransformStackContext.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelDeserializer.h"
#include "vsdk/toolkit/io/bin/reader/ScopedArrayBuffer.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelVertexRecordData.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelPatchRecordData.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelGeometryRecordData.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelSnapshotRecordData.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelReadPrimitives.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelReadCleanup.h"

ParseSnapshotContext *
BinaryModelDeserializer::read(const char *fileName) {
    if ( fileName == nullptr || fileName[0] == '\0' ) {
        return nullptr;
    }
    java::File file(fileName);
    if ( !(file.exists() && file.canRead() && file.isFile()) ) {
        return nullptr;
    }

    java::FileInputStream fileInput(fileName);
    java::BufferedInputStream input(&fileInput);

    java::ArrayList<Vector3D *> vectors;
    java::ArrayList<Vertex *> vertices;
    java::ArrayList<Patch *> patches;
    java::ArrayList<Material *> materials;
    java::ArrayList<Geometry *> geometries;
    java::ArrayList<ColorContext *> colorContexts;
    java::ArrayList<ReaderContext *> readerContexts;
    java::ArrayList<TransformSequenceContext *> transformArrays;
    java::ArrayList<TransformStackContext *> transformContexts;
    java::ArrayList<BinaryModelVertexRecordData> vertexRecords;
    java::ArrayList<BinaryModelPatchRecordData> patchRecords;
    java::ArrayList<BinaryModelGeometryRecordData> geometryRecords;
    BinaryModelSnapshotRecordData modelRecord;
    ParseSnapshotContext *model = nullptr;
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
        if ( !BinaryModelReadPrimitives::validateBinaryHeader(input) ) goto fail;

        if ( !BinaryModelReadPrimitives::readNonNegativeCount(input, "vectors", &vectorCount) ) goto fail;
        if ( !BinaryModelReadPrimitives::readNonNegativeCount(input, "vertices", &vertexCount) ) goto fail;
        if ( !BinaryModelReadPrimitives::readNonNegativeCount(input, "patches", &patchCount) ) goto fail;
        if ( !BinaryModelReadPrimitives::readNonNegativeCount(input, "materials", &materialCount) ) goto fail;
        if ( !BinaryModelReadPrimitives::readNonNegativeCount(input, "geometries", &geometryCount) ) goto fail;
        if ( !BinaryModelReadPrimitives::readNonNegativeCount(input, "color contexts", &colorContextCount) ) goto fail;
        if ( !BinaryModelReadPrimitives::readNonNegativeCount(input, "reader contexts", &readerContextCount) ) goto fail;
        if ( !BinaryModelReadPrimitives::readNonNegativeCount(input, "transform arrays", &transformArrayCount) ) goto fail;
        if ( !BinaryModelReadPrimitives::readNonNegativeCount(input, "transform contexts", &transformContextCount) ) goto fail;

        if ( !BinaryModelReadPrimitives::initializeArrayList(&vectors, vectorCount, static_cast<Vector3D *>(nullptr), "vectors") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&vertices, vertexCount, static_cast<Vertex *>(nullptr), "vertices") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&patches, patchCount, static_cast<Patch *>(nullptr), "patches") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&materials, materialCount, static_cast<Material *>(nullptr), "materials") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&geometries, geometryCount, static_cast<Geometry *>(nullptr), "geometries") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&colorContexts, colorContextCount, static_cast<ColorContext *>(nullptr), "color contexts") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&readerContexts, readerContextCount, static_cast<ReaderContext *>(nullptr), "reader contexts") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&transformArrays, transformArrayCount, static_cast<TransformSequenceContext *>(nullptr), "transform arrays") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&transformContexts, transformContextCount, static_cast<TransformStackContext *>(nullptr), "transform contexts") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&vertexRecords, vertexCount, BinaryModelVertexRecordData(), "vertex records") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&patchRecords, patchCount, BinaryModelPatchRecordData(), "patch records") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&geometryRecords, geometryCount, BinaryModelGeometryRecordData(), "geometry records") ) goto fail;

        if ( !BinaryModelReadPrimitives::expectTag(input, "VEC3") ) goto fail;
        for ( int i = 0; i < vectorCount; i++ ) {
            Vector3D * const vector = new Vector3D();
            if ( !BinaryModelReadPrimitives::readVector(input, vector) ) {
                delete vector;
                goto fail;
            }
            vectors.set(static_cast<long int>(i), vector);
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "MTLS") ) goto fail;
        for ( int i = 0; i < materialCount; i++ ) {
            ScopedArrayBuffer<char> materialNameGuard;
            char *materialName = nullptr;
            bool hasMaterialName = false;
            if ( !BinaryModelReadPrimitives::readNullableString(input, &materialName, &hasMaterialName) ) goto fail;
            materialNameGuard.reset(materialName);
            const bool sided = BinaryModelReadPrimitives::readBool(input);

            PhongEmittanceDistributionFunction *edf = nullptr;
            const bool hasEdf = BinaryModelReadPrimitives::readBool(input);
            if ( hasEdf ) {
                ColorRgbMutable kd(0.0, 0.0, 0.0);
                ColorRgbMutable ks(0.0, 0.0, 0.0);
                if ( !BinaryModelReadPrimitives::readColor(input, &kd) ) goto fail;
                if ( !BinaryModelReadPrimitives::readColor(input, &ks) ) goto fail;
                const float ns = BinaryModelReadPrimitives::readFloatLE(input);
                edf = new PhongEmittanceDistributionFunction(&kd, &ks, ns);
            }

            PhongBidirectionalScatteringDistributionFunction *bsdf = nullptr;
            const bool hasBsdf = BinaryModelReadPrimitives::readBool(input);
            if ( hasBsdf ) {
                PhongBidirectionalReflectanceDistributionFunction *brdf = nullptr;
                PhongBidirectionalTransmittanceDistributionFunction *btdf = nullptr;
                Texture *texture = nullptr;

                const bool hasBrdf = BinaryModelReadPrimitives::readBool(input);
                if ( hasBrdf ) {
                    ColorRgbMutable kd(0.0, 0.0, 0.0);
                    ColorRgbMutable ks(0.0, 0.0, 0.0);
                    if ( !BinaryModelReadPrimitives::readColor(input, &kd) ) goto fail;
                    if ( !BinaryModelReadPrimitives::readColor(input, &ks) ) goto fail;
                    const float ns = BinaryModelReadPrimitives::readFloatLE(input);
                    brdf = new PhongBidirectionalReflectanceDistributionFunction(&kd, &ks, ns);
                }

                const bool hasBtdf = BinaryModelReadPrimitives::readBool(input);
                if ( hasBtdf ) {
                    ColorRgbMutable kd(0.0, 0.0, 0.0);
                    ColorRgbMutable ks(0.0, 0.0, 0.0);
                    if ( !BinaryModelReadPrimitives::readColor(input, &kd) ) goto fail;
                    if ( !BinaryModelReadPrimitives::readColor(input, &ks) ) goto fail;
                    const float ns = BinaryModelReadPrimitives::readFloatLE(input);
                    const float nr = BinaryModelReadPrimitives::readFloatLE(input);
                    const float ni = BinaryModelReadPrimitives::readFloatLE(input);
                    btdf = new PhongBidirectionalTransmittanceDistributionFunction(&kd, &ks, ns, nr, ni);
                }

                const bool hasTexture = BinaryModelReadPrimitives::readBool(input);
                if ( hasTexture ) {
                    const int width = BinaryModelReadPrimitives::readInt32LE(input);
                    const int height = BinaryModelReadPrimitives::readInt32LE(input);
                    const int channels = BinaryModelReadPrimitives::readInt32LE(input);
                    const long long dataBytes = BinaryModelReadPrimitives::readInt64LE(input);

                    if ( width < 0 || height < 0 || channels < 0 || dataBytes < 0 ) {
                        Logger::error("BinaryModelDeserializer::read", "%s", "Invalid texture dimensions in binary material");
                        goto fail;
                    }

                    const long long expectedBytes = static_cast<long long>(width)
                                                  * static_cast<long long>(height)
                                                  * static_cast<long long>(channels);
                    if ( expectedBytes != dataBytes ) {
                        Logger::error("BinaryModelDeserializer::read", "%s", "Texture byte count mismatch in binary material");
                        goto fail;
                    }

                    ScopedArrayBuffer<unsigned char> textureData;
                    if ( dataBytes > 0 ) {
                        if ( dataBytes > static_cast<long long>(java::Integer::MAX_VALUE) ) {
                            Logger::error("BinaryModelDeserializer::read", "%s", "Texture data too large for current platform");
                            goto fail;
                        }
                        textureData.reset(new unsigned char[static_cast<int>(dataBytes)]);
                        if ( !BinaryModelReadPrimitives::readBytesChunked(input, textureData.get(), dataBytes) ) goto fail;
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

        if ( !BinaryModelReadPrimitives::expectTag(input, "COLR") ) goto fail;
        for ( int i = 0; i < colorContextCount; i++ ) {
            ColorContext *colorContext = new ColorContext();
            colorContext->clock = BinaryModelReadPrimitives::readInt32LE(input);
            colorContext->flags = BinaryModelReadPrimitives::readInt16LE(input);
            for ( int j = 0; j < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; j++ ) {
                colorContext->straightSamples[j] = BinaryModelReadPrimitives::readInt16LE(input);
            }
            colorContext->spectralStraightSum = static_cast<long>(BinaryModelReadPrimitives::readInt64LE(input));
            colorContext->cx = BinaryModelReadPrimitives::readFloatLE(input);
            colorContext->cy = BinaryModelReadPrimitives::readFloatLE(input);
            colorContext->eff = BinaryModelReadPrimitives::readFloatLE(input);
            colorContexts.set(static_cast<long int>(i), colorContext);
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "RCTX") ) goto fail;
        java::ArrayList<int> readerContextPrevIndex;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&readerContextPrevIndex, readerContextCount, static_cast<int>(-1), "reader context prev index") ) goto fail;
        for ( int i = 0; i < readerContextCount; i++ ) {
            ReaderContext * const readerContext = new ReaderContext();
            BinaryModelReadPrimitives::readBytes(input, reinterpret_cast<unsigned char *>(readerContext->fileName), 96);
            readerContext->fileName[95] = '\0';

            const bool hasInputStream = BinaryModelReadPrimitives::readBool(input);
            readerContext->inputStream = nullptr;
            if ( hasInputStream ) {
                readerContext->inputStream = nullptr;
            }

            readerContext->fileContextId = BinaryModelReadPrimitives::readInt32LE(input);
            BinaryModelReadPrimitives::readBytes(
                input,
                reinterpret_cast<unsigned char *>(readerContext->inputLine),
                ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH);
            readerContext->inputLine[ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
            readerContext->lineNumber = BinaryModelReadPrimitives::readInt32LE(input);
            readerContext->isPipe = static_cast<char>(BinaryModelReadPrimitives::readByte(input));
            readerContextPrevIndex.set(static_cast<long int>(i), BinaryModelReadPrimitives::readInt32LE(input));
            readerContext->prev = nullptr;
            readerContexts.set(static_cast<long int>(i), readerContext);
        }
        for ( int i = 0; i < readerContextCount; i++ ) {
            ReaderContext *prev = nullptr;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(
                     readerContexts,
                     readerContextPrevIndex.get(static_cast<long int>(i)),
                     "readerContext.prev",
                     &prev) ) goto fail;
            readerContexts.get(static_cast<long int>(i))->prev = prev;
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "XFAR") ) goto fail;
        for ( int i = 0; i < transformArrayCount; i++ ) {
            TransformSequenceContext * const transformArray = new TransformSequenceContext();
            transformArray->startingPosition.fileId = BinaryModelReadPrimitives::readInt32LE(input);
            transformArray->startingPosition.lineNumber = BinaryModelReadPrimitives::readInt32LE(input);
            transformArray->startingPosition.offset = static_cast<long>(BinaryModelReadPrimitives::readInt64LE(input));
            transformArray->numberOfDimensions = BinaryModelReadPrimitives::readInt32LE(input);
            for ( int j = 0; j < TransformSequenceContext::TRANSFORM_MAXIMUM_DIMENSIONS; j++ ) {
                transformArray->transformArguments[j].i = BinaryModelReadPrimitives::readInt16LE(input);
                transformArray->transformArguments[j].n = BinaryModelReadPrimitives::readInt16LE(input);
                BinaryModelReadPrimitives::readBytes(
                    input,
                    reinterpret_cast<unsigned char *>(transformArray->transformArguments[j].arg),
                    8);
                transformArray->transformArguments[j].arg[7] = '\0';
            }
            transformArrays.set(static_cast<long int>(i), transformArray);
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "XFCT") ) goto fail;
        java::ArrayList<int> transformContextArrayIndex;
        java::ArrayList<int> transformContextPrevIndex;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&transformContextArrayIndex, transformContextCount, static_cast<int>(-1), "transform context array index") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&transformContextPrevIndex, transformContextCount, static_cast<int>(-1), "transform context prev index") ) goto fail;
        for ( int i = 0; i < transformContextCount; i++ ) {
            TransformStackContext *transformContext = new TransformStackContext();
            transformContext->xid = static_cast<long>(BinaryModelReadPrimitives::readInt64LE(input));
            transformContext->xac = BinaryModelReadPrimitives::readInt16LE(input);
            transformContext->rev = BinaryModelReadPrimitives::readInt16LE(input);

            for ( int row = 0; row < 4; row++ ) {
                for ( int col = 0; col < 4; col++ ) {
                    transformContext->xf.transformMatrix.m[row][col] = BinaryModelReadPrimitives::readDoubleLE(input);
                }
            }
            transformContext->xf.scaleFactor = BinaryModelReadPrimitives::readDoubleLE(input);
            transformContextArrayIndex.set(static_cast<long int>(i), BinaryModelReadPrimitives::readInt32LE(input));
            transformContextPrevIndex.set(static_cast<long int>(i), BinaryModelReadPrimitives::readInt32LE(input));
            transformContext->transformationArray = nullptr;
            transformContext->prev = nullptr;
            transformContexts.set(static_cast<long int>(i), transformContext);
        }
        for ( int i = 0; i < transformContextCount; i++ ) {
            TransformStackContext *transformContext = transformContexts.get(static_cast<long int>(i));
            TransformSequenceContext *transformArray = nullptr;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(
                     transformArrays,
                     transformContextArrayIndex.get(static_cast<long int>(i)),
                     "transformContext.transformationArray",
                     &transformArray) ) goto fail;
            transformContext->transformationArray = transformArray;

            TransformStackContext *previous = nullptr;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(
                     transformContexts,
                     transformContextPrevIndex.get(static_cast<long int>(i)),
                     "transformContext.prev",
                     &previous) ) goto fail;
            transformContext->prev = previous;
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "VRTX") ) goto fail;
        for ( int i = 0; i < vertexCount; i++ ) {
            BinaryModelVertexRecordData &record = vertexRecords[static_cast<long int>(i)];
            record.id = BinaryModelReadPrimitives::readInt32LE(input);
            record.pointIndex = BinaryModelReadPrimitives::readInt32LE(input);
            record.normalIndex = BinaryModelReadPrimitives::readInt32LE(input);
            record.textureCoordinateIndex = BinaryModelReadPrimitives::readInt32LE(input);
            if ( !BinaryModelReadPrimitives::readColor(input, &record.color) ) goto fail;
            record.backIndex = BinaryModelReadPrimitives::readInt32LE(input);
            record.tmp = BinaryModelReadPrimitives::readInt32LE(input);
            record.hasRadianceData = BinaryModelReadPrimitives::readBool(input);
            if ( record.hasRadianceData ) {
                Logger::error("BinaryModelDeserializer::read", "%s", "Vertex radianceData is not supported in binary reader");
                goto fail;
            }
            if ( !BinaryModelReadPrimitives::readIndexList(input, "vertex.patches", &record.patchIndices) ) goto fail;

            Vector3D *point = nullptr;
            Vector3D *normal = nullptr;
            Vector3D *texCoords = nullptr;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vectors, record.pointIndex, "vertex.point", &point) ) goto fail;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vectors, record.normalIndex, "vertex.normal", &normal) ) goto fail;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vectors, record.textureCoordinateIndex, "vertex.textureCoordinates", &texCoords) ) goto fail;

            Vertex *vertex = new Vertex(point, normal, texCoords, new java::ArrayList<Patch *>());
            vertex->id = record.id;
            vertex->color = record.color;
            vertex->tmp = record.tmp;
            vertex->radianceData = nullptr;
            vertices.set(static_cast<long int>(i), vertex);
        }

        for ( int i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices.get(static_cast<long int>(i));
            const BinaryModelVertexRecordData &record = vertexRecords[static_cast<long int>(i)];
            Vertex *back = nullptr;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vertices, record.backIndex, "vertex.back", &back) ) goto fail;
            vertex->back = back;
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "PTCH") ) goto fail;
        for ( int i = 0; i < patchCount; i++ ) {
            BinaryModelPatchRecordData &record = patchRecords[static_cast<long int>(i)];
            record.id = BinaryModelReadPrimitives::readInt32LE(input);
            record.twinIndex = BinaryModelReadPrimitives::readInt32LE(input);
            record.numberOfVertices = BinaryModelReadPrimitives::readInt32LE(input);
            if ( record.numberOfVertices != 3 && record.numberOfVertices != 4 ) {
                Logger::error("BinaryModelDeserializer::read", "%s", "Invalid patch vertex count while loading binary model");
                goto fail;
            }
            for ( int j = 0; j < MAXIMUM_VERTICES_PER_PATCH; j++ ) {
                record.vertexIndices[j] = BinaryModelReadPrimitives::readInt32LE(input);
            }

            record.hasBoundingBox = BinaryModelReadPrimitives::readBool(input);
            if ( record.hasBoundingBox ) {
                if ( !BinaryModelReadPrimitives::readBoundingBoxCoordinates(input, record.boundingBoxCoordinates) ) goto fail;
            }

            if ( !BinaryModelReadPrimitives::readVector(input, &record.normal) ) goto fail;
            record.planeConstant = BinaryModelReadPrimitives::readFloatLE(input);
            record.tolerance = BinaryModelReadPrimitives::readFloatLE(input);
            record.area = BinaryModelReadPrimitives::readFloatLE(input);
            if ( !BinaryModelReadPrimitives::readVector(input, &record.midPoint) ) goto fail;

            record.hasJacobian = BinaryModelReadPrimitives::readBool(input);
            record.jacobianA = 0.0F;
            record.jacobianB = 0.0F;
            record.jacobianC = 0.0F;
            if ( record.hasJacobian ) {
                record.jacobianA = BinaryModelReadPrimitives::readFloatLE(input);
                record.jacobianB = BinaryModelReadPrimitives::readFloatLE(input);
                record.jacobianC = BinaryModelReadPrimitives::readFloatLE(input);
            }

            record.directPotential = BinaryModelReadPrimitives::readFloatLE(input);
            record.dominantIndex = BinaryModelReadPrimitives::readInt32LE(input);
            record.omit = BinaryModelReadPrimitives::readBool(input);
            record.flags = BinaryModelReadPrimitives::readByte(input);
            if ( !BinaryModelReadPrimitives::readColor(input, &record.color) ) goto fail;
            record.materialIndex = BinaryModelReadPrimitives::readInt32LE(input);
            record.hasRadianceData = BinaryModelReadPrimitives::readBool(input);
            if ( record.hasRadianceData ) {
                Logger::error("BinaryModelDeserializer::read", "%s", "Patch radianceData is not supported in binary reader");
                goto fail;
            }

            Vertex *v1 = nullptr;
            Vertex *v2 = nullptr;
            Vertex *v3 = nullptr;
            Vertex *v4 = nullptr;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vertices, record.vertexIndices[0], "patch.vertex[0]", &v1) ) goto fail;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vertices, record.vertexIndices[1], "patch.vertex[1]", &v2) ) goto fail;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vertices, record.vertexIndices[2], "patch.vertex[2]", &v3) ) goto fail;
            if ( record.numberOfVertices == 4 ) {
                if ( !BinaryModelReadPrimitives::pointerFromIndex(vertices, record.vertexIndices[3], "patch.vertex[3]", &v4) ) goto fail;
            }

            Patch *patch = new Patch(record.numberOfVertices, v1, v2, v3, v4);
            patch->setId(static_cast<unsigned>(record.id));
            // Plane constant is immutable after construction and recomputed from geometry.
            // Plane tolerance is immutable after construction and recomputed from geometry.
            // Area is immutable after construction and recomputed from geometry.
            patch->setDirectPotential(record.directPotential);
            patch->setDominantAxisIndex(static_cast<char>(record.dominantIndex));
            patch->setOmit(record.omit);
            patch->setFlags(record.flags);
            patch->setColor(record.color);
            Material *material = nullptr;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(materials, record.materialIndex, "patch.material", &material) ) goto fail;
            patch->setMaterial(material);
            patch->setRadianceData(nullptr);

            patch->setJacobian(nullptr);
            if ( record.hasJacobian ) {
                patch->setJacobian(new Jacobian(record.jacobianA, record.jacobianB, record.jacobianC));
            }

            // Patch bounding box is immutable after construction.
            // The constructor computes it from vertices.
            // We keep reading serialized bounding-box payload for format compatibility.

            patches.set(static_cast<long int>(i), patch);
        }

        for ( int i = 0; i < patchCount; i++ ) {
            Patch *patch = patches.get(static_cast<long int>(i));
            const BinaryModelPatchRecordData &record = patchRecords[static_cast<long int>(i)];
            Patch *twin = nullptr;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(patches, record.twinIndex, "patch.twin", &twin) ) goto fail;
            patch->setTwin(twin);
        }

        for ( int i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices.get(static_cast<long int>(i));
            delete vertex->patches;
            java::ArrayList<Patch *> *patchList = nullptr;
            if ( !BinaryModelReadPrimitives::arrayListFromIndices(
                     vertexRecords[static_cast<long int>(i)].patchIndices,
                     patches,
                     "vertex.patches",
                     &patchList) ) goto fail;
            vertex->patches = patchList;
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "GEOM") ) goto fail;
        for ( int i = 0; i < geometryCount; i++ ) {
            BinaryModelGeometryRecordData &record = geometryRecords[static_cast<long int>(i)];
            record.classId = BinaryModelReadPrimitives::readInt32LE(input);
            record.id = BinaryModelReadPrimitives::readInt32LE(input);
            record.itemCount = BinaryModelReadPrimitives::readInt32LE(input);
            record.bounded = BinaryModelReadPrimitives::readBool(input);
            record.shaftCullGeometry = BinaryModelReadPrimitives::readBool(input);
            record.omit = BinaryModelReadPrimitives::readBool(input);
            record.isDuplicate = BinaryModelReadPrimitives::readBool(input);
            if ( !BinaryModelReadPrimitives::readBoundingBoxCoordinates(input, record.boundingBoxCoordinates) ) goto fail;
            record.hasRayIntersectionBox = BinaryModelReadPrimitives::readBool(input);
            record.hasRadianceData = BinaryModelReadPrimitives::readBool(input);
            if ( record.hasRadianceData ) {
                Logger::error("BinaryModelDeserializer::read", "%s", "Geometry radianceData is not supported in binary reader");
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
                if ( !BinaryModelReadPrimitives::readNullableString(input, &record.objectName, &record.hasObjectName) ) goto fail;
                record.meshId = BinaryModelReadPrimitives::readInt32LE(input);
                record.materialIndex = BinaryModelReadPrimitives::readInt32LE(input);
                if ( !BinaryModelReadPrimitives::readIndexList(input, "surface.positions", &record.positions) ) goto fail;
                if ( !BinaryModelReadPrimitives::readIndexList(input, "surface.normals", &record.normals) ) goto fail;
                if ( !BinaryModelReadPrimitives::readIndexList(input, "surface.vertices", &record.vertices) ) goto fail;
                if ( !BinaryModelReadPrimitives::readIndexList(input, "surface.faces", &record.faces) ) goto fail;
            } else if ( record.classId == static_cast<int>(GeometryClassId::COMPOUND) ) {
                if ( !BinaryModelReadPrimitives::readIndexList(input, "compound.children", &record.children) ) goto fail;
            } else if ( record.classId == static_cast<int>(GeometryClassId::PATCH_SET) ) {
                if ( !BinaryModelReadPrimitives::readIndexList(input, "patchSet.patchList", &record.patchSetPatches) ) goto fail;
            } else {
                Logger::error("BinaryModelDeserializer::read", "%s", "Unsupported geometry type in binary model");
                goto fail;
            }
        }

        for ( int i = 0; i < geometryCount; i++ ) {
            const BinaryModelGeometryRecordData &record = geometryRecords[static_cast<long int>(i)];
            Geometry *geometry = nullptr;

            if ( record.classId == static_cast<int>(GeometryClassId::SURFACE_MESH) ) {
                char *objectName = nullptr;
                if ( !BinaryModelReadPrimitives::duplicateNullableString(record.hasObjectName, record.objectName, &objectName) ) goto fail;

                java::ArrayList<Vector3D *> *positions = nullptr;
                java::ArrayList<Vector3D *> *normals = nullptr;
                java::ArrayList<Vertex *> *surfaceVertices = nullptr;
                java::ArrayList<Patch *> *faces = nullptr;
                Material *material = nullptr;
                if ( !BinaryModelReadPrimitives::arrayListFromIndices(record.positions, vectors, "surface.positions", &positions) ) goto fail;
                if ( !BinaryModelReadPrimitives::arrayListFromIndices(record.normals, vectors, "surface.normals", &normals) ) goto fail;
                if ( !BinaryModelReadPrimitives::arrayListFromIndices(record.vertices, vertices, "surface.vertices", &surfaceVertices) ) goto fail;
                if ( !BinaryModelReadPrimitives::arrayListFromIndices(record.faces, patches, "surface.faces", &faces) ) goto fail;
                if ( !BinaryModelReadPrimitives::pointerFromIndex(materials, record.materialIndex, "surface.material", &material) ) goto fail;

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
                if ( !BinaryModelReadPrimitives::arrayListFromIndices(record.patchSetPatches, patches, "patchSet.patchList", &patchList) ) goto fail;
                geometry = new PatchSet(patchList);
                delete patchList;
            }

            if ( geometry == nullptr ) {
                Logger::error("BinaryModelDeserializer::read", "%s", "Could not instantiate geometry while loading binary model");
                goto fail;
            }

            geometry->className = static_cast<GeometryClassId>(record.classId);
            geometry->id = record.id;
            geometry->itemCount = record.itemCount;
            geometry->bounded = static_cast<char>(record.bounded ? 1 : 0);
            geometry->shaftCullGeometry = static_cast<char>(record.shaftCullGeometry ? 1 : 0);
            geometry->omit = static_cast<char>(record.omit ? 1 : 0);
            geometry->isDuplicate = record.isDuplicate;
            if ( !BinaryModelReadPrimitives::setBoundingBoxFromCoordinates(&geometry->boundingBox, record.boundingBoxCoordinates) ) goto fail;

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
            const BinaryModelGeometryRecordData &record = geometryRecords[static_cast<long int>(i)];
            if ( record.classId == static_cast<int>(GeometryClassId::COMPOUND) ) {
                Compound *compound = static_cast<Compound *>(geometries.get(static_cast<long int>(i)));
                delete compound->children;
                java::ArrayList<Geometry *> *children = nullptr;
                if ( !BinaryModelReadPrimitives::arrayListFromIndices(record.children, geometries, "compound.children", &children) ) goto fail;
                compound->children = children;
            }
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "MODL") ) goto fail;
        modelRecord.currentColorIndex = BinaryModelReadPrimitives::readInt32LE(input);
        if ( !BinaryModelReadPrimitives::readNullableString(input, &modelRecord.currentMaterialName, &modelRecord.hasCurrentMaterialName) ) goto fail;
        if ( !BinaryModelReadPrimitives::readNullableString(input, &modelRecord.currentObjectName, &modelRecord.hasCurrentObjectName) ) goto fail;
        if ( !BinaryModelReadPrimitives::readNullableString(input, &modelRecord.currentVertexName, &modelRecord.hasCurrentVertexName) ) goto fail;
        modelRecord.geometryStackHeadIndex = BinaryModelReadPrimitives::readInt32LE(input);
        modelRecord.inComplex = BinaryModelReadPrimitives::readBool(input);
        modelRecord.inSurface = BinaryModelReadPrimitives::readBool(input);
        modelRecord.monochrome = BinaryModelReadPrimitives::readBool(input);
        modelRecord.readerContextIndex = BinaryModelReadPrimitives::readInt32LE(input);
        modelRecord.transformContextIndex = BinaryModelReadPrimitives::readInt32LE(input);

        if ( !BinaryModelReadPrimitives::readIndexList(input, "model.currentFaceList", &modelRecord.currentFaceList) ) goto fail;
        if ( !BinaryModelReadPrimitives::readIndexList(input, "model.currentGeometryList", &modelRecord.currentGeometryList) ) goto fail;
        if ( !BinaryModelReadPrimitives::readIndexList(input, "model.currentNormalList", &modelRecord.currentNormalList) ) goto fail;
        if ( !BinaryModelReadPrimitives::readIndexList(input, "model.currentPointList", &modelRecord.currentPointList) ) goto fail;
        if ( !BinaryModelReadPrimitives::readIndexList(input, "model.currentVertexList", &modelRecord.currentVertexList) ) goto fail;
        if ( !BinaryModelReadPrimitives::readIndexList(input, "model.geometries", &modelRecord.geometries) ) goto fail;
        if ( !BinaryModelReadPrimitives::readIndexList(input, "model.materials", &modelRecord.materials) ) goto fail;

        model = new ParseSnapshotContext();
        ColorContext *modelCurrentColor = nullptr;
        ReaderContext *modelReaderContext = nullptr;
        TransformStackContext *modelTransformContext = nullptr;
        if ( !BinaryModelReadPrimitives::pointerFromIndex(colorContexts, modelRecord.currentColorIndex, "model.currentColor", &modelCurrentColor) ) goto fail;
        if ( !BinaryModelReadPrimitives::pointerFromIndex(readerContexts, modelRecord.readerContextIndex, "model.readerContext", &modelReaderContext) ) goto fail;
        if ( !BinaryModelReadPrimitives::pointerFromIndex(transformContexts, modelRecord.transformContextIndex, "model.transformContext", &modelTransformContext) ) goto fail;
        model->currentColor = modelCurrentColor;
        model->geometryStackHeadIndex = modelRecord.geometryStackHeadIndex;
        model->inComplex = modelRecord.inComplex;
        model->inSurface = modelRecord.inSurface;
        model->monochrome = modelRecord.monochrome;
        model->readerContext = modelReaderContext;
        model->transformContext = modelTransformContext;

        if ( !BinaryModelReadPrimitives::populateModelStrings(model, modelRecord) ) goto fail;

        if ( !BinaryModelReadPrimitives::arrayListFromIndices(modelRecord.currentFaceList, patches, "model.currentFaceList", &model->currentFaceList) ) goto fail;
        if ( !BinaryModelReadPrimitives::arrayListFromIndices(modelRecord.currentGeometryList, geometries, "model.currentGeometryList", &model->currentGeometryList) ) goto fail;
        if ( !BinaryModelReadPrimitives::arrayListFromIndices(modelRecord.currentNormalList, vectors, "model.currentNormalList", &model->currentNormalList) ) goto fail;
        if ( !BinaryModelReadPrimitives::arrayListFromIndices(modelRecord.currentPointList, vectors, "model.currentPointList", &model->currentPointList) ) goto fail;
        if ( !BinaryModelReadPrimitives::arrayListFromIndices(modelRecord.currentVertexList, vertices, "model.currentVertexList", &model->currentVertexList) ) goto fail;
        if ( !BinaryModelReadPrimitives::arrayListFromIndices(modelRecord.geometries, geometries, "model.geometries", &model->geometries) ) goto fail;
        if ( !BinaryModelReadPrimitives::arrayListFromIndices(modelRecord.materials, materials, "model.materials", &model->materials) ) goto fail;

        int maxPatchId = 0;
        for ( long int i = 0; i < patches.size(); i++ ) {
            Patch *patch = patches.get(i);
            if ( patch != nullptr && static_cast<int>(patch->getId()) > maxPatchId ) {
                maxPatchId = static_cast<int>(patch->getId());
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
        Logger::error("BinaryModelDeserializer::read", "%s", "Unexpected failure while reading binary model");
        ok = false;
    }

fail:
    input.dispose();
    BinaryModelReadCleanup::releaseVertexRecordIndexLists(vertexRecords);
    BinaryModelReadCleanup::releaseGeometryRecordIndexLists(geometryRecords);
    BinaryModelReadCleanup::releaseModelRecordIndexLists(&modelRecord);

    if ( !ok ) {
        BinaryModelReadCleanup::cleanupPartialModel(
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
