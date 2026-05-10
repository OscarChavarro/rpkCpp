package vsdk.toolkit.io.bin.reader;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.CoordinateAxis;
import vsdk.toolkit.common.linealAlgebra.Jacobian;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.io.context.ColorContext;
import vsdk.toolkit.io.context.ParseSnapshotContext;
import vsdk.toolkit.io.context.ReaderContext;
import vsdk.toolkit.io.context.TransformSequenceContext;
import vsdk.toolkit.io.context.TransformStackContext;
import vsdk.toolkit.material.Material;
import vsdk.toolkit.material.PhongBidirectionalReflectanceDistributionFunction;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.material.PhongBidirectionalTransmittanceDistributionFunction;
import vsdk.toolkit.material.PhongEmittanceDistributionFunction;
import vsdk.toolkit.material.Texture;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.GeometryClassId;
import vsdk.toolkit.skin.MaterialColorFlags;
import vsdk.toolkit.skin.MeshSurface;
import vsdk.toolkit.skin.MinMaxBox;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.PatchSet;
import vsdk.toolkit.skin.Vertex;

public class BinaryModelDeserializer {
    private static final class ReadFailureException extends RuntimeException {
        private static final long serialVersionUID = 1L;
    }

    private static void require(boolean ok) {
        if (!ok) {
            throw new ReadFailureException();
        }
    }

    private static String decodeCString(byte[] bytes) {
        if (bytes == null) {
            return "";
        }
        int len = 0;
        while (len < bytes.length && bytes[len] != 0) {
            len++;
        }
        return new String(bytes, 0, len, StandardCharsets.UTF_8);
    }

    private static String readFixedCString(BufferedInputStream input, int size, int forcedNullIndex) {
        byte[] bytes = new byte[size];
        BinaryModelReadPrimitives.readBytes(input, bytes, size);
        if (forcedNullIndex >= 0 && forcedNullIndex < size) {
            bytes[forcedNullIndex] = 0;
        }
        return decodeCString(bytes);
    }

    public static ParseSnapshotContext read(String fileName) {
        if (fileName == null || fileName.isEmpty()) {
            return null;
        }

        File file = new File(fileName);
        if (!(file.exists() && file.canRead() && file.isFile())) {
            return null;
        }

        ArrayList<Vector3D> vectors = new ArrayList<>();
        ArrayList<Vertex> vertices = new ArrayList<>();
        ArrayList<Patch> patches = new ArrayList<>();
        ArrayList<Material> materials = new ArrayList<>();
        ArrayList<Geometry> geometries = new ArrayList<>();
        ArrayList<ColorContext> colorContexts = new ArrayList<>();
        ArrayList<ReaderContext> readerContexts = new ArrayList<>();
        ArrayList<TransformSequenceContext> transformArrays = new ArrayList<>();
        ArrayList<TransformStackContext> transformContexts = new ArrayList<>();
        ArrayList<BinaryModelVertexRecordData> vertexRecords = new ArrayList<>();
        ArrayList<BinaryModelPatchRecordData> patchRecords = new ArrayList<>();
        ArrayList<BinaryModelGeometryRecordData> geometryRecords = new ArrayList<>();

        BinaryModelSnapshotRecordData modelRecord = new BinaryModelSnapshotRecordData();
        ParseSnapshotContext model = null;
        boolean ok = false;

        int vectorCount = 0;
        int vertexCount = 0;
        int patchCount = 0;
        int materialCount = 0;
        int geometryCount = 0;
        int colorContextCount = 0;
        int readerContextCount = 0;
        int transformArrayCount = 0;
        int transformContextCount = 0;

        try (BufferedInputStream input = new BufferedInputStream(new FileInputStream(fileName))) {
            require(BinaryModelReadPrimitives.validateBinaryHeader(input));

            int[] countOut = new int[1];

            require(BinaryModelReadPrimitives.readNonNegativeCount(input, "vectors", countOut));
            vectorCount = countOut[0];
            require(BinaryModelReadPrimitives.readNonNegativeCount(input, "vertices", countOut));
            vertexCount = countOut[0];
            require(BinaryModelReadPrimitives.readNonNegativeCount(input, "patches", countOut));
            patchCount = countOut[0];
            require(BinaryModelReadPrimitives.readNonNegativeCount(input, "materials", countOut));
            materialCount = countOut[0];
            require(BinaryModelReadPrimitives.readNonNegativeCount(input, "geometries", countOut));
            geometryCount = countOut[0];
            require(BinaryModelReadPrimitives.readNonNegativeCount(input, "color contexts", countOut));
            colorContextCount = countOut[0];
            require(BinaryModelReadPrimitives.readNonNegativeCount(input, "reader contexts", countOut));
            readerContextCount = countOut[0];
            require(BinaryModelReadPrimitives.readNonNegativeCount(input, "transform arrays", countOut));
            transformArrayCount = countOut[0];
            require(BinaryModelReadPrimitives.readNonNegativeCount(input, "transform contexts", countOut));
            transformContextCount = countOut[0];

            require(BinaryModelReadPrimitives.initializeArrayList(vectors, vectorCount, (Vector3D)null, "vectors"));
            require(BinaryModelReadPrimitives.initializeArrayList(vertices, vertexCount, (Vertex)null, "vertices"));
            require(BinaryModelReadPrimitives.initializeArrayList(patches, patchCount, (Patch)null, "patches"));
            require(BinaryModelReadPrimitives.initializeArrayList(materials, materialCount, (Material)null, "materials"));
            require(BinaryModelReadPrimitives.initializeArrayList(geometries, geometryCount, (Geometry)null, "geometries"));
            require(BinaryModelReadPrimitives.initializeArrayList(colorContexts, colorContextCount, (ColorContext)null, "color contexts"));
            require(BinaryModelReadPrimitives.initializeArrayList(readerContexts, readerContextCount, (ReaderContext)null, "reader contexts"));
            require(BinaryModelReadPrimitives.initializeArrayList(transformArrays, transformArrayCount, (TransformSequenceContext)null, "transform arrays"));
            require(BinaryModelReadPrimitives.initializeArrayList(transformContexts, transformContextCount, (TransformStackContext)null, "transform contexts"));
            require(BinaryModelReadPrimitives.initializeArrayList(vertexRecords, vertexCount, new BinaryModelVertexRecordData(), "vertex records"));
            require(BinaryModelReadPrimitives.initializeArrayList(patchRecords, patchCount, new BinaryModelPatchRecordData(), "patch records"));
            require(BinaryModelReadPrimitives.initializeArrayList(geometryRecords, geometryCount, new BinaryModelGeometryRecordData(), "geometry records"));

            require(BinaryModelReadPrimitives.expectTag(input, "VEC3"));
            for (int i = 0; i < vectorCount; i++) {
                Vector3D vector = new Vector3D();
                require(BinaryModelReadPrimitives.readVector(input, vector));
                vectors.set(i, vector);
            }

            require(BinaryModelReadPrimitives.expectTag(input, "MTLS"));
            for (int i = 0; i < materialCount; i++) {
                String[] materialName = new String[1];
                boolean[] hasMaterialName = new boolean[1];
                require(BinaryModelReadPrimitives.readNullableString(input, materialName, hasMaterialName));
                boolean sided = BinaryModelReadPrimitives.readBool(input);

                PhongEmittanceDistributionFunction edf = null;
                boolean hasEdf = BinaryModelReadPrimitives.readBool(input);
                if (hasEdf) {
                    ColorRgb kd = new ColorRgb();
                    ColorRgb ks = new ColorRgb();
                    require(BinaryModelReadPrimitives.readColor(input, kd));
                    require(BinaryModelReadPrimitives.readColor(input, ks));
                    float ns = BinaryModelReadPrimitives.readFloatLE(input);
                    edf = new PhongEmittanceDistributionFunction(kd, ks, ns);
                }

                PhongBidirectionalScatteringDistributionFunction bsdf = null;
                boolean hasBsdf = BinaryModelReadPrimitives.readBool(input);
                if (hasBsdf) {
                    PhongBidirectionalReflectanceDistributionFunction brdf = null;
                    PhongBidirectionalTransmittanceDistributionFunction btdf = null;
                    Texture texture = null;

                    boolean hasBrdf = BinaryModelReadPrimitives.readBool(input);
                    if (hasBrdf) {
                        ColorRgb kd = new ColorRgb();
                        ColorRgb ks = new ColorRgb();
                        require(BinaryModelReadPrimitives.readColor(input, kd));
                        require(BinaryModelReadPrimitives.readColor(input, ks));
                        float ns = BinaryModelReadPrimitives.readFloatLE(input);
                        brdf = new PhongBidirectionalReflectanceDistributionFunction(kd, ks, ns);
                    }

                    boolean hasBtdf = BinaryModelReadPrimitives.readBool(input);
                    if (hasBtdf) {
                        ColorRgb kd = new ColorRgb();
                        ColorRgb ks = new ColorRgb();
                        require(BinaryModelReadPrimitives.readColor(input, kd));
                        require(BinaryModelReadPrimitives.readColor(input, ks));
                        float ns = BinaryModelReadPrimitives.readFloatLE(input);
                        float nr = BinaryModelReadPrimitives.readFloatLE(input);
                        float ni = BinaryModelReadPrimitives.readFloatLE(input);
                        btdf = new PhongBidirectionalTransmittanceDistributionFunction(kd, ks, ns, nr, ni);
                    }

                    boolean hasTexture = BinaryModelReadPrimitives.readBool(input);
                    if (hasTexture) {
                        int width = BinaryModelReadPrimitives.readInt32LE(input);
                        int height = BinaryModelReadPrimitives.readInt32LE(input);
                        int channels = BinaryModelReadPrimitives.readInt32LE(input);
                        long dataBytes = BinaryModelReadPrimitives.readInt64LE(input);

                        if (width < 0 || height < 0 || channels < 0 || dataBytes < 0) {
                            Error.error("BinaryModelDeserializer::read", "%s", "Invalid texture dimensions in binary material");
                            throw new ReadFailureException();
                        }

                        long expectedBytes = ((long)width) * ((long)height) * ((long)channels);
                        if (expectedBytes != dataBytes) {
                            Error.error("BinaryModelDeserializer::read", "%s", "Texture byte count mismatch in binary material");
                            throw new ReadFailureException();
                        }

                        byte[] textureData = null;
                        if (dataBytes > 0) {
                            if (dataBytes > Integer.MAX_VALUE) {
                                Error.error("BinaryModelDeserializer::read", "%s", "Texture data too large for current platform");
                                throw new ReadFailureException();
                            }
                            textureData = new byte[(int)dataBytes];
                            require(BinaryModelReadPrimitives.readBytesChunked(input, textureData, dataBytes));
                        }

                        texture = new Texture(width, height, channels, textureData);
                    }

                    bsdf = new PhongBidirectionalScatteringDistributionFunction(brdf, btdf, texture);
                }

                String materialNameText = hasMaterialName[0] ? materialName[0] : "";
                materials.set(i, new Material(materialNameText, edf, bsdf, sided));
            }

            require(BinaryModelReadPrimitives.expectTag(input, "COLR"));
            for (int i = 0; i < colorContextCount; i++) {
                ColorContext colorContext = new ColorContext();
                colorContext.clock = BinaryModelReadPrimitives.readInt32LE(input);
                colorContext.flags = BinaryModelReadPrimitives.readInt16LE(input);
                for (int j = 0; j < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; j++) {
                    colorContext.straightSamples[j] = BinaryModelReadPrimitives.readInt16LE(input);
                }
                colorContext.spectralStraightSum = BinaryModelReadPrimitives.readInt64LE(input);
                colorContext.cx = BinaryModelReadPrimitives.readFloatLE(input);
                colorContext.cy = BinaryModelReadPrimitives.readFloatLE(input);
                colorContext.eff = BinaryModelReadPrimitives.readFloatLE(input);
                colorContexts.set(i, colorContext);
            }

            require(BinaryModelReadPrimitives.expectTag(input, "RCTX"));
            ArrayList<Integer> readerContextPrevIndex = new ArrayList<>();
            require(BinaryModelReadPrimitives.initializeArrayList(readerContextPrevIndex, readerContextCount, Integer.valueOf(-1), "reader context prev index"));
            for (int i = 0; i < readerContextCount; i++) {
                ReaderContext readerContext = new ReaderContext();
                readerContext.fileName = readFixedCString(input, 96, 95);

                boolean hasInputStream = BinaryModelReadPrimitives.readBool(input);
                readerContext.inputStream = null;
                if (hasInputStream) {
                    readerContext.inputStream = null;
                }

                readerContext.fileContextId = BinaryModelReadPrimitives.readInt32LE(input);
                readerContext.inputLine = readFixedCString(input, ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH, ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH - 1);
                readerContext.lineNumber = BinaryModelReadPrimitives.readInt32LE(input);
                readerContext.isPipe = (char)(BinaryModelReadPrimitives.readByte(input) & 0xFF);
                readerContextPrevIndex.set(i, BinaryModelReadPrimitives.readInt32LE(input));
                readerContext.prev = null;
                readerContexts.set(i, readerContext);
            }
            for (int i = 0; i < readerContextCount; i++) {
                BinaryModelReadPrimitives.Out<ReaderContext> prevOut = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(
                    readerContexts,
                    readerContextPrevIndex.get(i),
                    "readerContext.prev",
                    prevOut));
                readerContexts.get(i).prev = prevOut.value;
            }

            require(BinaryModelReadPrimitives.expectTag(input, "XFAR"));
            for (int i = 0; i < transformArrayCount; i++) {
                TransformSequenceContext transformArray = new TransformSequenceContext();
                transformArray.startingPosition.fileId = BinaryModelReadPrimitives.readInt32LE(input);
                transformArray.startingPosition.lineNumber = BinaryModelReadPrimitives.readInt32LE(input);
                transformArray.startingPosition.offset = BinaryModelReadPrimitives.readInt64LE(input);
                transformArray.numberOfDimensions = BinaryModelReadPrimitives.readInt32LE(input);
                for (int j = 0; j < TransformSequenceContext.TRANSFORM_MAXIMUM_DIMENSIONS; j++) {
                    transformArray.transformArguments[j].i = BinaryModelReadPrimitives.readInt16LE(input);
                    transformArray.transformArguments[j].n = BinaryModelReadPrimitives.readInt16LE(input);
                    transformArray.transformArguments[j].arg = readFixedCString(input, 8, 7);
                }
                transformArrays.set(i, transformArray);
            }

            require(BinaryModelReadPrimitives.expectTag(input, "XFCT"));
            ArrayList<Integer> transformContextArrayIndex = new ArrayList<>();
            ArrayList<Integer> transformContextPrevIndex = new ArrayList<>();
            require(BinaryModelReadPrimitives.initializeArrayList(transformContextArrayIndex, transformContextCount, Integer.valueOf(-1), "transform context array index"));
            require(BinaryModelReadPrimitives.initializeArrayList(transformContextPrevIndex, transformContextCount, Integer.valueOf(-1), "transform context prev index"));
            for (int i = 0; i < transformContextCount; i++) {
                TransformStackContext transformContext = new TransformStackContext();
                transformContext.xid = BinaryModelReadPrimitives.readInt64LE(input);
                transformContext.xac = BinaryModelReadPrimitives.readInt16LE(input);
                transformContext.rev = BinaryModelReadPrimitives.readInt16LE(input);

                for (int row = 0; row < 4; row++) {
                    for (int col = 0; col < 4; col++) {
                        transformContext.xf.transformMatrix.m[row][col] = BinaryModelReadPrimitives.readDoubleLE(input);
                    }
                }
                transformContext.xf.scaleFactor = BinaryModelReadPrimitives.readDoubleLE(input);
                transformContextArrayIndex.set(i, BinaryModelReadPrimitives.readInt32LE(input));
                transformContextPrevIndex.set(i, BinaryModelReadPrimitives.readInt32LE(input));
                transformContext.transformationArray = null;
                transformContext.prev = null;
                transformContexts.set(i, transformContext);
            }
            for (int i = 0; i < transformContextCount; i++) {
                TransformStackContext transformContext = transformContexts.get(i);

                BinaryModelReadPrimitives.Out<TransformSequenceContext> transformArrayOut = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(
                    transformArrays,
                    transformContextArrayIndex.get(i),
                    "transformContext.transformationArray",
                    transformArrayOut));
                transformContext.transformationArray = transformArrayOut.value;

                BinaryModelReadPrimitives.Out<TransformStackContext> prevOut = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(
                    transformContexts,
                    transformContextPrevIndex.get(i),
                    "transformContext.prev",
                    prevOut));
                transformContext.prev = prevOut.value;
            }

            require(BinaryModelReadPrimitives.expectTag(input, "VRTX"));
            for (int i = 0; i < vertexCount; i++) {
                BinaryModelVertexRecordData record = vertexRecords.get(i);
                record.id = BinaryModelReadPrimitives.readInt32LE(input);
                record.pointIndex = BinaryModelReadPrimitives.readInt32LE(input);
                record.normalIndex = BinaryModelReadPrimitives.readInt32LE(input);
                record.textureCoordinateIndex = BinaryModelReadPrimitives.readInt32LE(input);
                require(BinaryModelReadPrimitives.readColor(input, record.color));
                record.backIndex = BinaryModelReadPrimitives.readInt32LE(input);
                record.tmp = BinaryModelReadPrimitives.readInt32LE(input);
                record.hasRadianceData = BinaryModelReadPrimitives.readBool(input);
                if (record.hasRadianceData) {
                    Error.error("BinaryModelDeserializer::read", "%s", "Vertex radianceData is not supported in binary reader");
                    throw new ReadFailureException();
                }
                require(BinaryModelReadPrimitives.readIndexList(input, "vertex.patches", record.patchIndices));

                BinaryModelReadPrimitives.Out<Vector3D> pointOut = new BinaryModelReadPrimitives.Out<>();
                BinaryModelReadPrimitives.Out<Vector3D> normalOut = new BinaryModelReadPrimitives.Out<>();
                BinaryModelReadPrimitives.Out<Vector3D> texCoordsOut = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(vectors, record.pointIndex, "vertex.point", pointOut));
                require(BinaryModelReadPrimitives.pointerFromIndex(vectors, record.normalIndex, "vertex.normal", normalOut));
                require(BinaryModelReadPrimitives.pointerFromIndex(vectors, record.textureCoordinateIndex, "vertex.textureCoordinates", texCoordsOut));

                Vertex vertex = new Vertex(pointOut.value, normalOut.value, texCoordsOut.value, new ArrayList<Patch>());
                vertex.id = record.id;
                vertex.color = new ColorRgb(record.color.r, record.color.g, record.color.b);
                vertex.tmp = record.tmp;
                vertex.radianceData = null;
                vertices.set(i, vertex);
            }

            for (int i = 0; i < vertexCount; i++) {
                Vertex vertex = vertices.get(i);
                BinaryModelVertexRecordData record = vertexRecords.get(i);
                BinaryModelReadPrimitives.Out<Vertex> backOut = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.backIndex, "vertex.back", backOut));
                vertex.back = backOut.value;
            }

            require(BinaryModelReadPrimitives.expectTag(input, "PTCH"));
            for (int i = 0; i < patchCount; i++) {
                BinaryModelPatchRecordData record = patchRecords.get(i);
                record.id = BinaryModelReadPrimitives.readInt32LE(input);
                record.twinIndex = BinaryModelReadPrimitives.readInt32LE(input);
                record.numberOfVertices = BinaryModelReadPrimitives.readInt32LE(input);
                if (record.numberOfVertices != 3 && record.numberOfVertices != 4) {
                    Error.error("BinaryModelDeserializer::read", "%s", "Invalid patch vertex count while loading binary model");
                    throw new ReadFailureException();
                }

                for (int j = 0; j < Patch.MAXIMUM_VERTICES_PER_PATCH; j++) {
                    record.vertexIndices[j] = BinaryModelReadPrimitives.readInt32LE(input);
                }

                record.hasBoundingBox = BinaryModelReadPrimitives.readBool(input);
                if (record.hasBoundingBox) {
                    require(BinaryModelReadPrimitives.readBoundingBoxCoordinates(input, record.boundingBoxCoordinates));
                }

                require(BinaryModelReadPrimitives.readVector(input, record.normal));
                record.planeConstant = BinaryModelReadPrimitives.readFloatLE(input);
                record.tolerance = BinaryModelReadPrimitives.readFloatLE(input);
                record.area = BinaryModelReadPrimitives.readFloatLE(input);
                require(BinaryModelReadPrimitives.readVector(input, record.midPoint));

                record.hasJacobian = BinaryModelReadPrimitives.readBool(input);
                record.jacobianA = 0.0f;
                record.jacobianB = 0.0f;
                record.jacobianC = 0.0f;
                if (record.hasJacobian) {
                    record.jacobianA = BinaryModelReadPrimitives.readFloatLE(input);
                    record.jacobianB = BinaryModelReadPrimitives.readFloatLE(input);
                    record.jacobianC = BinaryModelReadPrimitives.readFloatLE(input);
                }

                record.directPotential = BinaryModelReadPrimitives.readFloatLE(input);
                record.dominantIndex = BinaryModelReadPrimitives.readInt32LE(input);
                record.omit = BinaryModelReadPrimitives.readBool(input);
                record.flags = BinaryModelReadPrimitives.readByte(input);
                require(BinaryModelReadPrimitives.readColor(input, record.color));
                record.materialIndex = BinaryModelReadPrimitives.readInt32LE(input);
                record.hasRadianceData = BinaryModelReadPrimitives.readBool(input);
                if (record.hasRadianceData) {
                    Error.error("BinaryModelDeserializer::read", "%s", "Patch radianceData is not supported in binary reader");
                    throw new ReadFailureException();
                }

                BinaryModelReadPrimitives.Out<Vertex> v1Out = new BinaryModelReadPrimitives.Out<>();
                BinaryModelReadPrimitives.Out<Vertex> v2Out = new BinaryModelReadPrimitives.Out<>();
                BinaryModelReadPrimitives.Out<Vertex> v3Out = new BinaryModelReadPrimitives.Out<>();
                BinaryModelReadPrimitives.Out<Vertex> v4Out = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[0], "patch.vertex[0]", v1Out));
                require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[1], "patch.vertex[1]", v2Out));
                require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[2], "patch.vertex[2]", v3Out));
                if (record.numberOfVertices == 4) {
                    require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[3], "patch.vertex[3]", v4Out));
                }

                Patch patch = new Patch(record.numberOfVertices, v1Out.value, v2Out.value, v3Out.value, v4Out.value);
                patch.id = record.id;
                patch.normal.copy(record.normal);
                patch.planeConstant = record.planeConstant;
                patch.tolerance = record.tolerance;
                patch.area = record.area;
                patch.midPoint.copy(record.midPoint);
                patch.directPotential = record.directPotential;
                if (record.dominantIndex >= 0 && record.dominantIndex < CoordinateAxis.values().length) {
                    patch.index = CoordinateAxis.values()[record.dominantIndex];
                }
                patch.omit = (byte)(record.omit ? 1 : 0);
                patch.setFlags(Byte.toUnsignedInt(record.flags));
                patch.color = new ColorRgb(record.color.r, record.color.g, record.color.b);

                BinaryModelReadPrimitives.Out<Material> materialOut = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(materials, record.materialIndex, "patch.material", materialOut));
                patch.material = materialOut.value;
                patch.radianceData = null;

                patch.jacobian = null;
                if (record.hasJacobian) {
                    patch.jacobian = new Jacobian(record.jacobianA, record.jacobianB, record.jacobianC);
                }

                patch.boundingBox = null;
                if (record.hasBoundingBox) {
                    patch.boundingBox = new BoundingBox();
                    require(BinaryModelReadPrimitives.setBoundingBoxFromCoordinates(patch.boundingBox, record.boundingBoxCoordinates));
                }

                patches.set(i, patch);
            }

            for (int i = 0; i < patchCount; i++) {
                Patch patch = patches.get(i);
                BinaryModelPatchRecordData record = patchRecords.get(i);
                BinaryModelReadPrimitives.Out<Patch> twinOut = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(patches, record.twinIndex, "patch.twin", twinOut));
                patch.twin = twinOut.value;
            }

            for (int i = 0; i < vertexCount; i++) {
                Vertex vertex = vertices.get(i);
                BinaryModelReadPrimitives.Out<ArrayList<Patch>> patchListOut = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.arrayListFromIndices(
                    vertexRecords.get(i).patchIndices,
                    patches,
                    "vertex.patches",
                    patchListOut));
                vertex.patches = patchListOut.value;
            }

            require(BinaryModelReadPrimitives.expectTag(input, "GEOM"));
            for (int i = 0; i < geometryCount; i++) {
                BinaryModelGeometryRecordData record = geometryRecords.get(i);
                record.classId = BinaryModelReadPrimitives.readInt32LE(input);
                record.id = BinaryModelReadPrimitives.readInt32LE(input);
                record.itemCount = BinaryModelReadPrimitives.readInt32LE(input);
                record.bounded = BinaryModelReadPrimitives.readBool(input);
                record.shaftCullGeometry = BinaryModelReadPrimitives.readBool(input);
                record.omit = BinaryModelReadPrimitives.readBool(input);
                record.isDuplicate = BinaryModelReadPrimitives.readBool(input);
                require(BinaryModelReadPrimitives.readBoundingBoxCoordinates(input, record.boundingBoxCoordinates));
                record.hasRayIntersectionBox = BinaryModelReadPrimitives.readBool(input);
                record.hasRadianceData = BinaryModelReadPrimitives.readBool(input);
                if (record.hasRadianceData) {
                    Error.error("BinaryModelDeserializer::read", "%s", "Geometry radianceData is not supported in binary reader");
                    throw new ReadFailureException();
                }

                record.hasObjectName = false;
                record.objectName = null;
                record.meshId = 0;
                record.materialIndex = -1;

                if (record.classId == GeometryClassId.SURFACE_MESH) {
                    String[] objectName = new String[1];
                    boolean[] hasObjectName = new boolean[1];
                    require(BinaryModelReadPrimitives.readNullableString(input, objectName, hasObjectName));
                    record.objectName = objectName[0];
                    record.hasObjectName = hasObjectName[0];
                    record.meshId = BinaryModelReadPrimitives.readInt32LE(input);
                    record.materialIndex = BinaryModelReadPrimitives.readInt32LE(input);
                    require(BinaryModelReadPrimitives.readIndexList(input, "surface.positions", record.positions));
                    require(BinaryModelReadPrimitives.readIndexList(input, "surface.normals", record.normals));
                    require(BinaryModelReadPrimitives.readIndexList(input, "surface.vertices", record.vertices));
                    require(BinaryModelReadPrimitives.readIndexList(input, "surface.faces", record.faces));
                }
                else if (record.classId == GeometryClassId.COMPOUND) {
                    require(BinaryModelReadPrimitives.readIndexList(input, "compound.children", record.children));
                }
                else if (record.classId == GeometryClassId.PATCH_SET) {
                    require(BinaryModelReadPrimitives.readIndexList(input, "patchSet.patchList", record.patchSetPatches));
                }
                else {
                    Error.error("BinaryModelDeserializer::read", "%s", "Unsupported geometry type in binary model");
                    throw new ReadFailureException();
                }
            }

            for (int i = 0; i < geometryCount; i++) {
                BinaryModelGeometryRecordData record = geometryRecords.get(i);
                Geometry geometry = null;

                if (record.classId == GeometryClassId.SURFACE_MESH) {
                    BinaryModelReadPrimitives.Out<ArrayList<Vector3D>> positionsOut = new BinaryModelReadPrimitives.Out<>();
                    BinaryModelReadPrimitives.Out<ArrayList<Vector3D>> normalsOut = new BinaryModelReadPrimitives.Out<>();
                    BinaryModelReadPrimitives.Out<ArrayList<Vertex>> verticesOut = new BinaryModelReadPrimitives.Out<>();
                    BinaryModelReadPrimitives.Out<ArrayList<Patch>> facesOut = new BinaryModelReadPrimitives.Out<>();
                    BinaryModelReadPrimitives.Out<Material> materialOut = new BinaryModelReadPrimitives.Out<>();

                    require(BinaryModelReadPrimitives.arrayListFromIndices(record.positions, vectors, "surface.positions", positionsOut));
                    require(BinaryModelReadPrimitives.arrayListFromIndices(record.normals, vectors, "surface.normals", normalsOut));
                    require(BinaryModelReadPrimitives.arrayListFromIndices(record.vertices, vertices, "surface.vertices", verticesOut));
                    require(BinaryModelReadPrimitives.arrayListFromIndices(record.faces, patches, "surface.faces", facesOut));
                    require(BinaryModelReadPrimitives.pointerFromIndex(materials, record.materialIndex, "surface.material", materialOut));

                    String objectName = record.hasObjectName ? new String(record.objectName) : null;
                    MeshSurface surface = new MeshSurface(
                        objectName,
                        materialOut.value,
                        positionsOut.value,
                        normalsOut.value,
                        null,
                        verticesOut.value,
                        facesOut.value,
                        MaterialColorFlags.NO_COLORS);
                    surface.meshId = record.meshId;
                    geometry = surface;
                }
                else if (record.classId == GeometryClassId.COMPOUND) {
                    geometry = new Compound(new ArrayList<Geometry>());
                }
                else if (record.classId == GeometryClassId.PATCH_SET) {
                    BinaryModelReadPrimitives.Out<ArrayList<Patch>> patchListOut = new BinaryModelReadPrimitives.Out<>();
                    require(BinaryModelReadPrimitives.arrayListFromIndices(record.patchSetPatches, patches, "patchSet.patchList", patchListOut));
                    geometry = new PatchSet(patchListOut.value);
                }

                if (geometry == null) {
                    Error.error("BinaryModelDeserializer::read", "%s", "Could not instantiate geometry while loading binary model");
                    throw new ReadFailureException();
                }

                geometry.className = record.classId;
                geometry.id = record.id;
                geometry.itemCount = record.itemCount;
                geometry.bounded = record.bounded;
                geometry.shaftCullGeometry = record.shaftCullGeometry;
                geometry.omit = record.omit;
                geometry.isDuplicate = record.isDuplicate;
                require(BinaryModelReadPrimitives.setBoundingBoxFromCoordinates(geometry.boundingBox, record.boundingBoxCoordinates));

                if (record.hasRayIntersectionBox) {
                    if (geometry.rayIntersectionBox == null) {
                        geometry.rayIntersectionBox = new MinMaxBox(geometry.boundingBox);
                    }
                    else {
                        geometry.rayIntersectionBox.updateFromBoundingBox(geometry.boundingBox);
                    }
                }
                else {
                    geometry.rayIntersectionBox = null;
                }

                geometry.radianceData = null;
                geometries.set(i, geometry);
            }

            for (int i = 0; i < geometryCount; i++) {
                BinaryModelGeometryRecordData record = geometryRecords.get(i);
                if (record.classId == GeometryClassId.COMPOUND) {
                    Compound compound = (Compound)geometries.get(i);
                    BinaryModelReadPrimitives.Out<ArrayList<Geometry>> childrenOut = new BinaryModelReadPrimitives.Out<>();
                    require(BinaryModelReadPrimitives.arrayListFromIndices(record.children, geometries, "compound.children", childrenOut));
                    compound.children = childrenOut.value;
                }
            }

            require(BinaryModelReadPrimitives.expectTag(input, "MODL"));
            modelRecord.currentColorIndex = BinaryModelReadPrimitives.readInt32LE(input);
            String[] textOut = new String[1];
            boolean[] hasTextOut = new boolean[1];
            require(BinaryModelReadPrimitives.readNullableString(input, textOut, hasTextOut));
            modelRecord.currentMaterialName = textOut[0];
            modelRecord.hasCurrentMaterialName = hasTextOut[0];
            require(BinaryModelReadPrimitives.readNullableString(input, textOut, hasTextOut));
            modelRecord.currentObjectName = textOut[0];
            modelRecord.hasCurrentObjectName = hasTextOut[0];
            require(BinaryModelReadPrimitives.readNullableString(input, textOut, hasTextOut));
            modelRecord.currentVertexName = textOut[0];
            modelRecord.hasCurrentVertexName = hasTextOut[0];
            modelRecord.geometryStackHeadIndex = BinaryModelReadPrimitives.readInt32LE(input);
            modelRecord.inComplex = BinaryModelReadPrimitives.readBool(input);
            modelRecord.inSurface = BinaryModelReadPrimitives.readBool(input);
            modelRecord.monochrome = BinaryModelReadPrimitives.readBool(input);
            modelRecord.singleSided = BinaryModelReadPrimitives.readBool(input);
            modelRecord.warpConeEnds = BinaryModelReadPrimitives.readBool(input);
            modelRecord.numberOfQuarterCircleDivisions = BinaryModelReadPrimitives.readInt32LE(input);
            modelRecord.readerContextIndex = BinaryModelReadPrimitives.readInt32LE(input);
            modelRecord.transformContextIndex = BinaryModelReadPrimitives.readInt32LE(input);

            require(BinaryModelReadPrimitives.readIndexList(input, "model.currentFaceList", modelRecord.currentFaceList));
            require(BinaryModelReadPrimitives.readIndexList(input, "model.currentGeometryList", modelRecord.currentGeometryList));
            require(BinaryModelReadPrimitives.readIndexList(input, "model.currentNormalList", modelRecord.currentNormalList));
            require(BinaryModelReadPrimitives.readIndexList(input, "model.currentPointList", modelRecord.currentPointList));
            require(BinaryModelReadPrimitives.readIndexList(input, "model.currentVertexList", modelRecord.currentVertexList));
            require(BinaryModelReadPrimitives.readIndexList(input, "model.geometries", modelRecord.geometries));
            require(BinaryModelReadPrimitives.readIndexList(input, "model.materials", modelRecord.materials));

            model = new ParseSnapshotContext();
            BinaryModelReadPrimitives.Out<ColorContext> currentColorOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ReaderContext> readerContextOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<TransformStackContext> transformContextOut = new BinaryModelReadPrimitives.Out<>();
            require(BinaryModelReadPrimitives.pointerFromIndex(colorContexts, modelRecord.currentColorIndex, "model.currentColor", currentColorOut));
            require(BinaryModelReadPrimitives.pointerFromIndex(readerContexts, modelRecord.readerContextIndex, "model.readerContext", readerContextOut));
            require(BinaryModelReadPrimitives.pointerFromIndex(transformContexts, modelRecord.transformContextIndex, "model.transformContext", transformContextOut));

            model.currentColor = currentColorOut.value;
            model.geometryStackHeadIndex = modelRecord.geometryStackHeadIndex;
            model.inComplex = modelRecord.inComplex;
            model.inSurface = modelRecord.inSurface;
            model.monochrome = modelRecord.monochrome;
            model.singleSided = modelRecord.singleSided;
            model.warpConeEnds = modelRecord.warpConeEnds;
            model.numberOfQuarterCircleDivisions = modelRecord.numberOfQuarterCircleDivisions;
            model.readerContext = readerContextOut.value;
            model.transformContext = transformContextOut.value;

            require(BinaryModelReadPrimitives.populateModelStrings(model, modelRecord));

            BinaryModelReadPrimitives.Out<ArrayList<Patch>> modelFaceListOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<Geometry>> modelGeometryListOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<Vector3D>> modelNormalListOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<Vector3D>> modelPointListOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<Vertex>> modelVertexListOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<Geometry>> modelGeometriesOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<Material>> modelMaterialsOut = new BinaryModelReadPrimitives.Out<>();

            require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.currentFaceList, patches, "model.currentFaceList", modelFaceListOut));
            require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.currentGeometryList, geometries, "model.currentGeometryList", modelGeometryListOut));
            require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.currentNormalList, vectors, "model.currentNormalList", modelNormalListOut));
            require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.currentPointList, vectors, "model.currentPointList", modelPointListOut));
            require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.currentVertexList, vertices, "model.currentVertexList", modelVertexListOut));
            require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.geometries, geometries, "model.geometries", modelGeometriesOut));
            require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.materials, materials, "model.materials", modelMaterialsOut));

            model.currentFaceList = modelFaceListOut.value;
            model.currentGeometryList = modelGeometryListOut.value;
            model.currentNormalList = modelNormalListOut.value;
            model.currentPointList = modelPointListOut.value;
            model.currentVertexList = modelVertexListOut.value;
            model.geometries = modelGeometriesOut.value;
            model.materials = modelMaterialsOut.value;

            int maxPatchId = 0;
            for (int i = 0; i < patches.size(); i++) {
                Patch patch = patches.get(i);
                if (patch != null && patch.id > maxPatchId) {
                    maxPatchId = patch.id;
                }
            }
            Patch.setNextId(maxPatchId + 1);

            int maxGeometryId = -1;
            for (int i = 0; i < geometries.size(); i++) {
                Geometry geometry = geometries.get(i);
                if (geometry != null && geometry.id > maxGeometryId) {
                    maxGeometryId = geometry.id;
                }
            }
            Geometry.nextGeometryId = maxGeometryId + 1;

            ok = true;
        }
        catch (ReadFailureException ignored) {
            ok = false;
        }
        catch (Throwable ignored) {
            Error.error("BinaryModelDeserializer::read", "%s", "Unexpected failure while reading binary model");
            ok = false;
        }

        BinaryModelReadCleanup.releaseVertexRecordIndexLists(vertexRecords);
        BinaryModelReadCleanup.releaseGeometryRecordIndexLists(geometryRecords);
        BinaryModelReadCleanup.releaseModelRecordIndexLists(modelRecord);

        if (!ok) {
            BinaryModelReadCleanup.cleanupPartialModel(
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
            return null;
        }

        return model;
    }
}
