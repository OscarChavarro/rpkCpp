package vsdk.toolkit.io.bin.reader;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.CoordinateAxis;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.element.PlanarFace;
import vsdk.toolkit.io.context.ColorContext;
import vsdk.toolkit.io.context.ParseSnapshotContext;
import vsdk.toolkit.io.context.ReaderContext;
import vsdk.toolkit.io.context.TransformSequenceContext;
import vsdk.toolkit.io.context.TransformStackContext;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.environment.material.PhongLobe;
import vsdk.toolkit.environment.material.Texture;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.skin.GeometryClassId;
import vsdk.toolkit.material.MaterialColorFlags;
import vsdk.toolkit.skin.MeshSurface;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.PatchSet;
import vsdk.toolkit.environment.geometry.elements.RpkVertex;

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

        ArrayList<Vector3Dd> vectors = new ArrayList<>();
        ArrayList<RpkVertex> vertices = new ArrayList<>();
        ArrayList<Patch> patches = new ArrayList<>();
        ArrayList<RpkMaterial> materials = new ArrayList<>();
        ArrayList<RpkGeometry> geometries = new ArrayList<>();
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

            require(BinaryModelReadPrimitives.initializeArrayList(vectors, vectorCount, (Vector3Dd)null, "vectors"));
            require(BinaryModelReadPrimitives.initializeArrayList(vertices, vertexCount, (RpkVertex)null, "vertices"));
            require(BinaryModelReadPrimitives.initializeArrayList(patches, patchCount, (Patch)null, "patches"));
            require(BinaryModelReadPrimitives.initializeArrayList(materials, materialCount, (RpkMaterial)null, "materials"));
            require(BinaryModelReadPrimitives.initializeArrayList(geometries, geometryCount, (RpkGeometry)null, "geometries"));
            require(BinaryModelReadPrimitives.initializeArrayList(colorContexts, colorContextCount, (ColorContext)null, "color contexts"));
            require(BinaryModelReadPrimitives.initializeArrayList(readerContexts, readerContextCount, (ReaderContext)null, "reader contexts"));
            require(BinaryModelReadPrimitives.initializeArrayList(transformArrays, transformArrayCount, (TransformSequenceContext)null, "transform arrays"));
            require(BinaryModelReadPrimitives.initializeArrayList(transformContexts, transformContextCount, (TransformStackContext)null, "transform contexts"));
            require(BinaryModelReadPrimitives.initializeArrayList(vertexRecords, vertexCount, new BinaryModelVertexRecordData(), "vertex records"));
            require(BinaryModelReadPrimitives.initializeArrayList(patchRecords, patchCount, new BinaryModelPatchRecordData(), "patch records"));
            require(BinaryModelReadPrimitives.initializeArrayList(geometryRecords, geometryCount, new BinaryModelGeometryRecordData(), "geometry records"));

            require(BinaryModelReadPrimitives.expectTag(input, "VEC3"));
            for (int i = 0; i < vectorCount; i++) {
                Vector3Dd[] vector = new Vector3Dd[1];
                require(BinaryModelReadPrimitives.readVector(input, vector));
                vectors.set(i, vector[0]);
            }

            require(BinaryModelReadPrimitives.expectTag(input, "MTLS"));
            for (int i = 0; i < materialCount; i++) {
                String[] materialName = new String[1];
                boolean[] hasMaterialName = new boolean[1];
                require(BinaryModelReadPrimitives.readNullableString(input, materialName, hasMaterialName));
                boolean sided = BinaryModelReadPrimitives.readBool(input);

                PhongLobe emission = null;
                PhongLobe reflection = null;
                PhongLobe transmission = null;
                double refractionIndexReal = 1.0;
                double refractionIndexImaginary = 0.0;
                Texture texture = null;
                boolean hasEdf = BinaryModelReadPrimitives.readBool(input);
                if (hasEdf) {
                    final ColorRgb[] kdHolder = new ColorRgb[1];
                    final ColorRgb[] ksHolder = new ColorRgb[1];
                    require(BinaryModelReadPrimitives.readColor(input, kdHolder));
                    require(BinaryModelReadPrimitives.readColor(input, ksHolder));
                    ColorRgb kd = kdHolder[0];
                    ColorRgb ks = ksHolder[0];
                    double ns = BinaryModelReadPrimitives.readFloatLE(input);
                    emission = new PhongLobe(kd, ks, ns);
                }

                boolean hasBsdf = BinaryModelReadPrimitives.readBool(input);
                if (hasBsdf) {
                    boolean hasBrdf = BinaryModelReadPrimitives.readBool(input);
                    if (hasBrdf) {
                        final ColorRgb[] kdHolder = new ColorRgb[1];
                        final ColorRgb[] ksHolder = new ColorRgb[1];
                        require(BinaryModelReadPrimitives.readColor(input, kdHolder));
                        require(BinaryModelReadPrimitives.readColor(input, ksHolder));
                        ColorRgb kd = kdHolder[0];
                        ColorRgb ks = ksHolder[0];
                        double ns = BinaryModelReadPrimitives.readFloatLE(input);
                        reflection = new PhongLobe(kd, ks, ns);
                    }

                    boolean hasBtdf = BinaryModelReadPrimitives.readBool(input);
                    if (hasBtdf) {
                        final ColorRgb[] kdHolder = new ColorRgb[1];
                        final ColorRgb[] ksHolder = new ColorRgb[1];
                        require(BinaryModelReadPrimitives.readColor(input, kdHolder));
                        require(BinaryModelReadPrimitives.readColor(input, ksHolder));
                        ColorRgb kd = kdHolder[0];
                        ColorRgb ks = ksHolder[0];
                        double ns = BinaryModelReadPrimitives.readFloatLE(input);
                        double nr = BinaryModelReadPrimitives.readFloatLE(input);
                        double ni = BinaryModelReadPrimitives.readFloatLE(input);
                        transmission = new PhongLobe(kd, ks, ns);
                        refractionIndexReal = nr;
                        refractionIndexImaginary = ni;
                    }

                    boolean hasTexture = BinaryModelReadPrimitives.readBool(input);
                    if (hasTexture) {
                        int width = BinaryModelReadPrimitives.readInt32LE(input);
                        int height = BinaryModelReadPrimitives.readInt32LE(input);
                        int channels = BinaryModelReadPrimitives.readInt32LE(input);
                        long dataBytes = BinaryModelReadPrimitives.readInt64LE(input);

                        if (width < 0 || height < 0 || channels < 0 || dataBytes < 0) {
                            Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Invalid texture dimensions in binary material"));
                            throw new ReadFailureException();
                        }

                        long expectedBytes = ((long)width) * ((long)height) * ((long)channels);
                        if (expectedBytes != dataBytes) {
                            Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Texture byte count mismatch in binary material"));
                            throw new ReadFailureException();
                        }

                        byte[] textureData = null;
                        if (dataBytes > 0) {
                            if (dataBytes > Integer.MAX_VALUE) {
                                Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Texture data too large for current platform"));
                                throw new ReadFailureException();
                            }
                            textureData = new byte[(int)dataBytes];
                            require(BinaryModelReadPrimitives.readBytesChunked(input, textureData, dataBytes));
                        }

                        texture = new Texture(width, height, channels, textureData);
                    }
                }

                String materialNameText = hasMaterialName[0] ? materialName[0] : "";
                materials.set(i, new RpkMaterial(
                    materialNameText, emission, hasBsdf, reflection, transmission, texture,
                    refractionIndexReal, refractionIndexImaginary, sided));
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

                double[][] transformValues = new double[4][4];
                for (int row = 0; row < 4; row++) {
                    for (int col = 0; col < 4; col++) {
                        transformValues[row][col] = BinaryModelReadPrimitives.readDoubleLE(input);
                    }
                }
                transformContext.xf.transformMatrix = new Matrix4x4d(transformValues);
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
                final ColorRgb[] colorHolder = new ColorRgb[1];
                require(BinaryModelReadPrimitives.readColor(input, colorHolder));
                record.color = colorHolder[0];
                record.backIndex = BinaryModelReadPrimitives.readInt32LE(input);
                record.tmp = BinaryModelReadPrimitives.readInt32LE(input);
                record.hasRadianceData = BinaryModelReadPrimitives.readBool(input);
                if (record.hasRadianceData) {
                    Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Vertex radianceData is not supported in binary reader"));
                    throw new ReadFailureException();
                }
                require(BinaryModelReadPrimitives.readIndexList(input, "vertex.patches", record.patchIndices));

                BinaryModelReadPrimitives.Out<Vector3Dd> pointOut = new BinaryModelReadPrimitives.Out<>();
                BinaryModelReadPrimitives.Out<Vector3Dd> normalOut = new BinaryModelReadPrimitives.Out<>();
                BinaryModelReadPrimitives.Out<Vector3Dd> texCoordsOut = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(vectors, record.pointIndex, "vertex.point", pointOut));
                require(BinaryModelReadPrimitives.pointerFromIndex(vectors, record.normalIndex, "vertex.normal", normalOut));
                require(BinaryModelReadPrimitives.pointerFromIndex(vectors, record.textureCoordinateIndex, "vertex.textureCoordinates", texCoordsOut));

                RpkVertex vertex = new RpkVertex(pointOut.value, normalOut.value, texCoordsOut.value, new ArrayList<Patch>());
                vertex.id = record.id;
                vertex.color = record.color;
                vertex.tmp = record.tmp;
                vertex.radianceData = null;
                vertices.set(i, vertex);
            }

            for (int i = 0; i < vertexCount; i++) {
                RpkVertex vertex = vertices.get(i);
                BinaryModelVertexRecordData record = vertexRecords.get(i);
                BinaryModelReadPrimitives.Out<RpkVertex> backOut = new BinaryModelReadPrimitives.Out<>();
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
                    Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Invalid patch vertex count while loading binary model"));
                    throw new ReadFailureException();
                }

                for (int j = 0; j < Patch.MAXIMUM_VERTICES_PER_PATCH; j++) {
                    record.vertexIndices[j] = BinaryModelReadPrimitives.readInt32LE(input);
                }

                record.hasBoundingBox = BinaryModelReadPrimitives.readBool(input);
                if (record.hasBoundingBox) {
                    require(BinaryModelReadPrimitives.readBoundingBoxCoordinates(input, record.boundingBoxCoordinates));
                }

                final Vector3Dd[] vectorHolder = new Vector3Dd[1];
                require(BinaryModelReadPrimitives.readVector(input, vectorHolder));
                record.normal = vectorHolder[0];
                record.planeConstant = BinaryModelReadPrimitives.readFloatLE(input);
                record.tolerance = BinaryModelReadPrimitives.readFloatLE(input);
                record.area = BinaryModelReadPrimitives.readFloatLE(input);
                require(BinaryModelReadPrimitives.readVector(input, vectorHolder));
                record.midPoint = vectorHolder[0];

                record.hasJacobian = BinaryModelReadPrimitives.readBool(input);
                record.jacobianA = 0.0;
                record.jacobianB = 0.0;
                record.jacobianC = 0.0;
                if (record.hasJacobian) {
                    record.jacobianA = BinaryModelReadPrimitives.readFloatLE(input);
                    record.jacobianB = BinaryModelReadPrimitives.readFloatLE(input);
                    record.jacobianC = BinaryModelReadPrimitives.readFloatLE(input);
                }

                record.directPotential = BinaryModelReadPrimitives.readFloatLE(input);
                record.dominantIndex = BinaryModelReadPrimitives.readInt32LE(input);
                record.omit = BinaryModelReadPrimitives.readBool(input);
                if (record.omit) {
                    Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Patch omit flag is not supported in binary reader"));
                    throw new ReadFailureException();
                }
                record.flags = BinaryModelReadPrimitives.readByte(input);
                final ColorRgb[] colorHolder = new ColorRgb[1];
                require(BinaryModelReadPrimitives.readColor(input, colorHolder));
                record.color = colorHolder[0];
                record.materialIndex = BinaryModelReadPrimitives.readInt32LE(input);
                record.hasRadianceData = BinaryModelReadPrimitives.readBool(input);
                if (record.hasRadianceData) {
                    Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Patch radianceData is not supported in binary reader"));
                    throw new ReadFailureException();
                }

                BinaryModelReadPrimitives.Out<RpkVertex> v1Out = new BinaryModelReadPrimitives.Out<>();
                BinaryModelReadPrimitives.Out<RpkVertex> v2Out = new BinaryModelReadPrimitives.Out<>();
                BinaryModelReadPrimitives.Out<RpkVertex> v3Out = new BinaryModelReadPrimitives.Out<>();
                BinaryModelReadPrimitives.Out<RpkVertex> v4Out = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[0], "patch.vertex[0]", v1Out));
                require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[1], "patch.vertex[1]", v2Out));
                require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[2], "patch.vertex[2]", v3Out));
                if (record.numberOfVertices == 4) {
                    require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[3], "patch.vertex[3]", v4Out));
                }

                Patch patch = new Patch(record.numberOfVertices, v1Out.value, v2Out.value, v3Out.value, v4Out.value);
                patch.setId(record.id);
                // Plane constant is immutable after construction and recomputed from geometry.
                // Plane tolerance is immutable after construction and recomputed from geometry.
                // Area is immutable after construction and recomputed from geometry.
                // The direct potential is read and discarded: it is per-face solver data,
                // held by the scene's DirectPotentialTable, and starts at zero
                // The dominant axis and the Jacobian are derived from the vertices by the
                // patch's face; the file's values must agree with them bit for bit
                if ( record.dominantIndex != patch.getDominantAxisIndex() ) {
                    Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Patch dominant axis disagrees with its vertices"));
                    throw new ReadFailureException();
                }
                // The flags byte (direct visibility) is read and discarded: it is per-face
                // solver data, held by the scene's FaceVisibilityTable, and starts cleared
                // The display color is read and discarded: it is per-face display data,
                // held by the scene's FaceColorTable, and starts black

                BinaryModelReadPrimitives.Out<RpkMaterial> materialOut = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(materials, record.materialIndex, "patch.material", materialOut));
                patch.setMaterial(materialOut.value);

                {
                    final PlanarFace face = patch.getFace();
                    final boolean jacobianAgrees = record.hasJacobian
                        ? face.hasJacobian() && face.getJacobianA() == record.jacobianA && face.getJacobianB() == record.jacobianB && face.getJacobianC() == record.jacobianC
                        : !face.hasJacobian();
                    if ( !jacobianAgrees ) {
                        Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Patch Jacobian disagrees with its vertices"));
                        throw new ReadFailureException();
                    }
                }

                // Patch bounding box is immutable after construction.
                // The constructor computes it from vertices.
                // We keep reading serialized bounding-box payload for format compatibility.

                patches.set(i, patch);
            }

            for (int i = 0; i < patchCount; i++) {
                Patch patch = patches.get(i);
                BinaryModelPatchRecordData record = patchRecords.get(i);
                BinaryModelReadPrimitives.Out<Patch> twinOut = new BinaryModelReadPrimitives.Out<>();
                require(BinaryModelReadPrimitives.pointerFromIndex(patches, record.twinIndex, "patch.twin", twinOut));
                patch.setTwin(twinOut.value);
            }

            for (int i = 0; i < vertexCount; i++) {
                RpkVertex vertex = vertices.get(i);
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
                if (record.omit) {
                    Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Geometry omit flag is not supported in binary reader"));
                    throw new ReadFailureException();
                }
                record.isDuplicate = BinaryModelReadPrimitives.readBool(input);
                require(BinaryModelReadPrimitives.readBoundingBoxCoordinates(input, record.boundingBoxCoordinates));
                record.hasRayIntersectionBox = BinaryModelReadPrimitives.readBool(input);
                record.hasRadianceData = BinaryModelReadPrimitives.readBool(input);
                if (record.hasRadianceData) {
                    Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Geometry radianceData is not supported in binary reader"));
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
                    Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Unsupported geometry type in binary model"));
                    throw new ReadFailureException();
                }
            }

            for (int i = 0; i < geometryCount; i++) {
                BinaryModelGeometryRecordData record = geometryRecords.get(i);
                RpkGeometry geometry = null;

                if (record.classId == GeometryClassId.SURFACE_MESH) {
                    BinaryModelReadPrimitives.Out<ArrayList<Vector3Dd>> positionsOut = new BinaryModelReadPrimitives.Out<>();
                    BinaryModelReadPrimitives.Out<ArrayList<Vector3Dd>> normalsOut = new BinaryModelReadPrimitives.Out<>();
                    BinaryModelReadPrimitives.Out<ArrayList<RpkVertex>> verticesOut = new BinaryModelReadPrimitives.Out<>();
                    BinaryModelReadPrimitives.Out<ArrayList<Patch>> facesOut = new BinaryModelReadPrimitives.Out<>();
                    BinaryModelReadPrimitives.Out<RpkMaterial> materialOut = new BinaryModelReadPrimitives.Out<>();

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
                    geometry = new Compound(new ArrayList<RpkGeometry>());
                }
                else if (record.classId == GeometryClassId.PATCH_SET) {
                    BinaryModelReadPrimitives.Out<ArrayList<Patch>> patchListOut = new BinaryModelReadPrimitives.Out<>();
                    require(BinaryModelReadPrimitives.arrayListFromIndices(record.patchSetPatches, patches, "patchSet.patchList", patchListOut));
                    geometry = new PatchSet(patchListOut.value);
                }

                if (geometry == null) {
                    Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Could not instantiate geometry while loading binary model"));
                    throw new ReadFailureException();
                }

                geometry.className = record.classId;
                geometry.id = record.id;
                geometry.itemCount = record.itemCount;
                geometry.bounded = record.bounded;
                geometry.shaftCullGeometry = record.shaftCullGeometry;
                geometry.isDuplicate = record.isDuplicate;
                require(BinaryModelReadPrimitives.setBoundingBoxFromCoordinates(geometry.boundingBox, record.boundingBoxCoordinates));

                geometry.radianceData = null;
                geometries.set(i, geometry);
            }

            for (int i = 0; i < geometryCount; i++) {
                BinaryModelGeometryRecordData record = geometryRecords.get(i);
                if (record.classId == GeometryClassId.COMPOUND) {
                    Compound compound = (Compound)geometries.get(i);
                    BinaryModelReadPrimitives.Out<ArrayList<RpkGeometry>> childrenOut = new BinaryModelReadPrimitives.Out<>();
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
            model.numberOfQuarterCircleDivisions = modelRecord.numberOfQuarterCircleDivisions;
            model.readerContext = readerContextOut.value;
            model.transformContext = transformContextOut.value;

            require(BinaryModelReadPrimitives.populateModelStrings(model, modelRecord));

            BinaryModelReadPrimitives.Out<ArrayList<Patch>> modelFaceListOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<RpkGeometry>> modelGeometryListOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<Vector3Dd>> modelNormalListOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<Vector3Dd>> modelPointListOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<RpkVertex>> modelVertexListOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<RpkGeometry>> modelGeometriesOut = new BinaryModelReadPrimitives.Out<>();
            BinaryModelReadPrimitives.Out<ArrayList<RpkMaterial>> modelMaterialsOut = new BinaryModelReadPrimitives.Out<>();

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
                if (patch != null && patch.getId() > maxPatchId) {
                    maxPatchId = patch.getId();
                }
            }
            Patch.setNextId(maxPatchId + 1);

            int maxGeometryId = -1;
            for (int i = 0; i < geometries.size(); i++) {
                RpkGeometry geometry = geometries.get(i);
                if (geometry != null && geometry.id > maxGeometryId) {
                    maxGeometryId = geometry.id;
                }
            }
            RpkGeometry.nextGeometryId = maxGeometryId + 1;

            ok = true;
        }
        catch (ReadFailureException ignored) {
            ok = false;
        }
        catch (Throwable ignored) {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelDeserializer::read", String.format("%s", "Unexpected failure while reading binary model"));
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
