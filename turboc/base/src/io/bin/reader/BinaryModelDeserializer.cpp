#include "java/io/BufferedInputStream.h"
#include "java/io/FileInputStream.h"
#include "java/lang/Integer.h"
#include "java/util/ArrayList.txx"

#include "common/linealAlgebra/Jacobian.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/color/ColorRgb.h"
#include "common/logging/Logger.h"
#include "material/Material.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"
#include "material/PhongEmittanceDistributionFunction.h"
#include "material/Texture.h"
#include "skin/Compound.h"
#include "skin/Geometry.h"
#include "material/MaterialColorFlags.h"
#include "skin/MeshSurface.h"
#include "environment/geometry/elements/Patch.h"
#include "environment/geometry/elements/PatchSet.h"
#include "environment/geometry/elements/Vertex.h"
#include "io/context/ColorContext.h"
#include "io/context/ParseSnapshotContext.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformSequenceContext.h"
#include "io/context/TransformStackContext.h"
#include "io/bin/reader/BinaryModelDeserializer.h"
#include "io/bin/reader/ScopedArrayBuffer.h"
#include "io/bin/reader/BinaryModelVertexRecordData.h"
#include "io/bin/reader/BinaryModelPatchRecordData.h"
#include "io/bin/reader/BinaryModelGeometryRecordData.h"
#include "io/bin/reader/BinaryModelSnapshotRecordData.h"
#include "io/bin/reader/BinaryModelReadPrimitives.h"
#include "io/bin/reader/BinaryModelReadCleanup.h"

ParseSnapshotContext *
BinaryModelDeserializer::read(const char *fileName) {
    if ( fileName == NULL || fileName[0] == '\0' ) {
        return NULL;
    }
    File file(fileName);
    if ( !(file.exists() && file.canRead() && file.isFile()) ) {
        return NULL;
    }

    FileInputStream fileInput(fileName);
    BufferedInputStream input(&fileInput);

    ArrayList<Vector3D *> vectors;
    ArrayList<Vertex *> vertices;
    ArrayList<Patch *> patches;
    ArrayList<Material *> materials;
    ArrayList<Geometry *> geometries;
    ArrayList<ColorContext *> colorContexts;
    ArrayList<ReaderContext *> readerContexts;
    ArrayList<TransformSequenceContext *> transformArrays;
    ArrayList<TransformStackContext *> transformContexts;
    ArrayList<BinaryModelVertexRecordData> vertexRecords;
    ArrayList<BinaryModelPatchRecordData> patchRecords;
    ArrayList<BinaryModelGeometryRecordData> geometryRecords;
    BinaryModelSnapshotRecordData modelRecord;
    ParseSnapshotContext *model = NULL;
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
    ArrayList<int> readerContextPrevIndex;
    ArrayList<int> transformContextArrayIndex;
    ArrayList<int> transformContextPrevIndex;
    ColorContext *modelCurrentColor = NULL;
    ReaderContext *modelReaderContext = NULL;
    TransformStackContext *modelTransformContext = NULL;
    int maxPatchId = 0;
    int maxGeometryId = -1;

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

        if ( !BinaryModelReadPrimitives::initializeArrayList(&vectors, vectorCount, ((Vector3D *)(NULL)), "vectors") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&vertices, vertexCount, ((Vertex *)(NULL)), "vertices") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&patches, patchCount, ((Patch *)(NULL)), "patches") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&materials, materialCount, ((Material *)(NULL)), "materials") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&geometries, geometryCount, ((Geometry *)(NULL)), "geometries") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&colorContexts, colorContextCount, ((ColorContext *)(NULL)), "color contexts") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&readerContexts, readerContextCount, ((ReaderContext *)(NULL)), "reader contexts") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&transformArrays, transformArrayCount, ((TransformSequenceContext *)(NULL)), "transform arrays") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&transformContexts, transformContextCount, ((TransformStackContext *)(NULL)), "transform contexts") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&vertexRecords, vertexCount, BinaryModelVertexRecordData(), "vertex records") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&patchRecords, patchCount, BinaryModelPatchRecordData(), "patch records") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&geometryRecords, geometryCount, BinaryModelGeometryRecordData(), "geometry records") ) goto fail;

        if ( !BinaryModelReadPrimitives::expectTag(input, "VEC3") ) goto fail;
        for ( int i = 0; i < vectorCount; i++ ) {
            Vector3D *vector = new Vector3D();
            if ( !BinaryModelReadPrimitives::readVector(input, vector) ) {
                delete vector;
                goto fail;
            }
            vectors.set(((long int)(i)), vector);
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "MTLS") ) goto fail;
        for ( int i = 0; i < materialCount; i++ ) {
            ScopedArrayBuffer<char> materialNameGuard;
            char *materialName = NULL;
            bool hasMaterialName = false;
            if ( !BinaryModelReadPrimitives::readNullableString(input, &materialName, &hasMaterialName) ) goto fail;
            materialNameGuard.reset(materialName);
            const bool sided = BinaryModelReadPrimitives::readBool(input);

            PhongEmitDistFunc *edf = NULL;
            const bool hasEdf = BinaryModelReadPrimitives::readBool(input);
            if ( hasEdf ) {
                ColorRgb kd;
                ColorRgb ks;
                if ( !BinaryModelReadPrimitives::readColor(input, &kd) ) goto fail;
                if ( !BinaryModelReadPrimitives::readColor(input, &ks) ) goto fail;
                const float ns = BinaryModelReadPrimitives::readFloatLE(input);
                edf = new PhongEmitDistFunc(&kd, &ks, ns);
            }

            PhongBidirScattDistFunc *bsdf = NULL;
            const bool hasBsdf = BinaryModelReadPrimitives::readBool(input);
            if ( hasBsdf ) {
                PhongBidirReflDistFunc *brdf = NULL;
                PhongBidirTransDistFunc *btdf = NULL;
                Texture *texture = NULL;

                const bool hasBrdf = BinaryModelReadPrimitives::readBool(input);
                if ( hasBrdf ) {
                    ColorRgb kd;
                    ColorRgb ks;
                    if ( !BinaryModelReadPrimitives::readColor(input, &kd) ) goto fail;
                    if ( !BinaryModelReadPrimitives::readColor(input, &ks) ) goto fail;
                    const float ns = BinaryModelReadPrimitives::readFloatLE(input);
                    brdf = new PhongBidirReflDistFunc(&kd, &ks, ns);
                }

                const bool hasBtdf = BinaryModelReadPrimitives::readBool(input);
                if ( hasBtdf ) {
                    ColorRgb kd;
                    ColorRgb ks;
                    if ( !BinaryModelReadPrimitives::readColor(input, &kd) ) goto fail;
                    if ( !BinaryModelReadPrimitives::readColor(input, &ks) ) goto fail;
                    const float ns = BinaryModelReadPrimitives::readFloatLE(input);
                    const float nr = BinaryModelReadPrimitives::readFloatLE(input);
                    const float ni = BinaryModelReadPrimitives::readFloatLE(input);
                    btdf = new PhongBidirTransDistFunc(&kd, &ks, ns, nr, ni);
                }

                const bool hasTexture = BinaryModelReadPrimitives::readBool(input);
                if ( hasTexture ) {
                    const int width = BinaryModelReadPrimitives::readInt32LE(input);
                    const int height = BinaryModelReadPrimitives::readInt32LE(input);
                    const int channels = BinaryModelReadPrimitives::readInt32LE(input);
                    const long dataBytes = BinaryModelReadPrimitives::readInt64LE(input);

                    if ( width < 0 || height < 0 || channels < 0 || dataBytes < 0 ) {
                        Logger::error("BinaryModelDeserializer::read", "%s", "Invalid texture dimensions in binary material");
                        goto fail;
                    }

                    const long expectedBytes = ((long)(width))
                                                  * ((long)(height))
                                                  * ((long)(channels));
                    if ( expectedBytes != dataBytes ) {
                        Logger::error("BinaryModelDeserializer::read", "%s", "Texture byte count mismatch in binary material");
                        goto fail;
                    }

                    ScopedArrayBuffer<unsigned char> textureData;
                    if ( dataBytes > 0 ) {
                        if ( dataBytes > ((long)(Integer::MAX_VALUE)) ) {
                            Logger::error("BinaryModelDeserializer::read", "%s", "Texture data too large for current platform");
                            goto fail;
                        }
                        textureData.reset(new unsigned char[((int)(dataBytes))]);
                        if ( !BinaryModelReadPrimitives::readBytesChunked(input, textureData.get(), dataBytes) ) goto fail;
                    }
                    texture = new Texture(
                        width,
                        height,
                        channels,
                        textureData.get());
                }

                bsdf = new PhongBidirScattDistFunc(brdf, btdf, texture);
            }

            const char *materialNameCstr = hasMaterialName ? materialNameGuard.get() : "";
            materials.set(((long int)(i)), new Material(materialNameCstr, edf, bsdf, sided));
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "COLR") ) goto fail;
        for ( int i = 0; i < colorContextCount; i++ ) {
            ColorContext *colorContext = new ColorContext();
            colorContext->clock = BinaryModelReadPrimitives::readInt32LE(input);
            colorContext->flags = BinaryModelReadPrimitives::readInt16LE(input);
            for ( int j = 0; j < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; j++ ) {
                colorContext->straightSamples[j] = BinaryModelReadPrimitives::readInt16LE(input);
            }
            colorContext->spectralStraightSum = ((long)(BinaryModelReadPrimitives::readInt64LE(input)));
            colorContext->cx = BinaryModelReadPrimitives::readFloatLE(input);
            colorContext->cy = BinaryModelReadPrimitives::readFloatLE(input);
            colorContext->eff = BinaryModelReadPrimitives::readFloatLE(input);
            colorContexts.set(((long int)(i)), colorContext);
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "RCTX") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&readerContextPrevIndex, readerContextCount, ((int)(-1)), "reader context prev index") ) goto fail;
        for ( int i = 0; i < readerContextCount; i++ ) {
            ReaderContext *readerContext = new ReaderContext();
            BinaryModelReadPrimitives::readBytes(input, ((unsigned char *)(readerContext->fileName)), 96);
            readerContext->fileName[95] = '\0';

            const bool hasInputStream = BinaryModelReadPrimitives::readBool(input);
            readerContext->inputStream = NULL;
            if ( hasInputStream ) {
                readerContext->inputStream = NULL;
            }

            readerContext->fileContextId = BinaryModelReadPrimitives::readInt32LE(input);
            BinaryModelReadPrimitives::readBytes(
                input,
                ((unsigned char *)(readerContext->inputLine)),
                ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH);
            readerContext->inputLine[ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
            readerContext->lineNumber = BinaryModelReadPrimitives::readInt32LE(input);
            readerContext->isPipe = ((char)(BinaryModelReadPrimitives::readByte(input)));
            readerContextPrevIndex.set(((long int)(i)), BinaryModelReadPrimitives::readInt32LE(input));
            readerContext->prev = NULL;
            readerContexts.set(((long int)(i)), readerContext);
        }
        for ( int i = 0; i < readerContextCount; i++ ) {
            ReaderContext *prev = NULL;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(
                     readerContexts,
                     readerContextPrevIndex.get(((long int)(i))),
                     "readerContext.prev",
                     &prev) ) goto fail;
            readerContexts.get(((long int)(i)))->prev = prev;
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "XFAR") ) goto fail;
        for ( int i = 0; i < transformArrayCount; i++ ) {
            TransformSequenceContext *transformArray = new TransformSequenceContext();
            transformArray->startingPosition.fileId = BinaryModelReadPrimitives::readInt32LE(input);
            transformArray->startingPosition.lineNumber = BinaryModelReadPrimitives::readInt32LE(input);
            transformArray->startingPosition.offset = ((long)(BinaryModelReadPrimitives::readInt64LE(input)));
            transformArray->numberOfDimensions = BinaryModelReadPrimitives::readInt32LE(input);
            for ( int j = 0; j < TransformSequenceContext::TRANSFORM_MAXIMUM_DIMENSIONS; j++ ) {
                transformArray->transformArguments[j].i = BinaryModelReadPrimitives::readInt16LE(input);
                transformArray->transformArguments[j].n = BinaryModelReadPrimitives::readInt16LE(input);
                BinaryModelReadPrimitives::readBytes(
                    input,
                    ((unsigned char *)(transformArray->transformArguments[j].arg)),
                    8);
                transformArray->transformArguments[j].arg[7] = '\0';
            }
            transformArrays.set(((long int)(i)), transformArray);
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "XFCT") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&transformContextArrayIndex, transformContextCount, ((int)(-1)), "transform context array index") ) goto fail;
        if ( !BinaryModelReadPrimitives::initializeArrayList(&transformContextPrevIndex, transformContextCount, ((int)(-1)), "transform context prev index") ) goto fail;
        for ( int i = 0; i < transformContextCount; i++ ) {
            TransformStackContext *transformContext = new TransformStackContext();
            transformContext->xid = ((long)(BinaryModelReadPrimitives::readInt64LE(input)));
            transformContext->xac = BinaryModelReadPrimitives::readInt16LE(input);
            transformContext->rev = BinaryModelReadPrimitives::readInt16LE(input);

            for ( int row = 0; row < 4; row++ ) {
                for ( int col = 0; col < 4; col++ ) {
                    transformContext->xf.transformMatrix.m[row][col] = BinaryModelReadPrimitives::readDoubleLE(input);
                }
            }
            transformContext->xf.scaleFactor = BinaryModelReadPrimitives::readDoubleLE(input);
            transformContextArrayIndex.set(((long int)(i)), BinaryModelReadPrimitives::readInt32LE(input));
            transformContextPrevIndex.set(((long int)(i)), BinaryModelReadPrimitives::readInt32LE(input));
            transformContext->transformationArray = NULL;
            transformContext->prev = NULL;
            transformContexts.set(((long int)(i)), transformContext);
        }
        for ( int i = 0; i < transformContextCount; i++ ) {
            TransformStackContext *transformContext = transformContexts.get(((long int)(i)));
            TransformSequenceContext *transformArray = NULL;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(
                     transformArrays,
                     transformContextArrayIndex.get(((long int)(i))),
                     "transformContext.transformationArray",
                     &transformArray) ) goto fail;
            transformContext->transformationArray = transformArray;

            TransformStackContext *previous = NULL;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(
                     transformContexts,
                     transformContextPrevIndex.get(((long int)(i))),
                     "transformContext.prev",
                     &previous) ) goto fail;
            transformContext->prev = previous;
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "VRTX") ) goto fail;
        for ( int i = 0; i < vertexCount; i++ ) {
            BinaryModelVertexRecordData &record = vertexRecords[((long int)(i))];
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

            Vector3D *point = NULL;
            Vector3D *normal = NULL;
            Vector3D *texCoords = NULL;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vectors, record.pointIndex, "vertex.point", &point) ) goto fail;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vectors, record.normalIndex, "vertex.normal", &normal) ) goto fail;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vectors, record.textureCoordinateIndex, "vertex.textureCoordinates", &texCoords) ) goto fail;

            Vertex *vertex = new Vertex(point, normal, texCoords, new ArrayList<Patch *>());
            vertex->id = record.id;
            vertex->color = record.color;
            vertex->tmp = record.tmp;
            vertex->radianceData = NULL;
            vertices.set(((long int)(i)), vertex);
        }

        for ( int i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices.get(((long int)(i)));
            const BinaryModelVertexRecordData &record = vertexRecords[((long int)(i))];
            Vertex *back = NULL;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vertices, record.backIndex, "vertex.back", &back) ) goto fail;
            vertex->back = back;
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "PTCH") ) goto fail;
        for ( int i = 0; i < patchCount; i++ ) {
            BinaryModelPatchRecordData &record = patchRecords[((long int)(i))];
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
            record.jacobianA = 0.0f;
            record.jacobianB = 0.0f;
            record.jacobianC = 0.0f;
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

            Vertex *v1 = NULL;
            Vertex *v2 = NULL;
            Vertex *v3 = NULL;
            Vertex *v4 = NULL;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vertices, record.vertexIndices[0], "patch.vertex[0]", &v1) ) goto fail;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vertices, record.vertexIndices[1], "patch.vertex[1]", &v2) ) goto fail;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(vertices, record.vertexIndices[2], "patch.vertex[2]", &v3) ) goto fail;
            if ( record.numberOfVertices == 4 ) {
                if ( !BinaryModelReadPrimitives::pointerFromIndex(vertices, record.vertexIndices[3], "patch.vertex[3]", &v4) ) goto fail;
            }

            Patch *patch = new Patch(record.numberOfVertices, v1, v2, v3, v4);
            patch->id = ((unsigned)(record.id));
            patch->normal = record.normal;
            patch->planeConstant = record.planeConstant;
            patch->tolerance = record.tolerance;
            patch->area = record.area;
            patch->midPoint = record.midPoint;
            patch->directPotential = record.directPotential;
            patch->index = ((char)(record.dominantIndex));
            patch->omit = ((char)(record.omit ? 1 : 0));
            patch->setFlags(record.flags);
            patch->color = record.color;
            Material *material = NULL;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(materials, record.materialIndex, "patch.material", &material) ) goto fail;
            patch->material = material;
            patch->radianceData = NULL;

            if ( patch->jacobian != NULL ) {
                delete patch->jacobian;
                patch->jacobian = NULL;
            }
            if ( record.hasJacobian ) {
                patch->jacobian = new Jacobian(record.jacobianA, record.jacobianB, record.jacobianC);
            }

            if ( patch->boundingBox != NULL ) {
                delete patch->boundingBox;
                patch->boundingBox = NULL;
            }
            if ( record.hasBoundingBox ) {
                patch->boundingBox = new BoundingBox();
                if ( !BinaryModelReadPrimitives::setBoundingBoxFromCoordinates(patch->boundingBox, record.boundingBoxCoordinates) ) goto fail;
            }

            patches.set(((long int)(i)), patch);
        }

        for ( int i = 0; i < patchCount; i++ ) {
            Patch *patch = patches.get(((long int)(i)));
            const BinaryModelPatchRecordData &record = patchRecords[((long int)(i))];
            Patch *twin = NULL;
            if ( !BinaryModelReadPrimitives::pointerFromIndex(patches, record.twinIndex, "patch.twin", &twin) ) goto fail;
            patch->twin = twin;
        }

        for ( int i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices.get(((long int)(i)));
            delete vertex->patches;
            ArrayList<Patch *> *patchList = NULL;
            if ( !BinaryModelReadPrimitives::arrayListFromIndices(
                     vertexRecords[((long int)(i))].patchIndices,
                     patches,
                     "vertex.patches",
                     &patchList) ) goto fail;
            vertex->patches = patchList;
        }

        if ( !BinaryModelReadPrimitives::expectTag(input, "GEOM") ) goto fail;
        for ( int i = 0; i < geometryCount; i++ ) {
            BinaryModelGeometryRecordData &record = geometryRecords[((long int)(i))];
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
            if ( record.objectName != NULL ) {
                delete[] record.objectName;
                record.objectName = NULL;
            }
            record.meshId = 0;
            record.materialIndex = -1;

            if ( record.classId == ((int)(SURFACE_MESH)) ) {
                if ( !BinaryModelReadPrimitives::readNullableString(input, &record.objectName, &record.hasObjectName) ) goto fail;
                record.meshId = BinaryModelReadPrimitives::readInt32LE(input);
                record.materialIndex = BinaryModelReadPrimitives::readInt32LE(input);
                if ( !BinaryModelReadPrimitives::readIndexList(input, "surface.positions", &record.positions) ) goto fail;
                if ( !BinaryModelReadPrimitives::readIndexList(input, "surface.normals", &record.normals) ) goto fail;
                if ( !BinaryModelReadPrimitives::readIndexList(input, "surface.vertices", &record.vertices) ) goto fail;
                if ( !BinaryModelReadPrimitives::readIndexList(input, "surface.faces", &record.faces) ) goto fail;
            } else if ( record.classId == ((int)(COMPOUND)) ) {
                if ( !BinaryModelReadPrimitives::readIndexList(input, "compound.children", &record.children) ) goto fail;
            } else if ( record.classId == ((int)(PATCH_SET)) ) {
                if ( !BinaryModelReadPrimitives::readIndexList(input, "patchSet.patchList", &record.patchSetPatches) ) goto fail;
            } else {
                Logger::error("BinaryModelDeserializer::read", "%s", "Unsupported geometry type in binary model");
                goto fail;
            }
        }

        for ( int i = 0; i < geometryCount; i++ ) {
            const BinaryModelGeometryRecordData &record = geometryRecords[((long int)(i))];
            Geometry *geometry = NULL;

            if ( record.classId == ((int)(SURFACE_MESH)) ) {
                char *objectName = NULL;
                if ( !BinaryModelReadPrimitives::duplicateNullableString(record.hasObjectName, record.objectName, &objectName) ) goto fail;

                ArrayList<Vector3D *> *positions = NULL;
                ArrayList<Vector3D *> *normals = NULL;
                ArrayList<Vertex *> *surfaceVertices = NULL;
                ArrayList<Patch *> *faces = NULL;
                Material *material = NULL;
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
                    NULL,
                    surfaceVertices,
                    faces,
                    NO_COLORS);
                surface->meshId = record.meshId;
                geometry = surface;
            } else if ( record.classId == ((int)(COMPOUND)) ) {
                geometry = new Compound(new ArrayList<Geometry *>());
            } else if ( record.classId == ((int)(PATCH_SET)) ) {
                ArrayList<Patch *> *patchList = NULL;
                if ( !BinaryModelReadPrimitives::arrayListFromIndices(record.patchSetPatches, patches, "patchSet.patchList", &patchList) ) goto fail;
                geometry = new PatchSet(patchList);
                delete patchList;
            }

            if ( geometry == NULL ) {
                Logger::error("BinaryModelDeserializer::read", "%s", "Could not instantiate geometry while loading binary model");
                goto fail;
            }

            geometry->className = ((GeometryClassId)(record.classId));
            geometry->id = record.id;
            geometry->itemCount = record.itemCount;
            geometry->bounded = ((char)(record.bounded ? 1 : 0));
            geometry->shaftCullGeometry = ((char)(record.shaftCullGeometry ? 1 : 0));
            geometry->omit = ((char)(record.omit ? 1 : 0));
            geometry->isDuplicate = record.isDuplicate;
            if ( !BinaryModelReadPrimitives::setBoundingBoxFromCoordinates(&geometry->boundingBox, record.boundingBoxCoordinates) ) goto fail;

            (void) record.hasRayIntersectionBox;

            geometry->radianceData = NULL;
            geometries.set(((long int)(i)), geometry);
        }

        for ( int i = 0; i < geometryCount; i++ ) {
            const BinaryModelGeometryRecordData &record = geometryRecords[((long int)(i))];
            if ( record.classId == ((int)(COMPOUND)) ) {
                Compound *compound = ((Compound *)(geometries.get(((long int)(i)))));
                delete compound->children;
                ArrayList<Geometry *> *children = NULL;
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
        modelCurrentColor = NULL;
        modelReaderContext = NULL;
        modelTransformContext = NULL;
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

        maxPatchId = 0;
        for ( long int i = 0; i < patches.size(); i++ ) {
            Patch *patch = patches.get(i);
            if ( patch != NULL && ((int)(patch->id)) > maxPatchId ) {
                maxPatchId = ((int)(patch->id));
            }
        }
        Patch::setNextId(maxPatchId + 1);

        maxGeometryId = -1;
        for ( long int i = 0; i < geometries.size(); i++ ) {
            Geometry *geometry = geometries.get(i);
            if ( geometry != NULL && geometry->id > maxGeometryId ) {
                maxGeometryId = geometry->id;
            }
        }
        Geometry::nextGeometryId = maxGeometryId + 1;

    ok = true;

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
        return NULL;
    }

    return model;
}
