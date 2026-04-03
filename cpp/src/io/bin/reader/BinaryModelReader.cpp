#include "java/io/BufferedInputStream.h"
#include "java/io/FileInputStream.h"
#include "java/lang/Integer.h"
#include "java/util/ArrayList.txx"

#include "common/linealAlgebra/Jacobian.h"
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
#include "skin/MaterialColorFlags.h"
#include "skin/MeshSurface.h"
#include "skin/MinMaxBox.h"
#include "skin/Patch.h"
#include "skin/PatchSet.h"
#include "skin/Vertex.h"
#include "io/context/ColorContext.h"
#include "io/context/ParseSnapshotContext.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformSequenceContext.h"
#include "io/context/TransformStackContext.h"
#include "io/bin/reader/BinaryModelReader.h"
#include "io/bin/reader/ScopedArray.h"
#include "io/bin/reader/BinaryModelReaderVertexRecord.h"
#include "io/bin/reader/BinaryModelReaderPatchRecord.h"
#include "io/bin/reader/BinaryModelReaderGeometryRecord.h"
#include "io/bin/reader/BinaryModelReaderModelRecord.h"
#include "io/bin/reader/BinaryModelReaderSupport.h"
#include "io/bin/reader/BinaryModelReaderCleanup.h"

ParseSnapshotContext *
BinaryModelReader::read(const char *fileName) {
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
    java::ArrayList<BinaryModelReaderVertexRecord> vertexRecords;
    java::ArrayList<BinaryModelReaderPatchRecord> patchRecords;
    java::ArrayList<BinaryModelReaderGeometryRecord> geometryRecords;
    BinaryModelReaderModelRecord modelRecord;
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
        if ( !BinaryModelReaderSupport::validateBinaryHeader(input) ) goto fail;

        if ( !BinaryModelReaderSupport::readNonNegativeCount(input, "vectors", &vectorCount) ) goto fail;
        if ( !BinaryModelReaderSupport::readNonNegativeCount(input, "vertices", &vertexCount) ) goto fail;
        if ( !BinaryModelReaderSupport::readNonNegativeCount(input, "patches", &patchCount) ) goto fail;
        if ( !BinaryModelReaderSupport::readNonNegativeCount(input, "materials", &materialCount) ) goto fail;
        if ( !BinaryModelReaderSupport::readNonNegativeCount(input, "geometries", &geometryCount) ) goto fail;
        if ( !BinaryModelReaderSupport::readNonNegativeCount(input, "color contexts", &colorContextCount) ) goto fail;
        if ( !BinaryModelReaderSupport::readNonNegativeCount(input, "reader contexts", &readerContextCount) ) goto fail;
        if ( !BinaryModelReaderSupport::readNonNegativeCount(input, "transform arrays", &transformArrayCount) ) goto fail;
        if ( !BinaryModelReaderSupport::readNonNegativeCount(input, "transform contexts", &transformContextCount) ) goto fail;

        if ( !BinaryModelReaderSupport::initializeArrayList(&vectors, vectorCount, static_cast<Vector3D *>(nullptr), "vectors") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&vertices, vertexCount, static_cast<Vertex *>(nullptr), "vertices") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&patches, patchCount, static_cast<Patch *>(nullptr), "patches") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&materials, materialCount, static_cast<Material *>(nullptr), "materials") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&geometries, geometryCount, static_cast<Geometry *>(nullptr), "geometries") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&colorContexts, colorContextCount, static_cast<ColorContext *>(nullptr), "color contexts") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&readerContexts, readerContextCount, static_cast<ReaderContext *>(nullptr), "reader contexts") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&transformArrays, transformArrayCount, static_cast<TransformSequenceContext *>(nullptr), "transform arrays") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&transformContexts, transformContextCount, static_cast<TransformStackContext *>(nullptr), "transform contexts") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&vertexRecords, vertexCount, BinaryModelReaderVertexRecord(), "vertex records") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&patchRecords, patchCount, BinaryModelReaderPatchRecord(), "patch records") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&geometryRecords, geometryCount, BinaryModelReaderGeometryRecord(), "geometry records") ) goto fail;

        if ( !BinaryModelReaderSupport::expectTag(input, "VEC3") ) goto fail;
        for ( int i = 0; i < vectorCount; i++ ) {
            Vector3D *vector = new Vector3D();
            if ( !BinaryModelReaderSupport::readVector(input, vector) ) {
                delete vector;
                goto fail;
            }
            vectors.set(static_cast<long int>(i), vector);
        }

        if ( !BinaryModelReaderSupport::expectTag(input, "MTLS") ) goto fail;
        for ( int i = 0; i < materialCount; i++ ) {
            ScopedArray<char> materialNameGuard;
            char *materialName = nullptr;
            bool hasMaterialName = false;
            if ( !BinaryModelReaderSupport::readNullableString(input, &materialName, &hasMaterialName) ) goto fail;
            materialNameGuard.reset(materialName);
            const bool sided = BinaryModelReaderSupport::readBool(input);

            PhongEmittanceDistributionFunction *edf = nullptr;
            const bool hasEdf = BinaryModelReaderSupport::readBool(input);
            if ( hasEdf ) {
                ColorRgb kd;
                ColorRgb ks;
                if ( !BinaryModelReaderSupport::readColor(input, &kd) ) goto fail;
                if ( !BinaryModelReaderSupport::readColor(input, &ks) ) goto fail;
                const float ns = BinaryModelReaderSupport::readFloatLE(input);
                edf = new PhongEmittanceDistributionFunction(&kd, &ks, ns);
            }

            PhongBidirectionalScatteringDistributionFunction *bsdf = nullptr;
            const bool hasBsdf = BinaryModelReaderSupport::readBool(input);
            if ( hasBsdf ) {
                PhongBidirectionalReflectanceDistributionFunction *brdf = nullptr;
                PhongBidirectionalTransmittanceDistributionFunction *btdf = nullptr;
                Texture *texture = nullptr;

                const bool hasBrdf = BinaryModelReaderSupport::readBool(input);
                if ( hasBrdf ) {
                    ColorRgb kd;
                    ColorRgb ks;
                    if ( !BinaryModelReaderSupport::readColor(input, &kd) ) goto fail;
                    if ( !BinaryModelReaderSupport::readColor(input, &ks) ) goto fail;
                    const float ns = BinaryModelReaderSupport::readFloatLE(input);
                    brdf = new PhongBidirectionalReflectanceDistributionFunction(&kd, &ks, ns);
                }

                const bool hasBtdf = BinaryModelReaderSupport::readBool(input);
                if ( hasBtdf ) {
                    ColorRgb kd;
                    ColorRgb ks;
                    if ( !BinaryModelReaderSupport::readColor(input, &kd) ) goto fail;
                    if ( !BinaryModelReaderSupport::readColor(input, &ks) ) goto fail;
                    const float ns = BinaryModelReaderSupport::readFloatLE(input);
                    const float nr = BinaryModelReaderSupport::readFloatLE(input);
                    const float ni = BinaryModelReaderSupport::readFloatLE(input);
                    btdf = new PhongBidirectionalTransmittanceDistributionFunction(&kd, &ks, ns, nr, ni);
                }

                const bool hasTexture = BinaryModelReaderSupport::readBool(input);
                if ( hasTexture ) {
                    const int width = BinaryModelReaderSupport::readInt32LE(input);
                    const int height = BinaryModelReaderSupport::readInt32LE(input);
                    const int channels = BinaryModelReaderSupport::readInt32LE(input);
                    const long long dataBytes = BinaryModelReaderSupport::readInt64LE(input);

                    if ( width < 0 || height < 0 || channels < 0 || dataBytes < 0 ) {
                        Error::error("BinaryModelReader::read", "%s", "Invalid texture dimensions in binary material");
                        goto fail;
                    }

                    const long long expectedBytes = static_cast<long long>(width)
                                                  * static_cast<long long>(height)
                                                  * static_cast<long long>(channels);
                    if ( expectedBytes != dataBytes ) {
                        Error::error("BinaryModelReader::read", "%s", "Texture byte count mismatch in binary material");
                        goto fail;
                    }

                    ScopedArray<unsigned char> textureData;
                    if ( dataBytes > 0 ) {
                        if ( dataBytes > static_cast<long long>(java::Integer::MAX_VALUE) ) {
                            Error::error("BinaryModelReader::read", "%s", "Texture data too large for current platform");
                            goto fail;
                        }
                        textureData.reset(new unsigned char[static_cast<int>(dataBytes)]);
                        if ( !BinaryModelReaderSupport::readBytesChunked(input, textureData.get(), dataBytes) ) goto fail;
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

        if ( !BinaryModelReaderSupport::expectTag(input, "COLR") ) goto fail;
        for ( int i = 0; i < colorContextCount; i++ ) {
            ColorContext *colorContext = new ColorContext();
            colorContext->clock = BinaryModelReaderSupport::readInt32LE(input);
            colorContext->flags = BinaryModelReaderSupport::readInt16LE(input);
            for ( int j = 0; j < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; j++ ) {
                colorContext->straightSamples[j] = BinaryModelReaderSupport::readInt16LE(input);
            }
            colorContext->spectralStraightSum = static_cast<long>(BinaryModelReaderSupport::readInt64LE(input));
            colorContext->cx = BinaryModelReaderSupport::readFloatLE(input);
            colorContext->cy = BinaryModelReaderSupport::readFloatLE(input);
            colorContext->eff = BinaryModelReaderSupport::readFloatLE(input);
            colorContexts.set(static_cast<long int>(i), colorContext);
        }

        if ( !BinaryModelReaderSupport::expectTag(input, "RCTX") ) goto fail;
        java::ArrayList<int> readerContextPrevIndex;
        if ( !BinaryModelReaderSupport::initializeArrayList(&readerContextPrevIndex, readerContextCount, static_cast<int>(-1), "reader context prev index") ) goto fail;
        for ( int i = 0; i < readerContextCount; i++ ) {
            ReaderContext *readerContext = new ReaderContext();
            BinaryModelReaderSupport::readBytes(input, reinterpret_cast<unsigned char *>(readerContext->fileName), 96);
            readerContext->fileName[95] = '\0';

            const bool hasInputStream = BinaryModelReaderSupport::readBool(input);
            readerContext->inputStream = nullptr;
            if ( hasInputStream ) {
                readerContext->inputStream = nullptr;
            }

            readerContext->fileContextId = BinaryModelReaderSupport::readInt32LE(input);
            BinaryModelReaderSupport::readBytes(
                input,
                reinterpret_cast<unsigned char *>(readerContext->inputLine),
                ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH);
            readerContext->inputLine[ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
            readerContext->lineNumber = BinaryModelReaderSupport::readInt32LE(input);
            readerContext->isPipe = static_cast<char>(BinaryModelReaderSupport::readByte(input));
            readerContextPrevIndex.set(static_cast<long int>(i), BinaryModelReaderSupport::readInt32LE(input));
            readerContext->prev = nullptr;
            readerContexts.set(static_cast<long int>(i), readerContext);
        }
        for ( int i = 0; i < readerContextCount; i++ ) {
            ReaderContext *prev = nullptr;
            if ( !BinaryModelReaderSupport::pointerFromIndex(
                     readerContexts,
                     readerContextPrevIndex.get(static_cast<long int>(i)),
                     "readerContext.prev",
                     &prev) ) goto fail;
            readerContexts.get(static_cast<long int>(i))->prev = prev;
        }

        if ( !BinaryModelReaderSupport::expectTag(input, "XFAR") ) goto fail;
        for ( int i = 0; i < transformArrayCount; i++ ) {
            TransformSequenceContext *transformArray = new TransformSequenceContext();
            transformArray->startingPosition.fileId = BinaryModelReaderSupport::readInt32LE(input);
            transformArray->startingPosition.lineNumber = BinaryModelReaderSupport::readInt32LE(input);
            transformArray->startingPosition.offset = static_cast<long>(BinaryModelReaderSupport::readInt64LE(input));
            transformArray->numberOfDimensions = BinaryModelReaderSupport::readInt32LE(input);
            for ( int j = 0; j < TransformSequenceContext::TRANSFORM_MAXIMUM_DIMENSIONS; j++ ) {
                transformArray->transformArguments[j].i = BinaryModelReaderSupport::readInt16LE(input);
                transformArray->transformArguments[j].n = BinaryModelReaderSupport::readInt16LE(input);
                BinaryModelReaderSupport::readBytes(
                    input,
                    reinterpret_cast<unsigned char *>(transformArray->transformArguments[j].arg),
                    8);
                transformArray->transformArguments[j].arg[7] = '\0';
            }
            transformArrays.set(static_cast<long int>(i), transformArray);
        }

        if ( !BinaryModelReaderSupport::expectTag(input, "XFCT") ) goto fail;
        java::ArrayList<int> transformContextArrayIndex;
        java::ArrayList<int> transformContextPrevIndex;
        if ( !BinaryModelReaderSupport::initializeArrayList(&transformContextArrayIndex, transformContextCount, static_cast<int>(-1), "transform context array index") ) goto fail;
        if ( !BinaryModelReaderSupport::initializeArrayList(&transformContextPrevIndex, transformContextCount, static_cast<int>(-1), "transform context prev index") ) goto fail;
        for ( int i = 0; i < transformContextCount; i++ ) {
            TransformStackContext *transformContext = new TransformStackContext();
            transformContext->xid = static_cast<long>(BinaryModelReaderSupport::readInt64LE(input));
            transformContext->xac = BinaryModelReaderSupport::readInt16LE(input);
            transformContext->rev = BinaryModelReaderSupport::readInt16LE(input);

            for ( int row = 0; row < 4; row++ ) {
                for ( int col = 0; col < 4; col++ ) {
                    transformContext->xf.transformMatrix.m[row][col] = BinaryModelReaderSupport::readDoubleLE(input);
                }
            }
            transformContext->xf.scaleFactor = BinaryModelReaderSupport::readDoubleLE(input);
            transformContextArrayIndex.set(static_cast<long int>(i), BinaryModelReaderSupport::readInt32LE(input));
            transformContextPrevIndex.set(static_cast<long int>(i), BinaryModelReaderSupport::readInt32LE(input));
            transformContext->transformationArray = nullptr;
            transformContext->prev = nullptr;
            transformContexts.set(static_cast<long int>(i), transformContext);
        }
        for ( int i = 0; i < transformContextCount; i++ ) {
            TransformStackContext *transformContext = transformContexts.get(static_cast<long int>(i));
            TransformSequenceContext *transformArray = nullptr;
            if ( !BinaryModelReaderSupport::pointerFromIndex(
                     transformArrays,
                     transformContextArrayIndex.get(static_cast<long int>(i)),
                     "transformContext.transformationArray",
                     &transformArray) ) goto fail;
            transformContext->transformationArray = transformArray;

            TransformStackContext *previous = nullptr;
            if ( !BinaryModelReaderSupport::pointerFromIndex(
                     transformContexts,
                     transformContextPrevIndex.get(static_cast<long int>(i)),
                     "transformContext.prev",
                     &previous) ) goto fail;
            transformContext->prev = previous;
        }

        if ( !BinaryModelReaderSupport::expectTag(input, "VRTX") ) goto fail;
        for ( int i = 0; i < vertexCount; i++ ) {
            BinaryModelReaderVertexRecord &record = vertexRecords[static_cast<long int>(i)];
            record.id = BinaryModelReaderSupport::readInt32LE(input);
            record.pointIndex = BinaryModelReaderSupport::readInt32LE(input);
            record.normalIndex = BinaryModelReaderSupport::readInt32LE(input);
            record.textureCoordinateIndex = BinaryModelReaderSupport::readInt32LE(input);
            if ( !BinaryModelReaderSupport::readColor(input, &record.color) ) goto fail;
            record.backIndex = BinaryModelReaderSupport::readInt32LE(input);
            record.tmp = BinaryModelReaderSupport::readInt32LE(input);
            record.hasRadianceData = BinaryModelReaderSupport::readBool(input);
            if ( record.hasRadianceData ) {
                Error::error("BinaryModelReader::read", "%s", "Vertex radianceData is not supported in binary reader");
                goto fail;
            }
            if ( !BinaryModelReaderSupport::readIndexList(input, "vertex.patches", &record.patchIndices) ) goto fail;

            Vector3D *point = nullptr;
            Vector3D *normal = nullptr;
            Vector3D *texCoords = nullptr;
            if ( !BinaryModelReaderSupport::pointerFromIndex(vectors, record.pointIndex, "vertex.point", &point) ) goto fail;
            if ( !BinaryModelReaderSupport::pointerFromIndex(vectors, record.normalIndex, "vertex.normal", &normal) ) goto fail;
            if ( !BinaryModelReaderSupport::pointerFromIndex(vectors, record.textureCoordinateIndex, "vertex.textureCoordinates", &texCoords) ) goto fail;

            Vertex *vertex = new Vertex(point, normal, texCoords, new java::ArrayList<Patch *>());
            vertex->id = record.id;
            vertex->color = record.color;
            vertex->tmp = record.tmp;
            vertex->radianceData = nullptr;
            vertices.set(static_cast<long int>(i), vertex);
        }

        for ( int i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices.get(static_cast<long int>(i));
            const BinaryModelReaderVertexRecord &record = vertexRecords[static_cast<long int>(i)];
            Vertex *back = nullptr;
            if ( !BinaryModelReaderSupport::pointerFromIndex(vertices, record.backIndex, "vertex.back", &back) ) goto fail;
            vertex->back = back;
        }

        if ( !BinaryModelReaderSupport::expectTag(input, "PTCH") ) goto fail;
        for ( int i = 0; i < patchCount; i++ ) {
            BinaryModelReaderPatchRecord &record = patchRecords[static_cast<long int>(i)];
            record.id = BinaryModelReaderSupport::readInt32LE(input);
            record.twinIndex = BinaryModelReaderSupport::readInt32LE(input);
            record.numberOfVertices = BinaryModelReaderSupport::readInt32LE(input);
            if ( record.numberOfVertices != 3 && record.numberOfVertices != 4 ) {
                Error::error("BinaryModelReader::read", "%s", "Invalid patch vertex count while loading binary model");
                goto fail;
            }
            for ( int j = 0; j < MAXIMUM_VERTICES_PER_PATCH; j++ ) {
                record.vertexIndices[j] = BinaryModelReaderSupport::readInt32LE(input);
            }

            record.hasBoundingBox = BinaryModelReaderSupport::readBool(input);
            if ( record.hasBoundingBox ) {
                if ( !BinaryModelReaderSupport::readBoundingBoxCoordinates(input, record.boundingBoxCoordinates) ) goto fail;
            }

            if ( !BinaryModelReaderSupport::readVector(input, &record.normal) ) goto fail;
            record.planeConstant = BinaryModelReaderSupport::readFloatLE(input);
            record.tolerance = BinaryModelReaderSupport::readFloatLE(input);
            record.area = BinaryModelReaderSupport::readFloatLE(input);
            if ( !BinaryModelReaderSupport::readVector(input, &record.midPoint) ) goto fail;

            record.hasJacobian = BinaryModelReaderSupport::readBool(input);
            record.jacobianA = 0.0f;
            record.jacobianB = 0.0f;
            record.jacobianC = 0.0f;
            if ( record.hasJacobian ) {
                record.jacobianA = BinaryModelReaderSupport::readFloatLE(input);
                record.jacobianB = BinaryModelReaderSupport::readFloatLE(input);
                record.jacobianC = BinaryModelReaderSupport::readFloatLE(input);
            }

            record.directPotential = BinaryModelReaderSupport::readFloatLE(input);
            record.dominantIndex = BinaryModelReaderSupport::readInt32LE(input);
            record.omit = BinaryModelReaderSupport::readBool(input);
            record.flags = BinaryModelReaderSupport::readByte(input);
            if ( !BinaryModelReaderSupport::readColor(input, &record.color) ) goto fail;
            record.materialIndex = BinaryModelReaderSupport::readInt32LE(input);
            record.hasRadianceData = BinaryModelReaderSupport::readBool(input);
            if ( record.hasRadianceData ) {
                Error::error("BinaryModelReader::read", "%s", "Patch radianceData is not supported in binary reader");
                goto fail;
            }

            Vertex *v1 = nullptr;
            Vertex *v2 = nullptr;
            Vertex *v3 = nullptr;
            Vertex *v4 = nullptr;
            if ( !BinaryModelReaderSupport::pointerFromIndex(vertices, record.vertexIndices[0], "patch.vertex[0]", &v1) ) goto fail;
            if ( !BinaryModelReaderSupport::pointerFromIndex(vertices, record.vertexIndices[1], "patch.vertex[1]", &v2) ) goto fail;
            if ( !BinaryModelReaderSupport::pointerFromIndex(vertices, record.vertexIndices[2], "patch.vertex[2]", &v3) ) goto fail;
            if ( record.numberOfVertices == 4 ) {
                if ( !BinaryModelReaderSupport::pointerFromIndex(vertices, record.vertexIndices[3], "patch.vertex[3]", &v4) ) goto fail;
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
            if ( !BinaryModelReaderSupport::pointerFromIndex(materials, record.materialIndex, "patch.material", &material) ) goto fail;
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
                if ( !BinaryModelReaderSupport::setBoundingBoxFromCoordinates(patch->boundingBox, record.boundingBoxCoordinates) ) goto fail;
            }

            patches.set(static_cast<long int>(i), patch);
        }

        for ( int i = 0; i < patchCount; i++ ) {
            Patch *patch = patches.get(static_cast<long int>(i));
            const BinaryModelReaderPatchRecord &record = patchRecords[static_cast<long int>(i)];
            Patch *twin = nullptr;
            if ( !BinaryModelReaderSupport::pointerFromIndex(patches, record.twinIndex, "patch.twin", &twin) ) goto fail;
            patch->twin = twin;
        }

        for ( int i = 0; i < vertexCount; i++ ) {
            Vertex *vertex = vertices.get(static_cast<long int>(i));
            delete vertex->patches;
            java::ArrayList<Patch *> *patchList = nullptr;
            if ( !BinaryModelReaderSupport::arrayListFromIndices(
                     vertexRecords[static_cast<long int>(i)].patchIndices,
                     patches,
                     "vertex.patches",
                     &patchList) ) goto fail;
            vertex->patches = patchList;
        }

        if ( !BinaryModelReaderSupport::expectTag(input, "GEOM") ) goto fail;
        for ( int i = 0; i < geometryCount; i++ ) {
            BinaryModelReaderGeometryRecord &record = geometryRecords[static_cast<long int>(i)];
            record.classId = BinaryModelReaderSupport::readInt32LE(input);
            record.id = BinaryModelReaderSupport::readInt32LE(input);
            record.itemCount = BinaryModelReaderSupport::readInt32LE(input);
            record.bounded = BinaryModelReaderSupport::readBool(input);
            record.shaftCullGeometry = BinaryModelReaderSupport::readBool(input);
            record.omit = BinaryModelReaderSupport::readBool(input);
            record.isDuplicate = BinaryModelReaderSupport::readBool(input);
            if ( !BinaryModelReaderSupport::readBoundingBoxCoordinates(input, record.boundingBoxCoordinates) ) goto fail;
            record.hasRayIntersectionBox = BinaryModelReaderSupport::readBool(input);
            record.hasRadianceData = BinaryModelReaderSupport::readBool(input);
            if ( record.hasRadianceData ) {
                Error::error("BinaryModelReader::read", "%s", "Geometry radianceData is not supported in binary reader");
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
                if ( !BinaryModelReaderSupport::readNullableString(input, &record.objectName, &record.hasObjectName) ) goto fail;
                record.meshId = BinaryModelReaderSupport::readInt32LE(input);
                record.materialIndex = BinaryModelReaderSupport::readInt32LE(input);
                if ( !BinaryModelReaderSupport::readIndexList(input, "surface.positions", &record.positions) ) goto fail;
                if ( !BinaryModelReaderSupport::readIndexList(input, "surface.normals", &record.normals) ) goto fail;
                if ( !BinaryModelReaderSupport::readIndexList(input, "surface.vertices", &record.vertices) ) goto fail;
                if ( !BinaryModelReaderSupport::readIndexList(input, "surface.faces", &record.faces) ) goto fail;
            } else if ( record.classId == static_cast<int>(GeometryClassId::COMPOUND) ) {
                if ( !BinaryModelReaderSupport::readIndexList(input, "compound.children", &record.children) ) goto fail;
            } else if ( record.classId == static_cast<int>(GeometryClassId::PATCH_SET) ) {
                if ( !BinaryModelReaderSupport::readIndexList(input, "patchSet.patchList", &record.patchSetPatches) ) goto fail;
            } else {
                Error::error("BinaryModelReader::read", "%s", "Unsupported geometry type in binary model");
                goto fail;
            }
        }

        for ( int i = 0; i < geometryCount; i++ ) {
            const BinaryModelReaderGeometryRecord &record = geometryRecords[static_cast<long int>(i)];
            Geometry *geometry = nullptr;

            if ( record.classId == static_cast<int>(GeometryClassId::SURFACE_MESH) ) {
                char *objectName = nullptr;
                if ( !BinaryModelReaderSupport::duplicateNullableString(record.hasObjectName, record.objectName, &objectName) ) goto fail;

                java::ArrayList<Vector3D *> *positions = nullptr;
                java::ArrayList<Vector3D *> *normals = nullptr;
                java::ArrayList<Vertex *> *surfaceVertices = nullptr;
                java::ArrayList<Patch *> *faces = nullptr;
                Material *material = nullptr;
                if ( !BinaryModelReaderSupport::arrayListFromIndices(record.positions, vectors, "surface.positions", &positions) ) goto fail;
                if ( !BinaryModelReaderSupport::arrayListFromIndices(record.normals, vectors, "surface.normals", &normals) ) goto fail;
                if ( !BinaryModelReaderSupport::arrayListFromIndices(record.vertices, vertices, "surface.vertices", &surfaceVertices) ) goto fail;
                if ( !BinaryModelReaderSupport::arrayListFromIndices(record.faces, patches, "surface.faces", &faces) ) goto fail;
                if ( !BinaryModelReaderSupport::pointerFromIndex(materials, record.materialIndex, "surface.material", &material) ) goto fail;

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
                if ( !BinaryModelReaderSupport::arrayListFromIndices(record.patchSetPatches, patches, "patchSet.patchList", &patchList) ) goto fail;
                geometry = new PatchSet(patchList);
                delete patchList;
            }

            if ( geometry == nullptr ) {
                Error::error("BinaryModelReader::read", "%s", "Could not instantiate geometry while loading binary model");
                goto fail;
            }

            geometry->className = static_cast<GeometryClassId>(record.classId);
            geometry->id = record.id;
            geometry->itemCount = record.itemCount;
            geometry->bounded = static_cast<char>(record.bounded ? 1 : 0);
            geometry->shaftCullGeometry = static_cast<char>(record.shaftCullGeometry ? 1 : 0);
            geometry->omit = static_cast<char>(record.omit ? 1 : 0);
            geometry->isDuplicate = record.isDuplicate;
            if ( !BinaryModelReaderSupport::setBoundingBoxFromCoordinates(&geometry->boundingBox, record.boundingBoxCoordinates) ) goto fail;

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
            const BinaryModelReaderGeometryRecord &record = geometryRecords[static_cast<long int>(i)];
            if ( record.classId == static_cast<int>(GeometryClassId::COMPOUND) ) {
                Compound *compound = static_cast<Compound *>(geometries.get(static_cast<long int>(i)));
                delete compound->children;
                java::ArrayList<Geometry *> *children = nullptr;
                if ( !BinaryModelReaderSupport::arrayListFromIndices(record.children, geometries, "compound.children", &children) ) goto fail;
                compound->children = children;
            }
        }

        if ( !BinaryModelReaderSupport::expectTag(input, "MODL") ) goto fail;
        modelRecord.currentColorIndex = BinaryModelReaderSupport::readInt32LE(input);
        if ( !BinaryModelReaderSupport::readNullableString(input, &modelRecord.currentMaterialName, &modelRecord.hasCurrentMaterialName) ) goto fail;
        if ( !BinaryModelReaderSupport::readNullableString(input, &modelRecord.currentObjectName, &modelRecord.hasCurrentObjectName) ) goto fail;
        if ( !BinaryModelReaderSupport::readNullableString(input, &modelRecord.currentVertexName, &modelRecord.hasCurrentVertexName) ) goto fail;
        modelRecord.geometryStackHeadIndex = BinaryModelReaderSupport::readInt32LE(input);
        modelRecord.inComplex = BinaryModelReaderSupport::readBool(input);
        modelRecord.inSurface = BinaryModelReaderSupport::readBool(input);
        modelRecord.monochrome = BinaryModelReaderSupport::readBool(input);
        modelRecord.readerContextIndex = BinaryModelReaderSupport::readInt32LE(input);
        modelRecord.transformContextIndex = BinaryModelReaderSupport::readInt32LE(input);

        if ( !BinaryModelReaderSupport::readIndexList(input, "model.currentFaceList", &modelRecord.currentFaceList) ) goto fail;
        if ( !BinaryModelReaderSupport::readIndexList(input, "model.currentGeometryList", &modelRecord.currentGeometryList) ) goto fail;
        if ( !BinaryModelReaderSupport::readIndexList(input, "model.currentNormalList", &modelRecord.currentNormalList) ) goto fail;
        if ( !BinaryModelReaderSupport::readIndexList(input, "model.currentPointList", &modelRecord.currentPointList) ) goto fail;
        if ( !BinaryModelReaderSupport::readIndexList(input, "model.currentVertexList", &modelRecord.currentVertexList) ) goto fail;
        if ( !BinaryModelReaderSupport::readIndexList(input, "model.geometries", &modelRecord.geometries) ) goto fail;
        if ( !BinaryModelReaderSupport::readIndexList(input, "model.materials", &modelRecord.materials) ) goto fail;

        model = new ParseSnapshotContext();
        ColorContext *modelCurrentColor = nullptr;
        ReaderContext *modelReaderContext = nullptr;
        TransformStackContext *modelTransformContext = nullptr;
        if ( !BinaryModelReaderSupport::pointerFromIndex(colorContexts, modelRecord.currentColorIndex, "model.currentColor", &modelCurrentColor) ) goto fail;
        if ( !BinaryModelReaderSupport::pointerFromIndex(readerContexts, modelRecord.readerContextIndex, "model.readerContext", &modelReaderContext) ) goto fail;
        if ( !BinaryModelReaderSupport::pointerFromIndex(transformContexts, modelRecord.transformContextIndex, "model.transformContext", &modelTransformContext) ) goto fail;
        model->currentColor = modelCurrentColor;
        model->geometryStackHeadIndex = modelRecord.geometryStackHeadIndex;
        model->inComplex = modelRecord.inComplex;
        model->inSurface = modelRecord.inSurface;
        model->monochrome = modelRecord.monochrome;
        model->readerContext = modelReaderContext;
        model->transformContext = modelTransformContext;

        if ( !BinaryModelReaderSupport::populateModelStrings(model, modelRecord) ) goto fail;

        if ( !BinaryModelReaderSupport::arrayListFromIndices(modelRecord.currentFaceList, patches, "model.currentFaceList", &model->currentFaceList) ) goto fail;
        if ( !BinaryModelReaderSupport::arrayListFromIndices(modelRecord.currentGeometryList, geometries, "model.currentGeometryList", &model->currentGeometryList) ) goto fail;
        if ( !BinaryModelReaderSupport::arrayListFromIndices(modelRecord.currentNormalList, vectors, "model.currentNormalList", &model->currentNormalList) ) goto fail;
        if ( !BinaryModelReaderSupport::arrayListFromIndices(modelRecord.currentPointList, vectors, "model.currentPointList", &model->currentPointList) ) goto fail;
        if ( !BinaryModelReaderSupport::arrayListFromIndices(modelRecord.currentVertexList, vertices, "model.currentVertexList", &model->currentVertexList) ) goto fail;
        if ( !BinaryModelReaderSupport::arrayListFromIndices(modelRecord.geometries, geometries, "model.geometries", &model->geometries) ) goto fail;
        if ( !BinaryModelReaderSupport::arrayListFromIndices(modelRecord.materials, materials, "model.materials", &model->materials) ) goto fail;

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
        Error::error("BinaryModelReader::read", "%s", "Unexpected failure while reading binary model");
        ok = false;
    }

fail:
    input.dispose();
    BinaryModelReaderCleanup::releaseVertexRecordIndexLists(vertexRecords);
    BinaryModelReaderCleanup::releaseGeometryRecordIndexLists(geometryRecords);
    BinaryModelReaderCleanup::releaseModelRecordIndexLists(&modelRecord);

    if ( !ok ) {
        BinaryModelReaderCleanup::cleanupPartialModel(
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
