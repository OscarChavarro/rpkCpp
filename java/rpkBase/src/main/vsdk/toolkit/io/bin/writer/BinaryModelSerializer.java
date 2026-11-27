package vsdk.toolkit.io.bin.writer;

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.IdentityHashMap;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.element.PlanarFace;
import vsdk.toolkit.io.wrapper.PersistenceElement;
import vsdk.toolkit.io.context.ColorContext;
import vsdk.toolkit.io.context.ParseSnapshotContext;
import vsdk.toolkit.io.context.ReaderContext;
import vsdk.toolkit.io.context.TransformSequenceContext;
import vsdk.toolkit.io.context.TransformStackContext;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.environment.material.PhongLobe;
import vsdk.toolkit.environment.material.PhongMaterial;
import vsdk.toolkit.environment.material.Texture;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.skin.GeometryClassId;
import vsdk.toolkit.skin.MeshSurface;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.PatchSet;
import vsdk.toolkit.environment.geometry.elements.RpkVertex;

public class BinaryModelSerializer {
    private static final byte[] BINARY_MODEL_MAGIC = new byte[] {
        'R', 'P', 'K', '_', 'M', 'G', 'F', '_',
        'B', 'I', 'N', '_', '1', 0, 0, 0
    };

    private static final int BINARY_MODEL_VERSION = 2;
    private static final int BINARY_MODEL_POINTER_SIZE = 8;
    private static final int BINARY_MODEL_LONG_SIZE = 8;
    private static final int BINARY_MODEL_PARSE_SNAPSHOT_CONTEXT_SIZE = 120;

    private static byte[] fixedCStringBytes(String text, int size) {
        byte[] output = new byte[size];
        if (size <= 0) {
            return output;
        }

        String safeText = text == null ? "" : text;
        byte[] bytes = safeText.getBytes(StandardCharsets.UTF_8);
        int maxCopy = size - 1;
        if (maxCopy < 0) {
            maxCopy = 0;
        }
        int copyLength = Math.min(maxCopy, bytes.length);
        System.arraycopy(bytes, 0, output, 0, copyLength);
        return output;
    }

    private static String safeLabel(String text) {
        if (text == null) {
            return "(null)";
        }
        return text;
    }

    private static void writeByte(OutputStream output, int value) {
        byte[] data = new byte[] {(byte)value};
        PersistenceElement.writeBytes(output, data, 1);
    }

    private static void writeBool(OutputStream output, boolean value) {
        writeByte(output, value ? 1 : 0);
    }

    private static void writeInt16LE(OutputStream output, int value) {
        byte[] data = new byte[2];
        data[0] = (byte)(value & 0xFF);
        data[1] = (byte)((value >>> 8) & 0xFF);
        PersistenceElement.writeBytes(output, data, 2);
    }

    private static void writeInt32LE(OutputStream output, int value) {
        byte[] data = new byte[4];
        data[0] = (byte)(value & 0xFF);
        data[1] = (byte)((value >>> 8) & 0xFF);
        data[2] = (byte)((value >>> 16) & 0xFF);
        data[3] = (byte)((value >>> 24) & 0xFF);
        PersistenceElement.writeBytes(output, data, 4);
    }

    private static void writeInt64LE(OutputStream output, long value) {
        byte[] data = new byte[8];
        data[0] = (byte)(value & 0xFFL);
        data[1] = (byte)((value >>> 8) & 0xFFL);
        data[2] = (byte)((value >>> 16) & 0xFFL);
        data[3] = (byte)((value >>> 24) & 0xFFL);
        data[4] = (byte)((value >>> 32) & 0xFFL);
        data[5] = (byte)((value >>> 40) & 0xFFL);
        data[6] = (byte)((value >>> 48) & 0xFFL);
        data[7] = (byte)((value >>> 56) & 0xFFL);
        PersistenceElement.writeBytes(output, data, 8);
    }

    // The binary model format stores 4-byte floats: the value is rounded to float
    private static void writeFloatLE(OutputStream output, double value) {
        writeInt32LE(output, Float.floatToIntBits((float)value));
    }

    private static void writeDoubleLE(OutputStream output, double value) {
        writeInt64LE(output, Double.doubleToLongBits(value));
    }

    private static boolean writeBytesChunked(OutputStream output, byte[] data, long length) {
        if (length < 0) {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::writeBytesChunked", "Negative block length");
            return false;
        }
        if (length == 0) {
            return true;
        }
        if (data == null) {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::writeBytesChunked", "Null block data");
            return false;
        }
        if (length > data.length) {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::writeBytesChunked", "Requested length exceeds source buffer");
            return false;
        }

        long offset = 0;
        long maxChunk = Integer.MAX_VALUE;
        while (offset < length) {
            long remaining = length - offset;
            int chunk = (int)(remaining < maxChunk ? remaining : maxChunk);
            byte[] chunkBuffer = new byte[chunk];
            System.arraycopy(data, (int)offset, chunkBuffer, 0, chunk);
            PersistenceElement.writeBytes(output, chunkBuffer, chunk);
            offset += chunk;
        }
        return true;
    }

    private static void writeTag(OutputStream output, String tag) {
        byte[] tagBytes = tag.getBytes(StandardCharsets.US_ASCII);
        PersistenceElement.writeBytes(output, tagBytes, 4);
    }

    private static boolean checkedLongToInt32(long value, String what, int[] result) {
        if (result == null || result.length == 0) {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::checkedLongToInt32", String.format("Null output pointer for %s", safeLabel(what)));
            return false;
        }
        if (value > Integer.MAX_VALUE || value < Integer.MIN_VALUE) {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::checkedLongToInt32", String.format("Overflow converting to int32 for %s", safeLabel(what)));
            return false;
        }
        result[0] = (int)value;
        return true;
    }

    private static boolean writeString(OutputStream output, String text) {
        if (text == null) {
            writeInt32LE(output, -1);
            return true;
        }

        byte[] bytes = text.getBytes(StandardCharsets.UTF_8);
        int[] size = new int[1];
        if (!checkedLongToInt32(bytes.length, "string length", size)) {
            return false;
        }

        writeInt32LE(output, size[0]);
        if (size[0] > 0) {
            PersistenceElement.writeBytes(output, bytes, size[0]);
        }
        return true;
    }

    private static void writeColor(OutputStream output, ColorRgb color) {
        writeFloatLE(output, (double)color.getR());
        writeFloatLE(output, (double)color.getG());
        writeFloatLE(output, (double)color.getB());
    }

    private static void writeVector(OutputStream output, Vector3Dd vector) {
        writeFloatLE(output, vector.x());
        writeFloatLE(output, vector.y());
        writeFloatLE(output, vector.z());
    }

    private static void writeBoundingBox(OutputStream output, RpkAxisAlignedBoundingBox boundingBox) {
        for (int i = 0; i < 6; i++) {
            writeFloatLE(output, boundingBox.valueAt(i));
        }
    }

    private static <T> boolean indexOfPointer(
        T ptr,
        IdentityHashMap<T, Integer> indices,
        String what,
        int[] result) {
        if (result == null || result.length == 0) {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::indexOfPointer", String.format("Missing pointer index for %s", safeLabel(what)));
            return false;
        }

        if (ptr == null) {
            result[0] = -1;
            return true;
        }

        Integer index = indices.get(ptr);
        if (index == null) {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::indexOfPointer", String.format("Missing pointer index for %s", safeLabel(what)));
            return false;
        }

        result[0] = index;
        return true;
    }

    private static <T> boolean writeIndexList(
        OutputStream output,
        ArrayList<T> list,
        IdentityHashMap<T, Integer> indices,
        String what) {
        if (list == null) {
            writeInt32LE(output, -1);
            return true;
        }

        int[] size = new int[1];
        if (!checkedLongToInt32(list.size(), what, size)) {
            return false;
        }

        writeInt32LE(output, size[0]);
        for (int i = 0; i < size[0]; i++) {
            int[] elementIndex = new int[1];
            if (!indexOfPointer(list.get(i), indices, what, elementIndex)) {
                return false;
            }
            writeInt32LE(output, elementIndex[0]);
        }
        return true;
    }

    private static boolean writeMaterialRecord(OutputStream output, RpkMaterial material) {
        if (!writeString(output, material.getName())) {
            return false;
        }
        writeBool(output, material.isSided());

        PhongMaterial surfaceMaterial = material.getSurfaceMaterial();
        PhongLobe emission = material.getEdf() != null ? surfaceMaterial.getEmission() : null;
        writeBool(output, emission != null);
        if (emission != null) {
            writeColor(output, emission.getDiffuse());
            writeColor(output, emission.getNonDiffuse());
            writeFloatLE(output, emission.getExponent());
        }

        writeBool(output, material.getBsdf() != null);
        if (material.getBsdf() == null) {
            return true;
        }

        PhongLobe reflection = surfaceMaterial.getReflection();
        writeBool(output, reflection != null);
        if (reflection != null) {
            writeColor(output, reflection.getDiffuse());
            writeColor(output, reflection.getNonDiffuse());
            writeFloatLE(output, reflection.getExponent());
        }

        PhongLobe transmission = surfaceMaterial.getTransmission();
        writeBool(output, transmission != null);
        if (transmission != null) {
            writeColor(output, transmission.getDiffuse());
            writeColor(output, transmission.getNonDiffuse());
            writeFloatLE(output, transmission.getExponent());
            writeFloatLE(output, surfaceMaterial.getStoredRefractionIndexReal());
            writeFloatLE(output, surfaceMaterial.getStoredRefractionIndexImaginary());
        }

        Texture texture = surfaceMaterial.getTexture();
        writeBool(output, texture != null);
        if (texture != null) {
            int width = texture.getWidth();
            int height = texture.getHeight();
            int channels = texture.getChannels();
            if (width < 0 || height < 0 || channels < 0) {
                Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::writeMaterialRecord", "Invalid texture dimensions");
                return false;
            }

            writeInt32LE(output, width);
            writeInt32LE(output, height);
            writeInt32LE(output, channels);

            long dataBytes = ((long)width) * ((long)height) * ((long)channels);
            writeInt64LE(output, dataBytes);

            if (dataBytes > 0) {
                byte[] data = texture.getData();
                if (data == null) {
                    Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::writeMaterialRecord", "Texture data is null with non-zero size");
                    return false;
                }
                if (!writeBytesChunked(output, data, dataBytes)) {
                    return false;
                }
            }
        }

        return true;
    }

    private static void writeColorContextRecord(OutputStream output, ColorContext colorContext) {
        writeInt32LE(output, colorContext.clock);
        writeInt16LE(output, colorContext.flags);
        for (int i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
            writeInt16LE(output, colorContext.straightSamples[i]);
        }
        writeInt64LE(output, colorContext.spectralStraightSum);
        writeFloatLE(output, colorContext.cx);
        writeFloatLE(output, colorContext.cy);
        writeFloatLE(output, colorContext.eff);
    }

    private static boolean writeReaderContextRecord(
        OutputStream output,
        ReaderContext readerContext,
        BinaryModelSerializationGraph context) {
        byte[] fileName = fixedCStringBytes(readerContext.fileName, 96);
        PersistenceElement.writeBytes(output, fileName, 96);
        writeBool(output, readerContext.inputStream != null);
        writeInt32LE(output, readerContext.fileContextId);

        byte[] inputLine = fixedCStringBytes(readerContext.inputLine, ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH);
        PersistenceElement.writeBytes(output, inputLine, ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH);

        writeInt32LE(output, readerContext.lineNumber);
        writeByte(output, readerContext.isPipe);

        int[] previousIndex = new int[1];
        if (!indexOfPointer(readerContext.prev, context.readerContextIndices, "readerContext.prev", previousIndex)) {
            return false;
        }
        writeInt32LE(output, previousIndex[0]);
        return true;
    }

    private static void writeTransformArrayRecord(OutputStream output, TransformSequenceContext transformArray) {
        writeInt32LE(output, transformArray.startingPosition.fileId);
        writeInt32LE(output, transformArray.startingPosition.lineNumber);
        writeInt64LE(output, transformArray.startingPosition.offset);
        writeInt32LE(output, transformArray.numberOfDimensions);
        for (int i = 0; i < TransformSequenceContext.TRANSFORM_MAXIMUM_DIMENSIONS; i++) {
            writeInt16LE(output, transformArray.transformArguments[i].i);
            writeInt16LE(output, transformArray.transformArguments[i].n);
            byte[] argument = fixedCStringBytes(transformArray.transformArguments[i].arg, 8);
            PersistenceElement.writeBytes(output, argument, 8);
        }
    }

    private static boolean writeTransformContextRecord(
        OutputStream output,
        TransformStackContext transformContext,
        BinaryModelSerializationGraph context) {
        writeInt64LE(output, transformContext.xid);
        writeInt16LE(output, transformContext.xac);
        writeInt16LE(output, transformContext.rev);

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                writeDoubleLE(output, transformContext.xf.transformMatrix.get(i, j));
            }
        }
        writeDoubleLE(output, transformContext.xf.scaleFactor);

        int[] transformArrayIndex = new int[1];
        if (!indexOfPointer(
            transformContext.transformationArray,
            context.transformArrayIndices,
            "transformContext.transformationArray",
            transformArrayIndex)) {
            return false;
        }
        writeInt32LE(output, transformArrayIndex[0]);

        int[] previousIndex = new int[1];
        if (!indexOfPointer(transformContext.prev, context.transformContextIndices, "transformContext.prev", previousIndex)) {
            return false;
        }
        writeInt32LE(output, previousIndex[0]);
        return true;
    }

    private static boolean writeVertexRecord(OutputStream output, RpkVertex vertex, BinaryModelSerializationGraph context) {
        writeInt32LE(output, vertex.id);

        int[] pointIndex = new int[1];
        if (!indexOfPointer(vertex.point, context.vectorIndices, "vertex.point", pointIndex)) {
            return false;
        }
        writeInt32LE(output, pointIndex[0]);

        int[] normalIndex = new int[1];
        if (!indexOfPointer(vertex.normal, context.vectorIndices, "vertex.normal", normalIndex)) {
            return false;
        }
        writeInt32LE(output, normalIndex[0]);

        int[] textureIndex = new int[1];
        if (!indexOfPointer(vertex.textureCoordinates, context.vectorIndices, "vertex.textureCoordinates", textureIndex)) {
            return false;
        }
        writeInt32LE(output, textureIndex[0]);

        writeColor(output, vertex.color);

        int[] backIndex = new int[1];
        if (!indexOfPointer(vertex.back, context.vertexIndices, "vertex.back", backIndex)) {
            return false;
        }
        writeInt32LE(output, backIndex[0]);

        writeInt32LE(output, vertex.tmp);
        writeBool(output, vertex.radianceData != null);
        return writeIndexList(output, vertex.patches, context.patchIndices, "vertex.patches");
    }

    private static boolean writePatchRecord(OutputStream output, Patch patch, BinaryModelSerializationGraph context) {
        writeInt32LE(output, patch.getId());

        int[] twinIndex = new int[1];
        if (!indexOfPointer(patch.getTwin(), context.patchIndices, "patch.twin", twinIndex)) {
            return false;
        }
        writeInt32LE(output, twinIndex[0]);

        writeInt32LE(output, patch.getNumberOfVertices());
        for (int i = 0; i < Patch.MAXIMUM_VERTICES_PER_PATCH; i++) {
            int[] vertexIndex = new int[1];
            if (!indexOfPointer(patch.getVertices()[i], context.vertexIndices, "patch.vertex", vertexIndex)) {
                return false;
            }
            writeInt32LE(output, vertexIndex[0]);
        }

        // The format keeps the patch's box; it is derived from the vertices
        RpkAxisAlignedBoundingBox patchBounds = new RpkAxisAlignedBoundingBox();
        patch.computeBounds(patchBounds);
        writeBool(output, true);
        writeBoundingBox(output, patchBounds);

        writeVector(output, patch.getNormal());
        writeFloatLE(output, patch.getPlaneConstant());
        writeFloatLE(output, patch.getTolerance());
        writeFloatLE(output, patch.getArea());
        writeVector(output, patch.midPoint());

        final PlanarFace face = patch.getFace();
        writeBool(output, face.hasJacobian());
        if (face.hasJacobian()) {
            writeFloatLE(output, face.getJacobianA());
            writeFloatLE(output, face.getJacobianB());
            writeFloatLE(output, face.getJacobianC());
        }

        // Direct potential: per-face solver data, not part of the model; written as
        // zero, the value every face starts with, so the file format is unchanged
        writeFloatLE(output, 0.0);
        writeInt32LE(output, patch.getDominantAxisIndex());
        // Omit flag: faces are excluded per query by the shaft's omit set, never
        // by a per-face flag; written false, as it always was
        writeBool(output, false);
        // Flags byte (direct visibility): per-face solver data, written cleared, the
        // value every face starts with, so the file format is unchanged
        writeByte(output, 0);
        // Display color: per-face display data, written black, the color every face
        // starts with, so the file format is unchanged
        writeColor(output, new ColorRgb(0.0, 0.0, 0.0));

        int[] materialIndex = new int[1];
        if (!indexOfPointer(patch.getMaterial(), context.materialIndices, "patch.material", materialIndex)) {
            return false;
        }
        writeInt32LE(output, materialIndex[0]);

        // Radiance elements are per-face solver state held by the scene's
        // RadianceElementTable, never serialized
        writeBool(output, false);
        return true;
    }

    private static boolean writeGeometryRecord(OutputStream output, RpkGeometry geometry, BinaryModelSerializationGraph context) {
        writeInt32LE(output, geometry.className);
        writeInt32LE(output, geometry.id);
        writeInt32LE(output, geometry.itemCount);
        writeBool(output, geometry.bounded);
        writeBool(output, geometry.shaftCullGeometry);
        // Omit flag: geometries were never omitted at run time (the shaft's omit set
        // excludes patches per query); written false, as it always was
        writeBool(output, false);
        writeBool(output, geometry.isDuplicate);
        writeBoundingBox(output, geometry.boundingBox);
        writeBool(output, false);
        writeBool(output, geometry.radianceData != null);

        if (geometry.className == GeometryClassId.SURFACE_MESH) {
            MeshSurface surface = (MeshSurface)geometry;
            if (!writeString(output, surface.objectName)) {
                return false;
            }
            writeInt32LE(output, surface.meshId);

            int[] materialIndex = new int[1];
            if (!indexOfPointer(surface.material, context.materialIndices, "surface.material", materialIndex)) {
                return false;
            }
            writeInt32LE(output, materialIndex[0]);

            if (!writeIndexList(output, surface.positions, context.vectorIndices, "surface.positions")) {
                return false;
            }
            if (!writeIndexList(output, surface.normals, context.vectorIndices, "surface.normals")) {
                return false;
            }
            if (!writeIndexList(output, surface.vertices, context.vertexIndices, "surface.vertices")) {
                return false;
            }
            if (!writeIndexList(output, surface.faces, context.patchIndices, "surface.faces")) {
                return false;
            }
        }
        else if (geometry.className == GeometryClassId.COMPOUND) {
            Compound compound = (Compound)geometry;
            if (!writeIndexList(output, compound.children, context.geometryIndices, "compound.children")) {
                return false;
            }
        }
        else if (geometry.className == GeometryClassId.PATCH_SET) {
            PatchSet patchSet = (PatchSet)geometry;
            if (!writeIndexList(output, patchSet.getPatchList(), context.patchIndices, "patchSet.patchList")) {
                return false;
            }
        }
        else {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::writeGeometryRecord", "Unsupported geometry class while writing");
            return false;
        }

        return true;
    }

    private static boolean writeModelRecord(OutputStream output, ParseSnapshotContext model, BinaryModelSerializationGraph context) {
        int[] currentColorIndex = new int[1];
        if (!indexOfPointer(model.currentColor, context.colorContextIndices, "model.currentColor", currentColorIndex)) {
            return false;
        }
        writeInt32LE(output, currentColorIndex[0]);

        if (!writeString(output, model.currentMaterialName)) {
            return false;
        }
        if (!writeString(output, model.currentObjectName)) {
            return false;
        }
        if (!writeString(output, model.currentVertexName)) {
            return false;
        }

        writeInt32LE(output, model.geometryStackHeadIndex);
        writeBool(output, model.inComplex);
        writeBool(output, model.inSurface);
        writeBool(output, model.monochrome);
        writeBool(output, model.singleSided);
        writeInt32LE(output, model.numberOfQuarterCircleDivisions);

        int[] readerContextIndex = new int[1];
        if (!indexOfPointer(model.readerContext, context.readerContextIndices, "model.readerContext", readerContextIndex)) {
            return false;
        }
        writeInt32LE(output, readerContextIndex[0]);

        int[] transformContextIndex = new int[1];
        if (!indexOfPointer(model.transformContext, context.transformContextIndices, "model.transformContext", transformContextIndex)) {
            return false;
        }
        writeInt32LE(output, transformContextIndex[0]);

        if (!writeIndexList(output, model.currentFaceList, context.patchIndices, "model.currentFaceList")) {
            return false;
        }
        if (!writeIndexList(output, model.currentGeometryList, context.geometryIndices, "model.currentGeometryList")) {
            return false;
        }
        if (!writeIndexList(output, model.currentNormalList, context.vectorIndices, "model.currentNormalList")) {
            return false;
        }
        if (!writeIndexList(output, model.currentPointList, context.vectorIndices, "model.currentPointList")) {
            return false;
        }
        if (!writeIndexList(output, model.currentVertexList, context.vertexIndices, "model.currentVertexList")) {
            return false;
        }
        if (!writeIndexList(output, model.geometries, context.geometryIndices, "model.geometries")) {
            return false;
        }
        if (!writeIndexList(output, model.materials, context.materialIndices, "model.materials")) {
            return false;
        }

        return true;
    }

    public static boolean write(ParseSnapshotContext model, String fileName) {
        if (model == null || fileName == null || fileName.isEmpty()) {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::write", "Invalid model or fileName");
            return false;
        }

        File file = new File(fileName);
        if (file.isDirectory()) {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::write", String.format("Could not open output file '%s'", fileName));
            return false;
        }

        try (FileOutputStream output = new FileOutputStream(fileName)) {
            BinaryModelSerializationGraph context = new BinaryModelSerializationGraph();
            if (!context.collectModel(model)) {
                return false;
            }

            PersistenceElement.writeBytes(output, BINARY_MODEL_MAGIC, 16);
            writeInt32LE(output, BINARY_MODEL_VERSION);
            writeInt32LE(output, BINARY_MODEL_POINTER_SIZE);
            writeInt32LE(output, BINARY_MODEL_LONG_SIZE);
            writeInt32LE(output, BINARY_MODEL_PARSE_SNAPSHOT_CONTEXT_SIZE);

            int[] count = new int[1];

            if (!checkedLongToInt32(context.vectors.size(), "vectors count", count)) {
                return false;
            }
            writeInt32LE(output, count[0]);

            if (!checkedLongToInt32(context.vertices.size(), "vertices count", count)) {
                return false;
            }
            writeInt32LE(output, count[0]);

            if (!checkedLongToInt32(context.patches.size(), "patches count", count)) {
                return false;
            }
            writeInt32LE(output, count[0]);

            if (!checkedLongToInt32(context.materials.size(), "materials count", count)) {
                return false;
            }
            writeInt32LE(output, count[0]);

            if (!checkedLongToInt32(context.geometries.size(), "geometries count", count)) {
                return false;
            }
            writeInt32LE(output, count[0]);

            if (!checkedLongToInt32(context.colorContexts.size(), "color contexts count", count)) {
                return false;
            }
            writeInt32LE(output, count[0]);

            if (!checkedLongToInt32(context.readerContexts.size(), "reader contexts count", count)) {
                return false;
            }
            writeInt32LE(output, count[0]);

            if (!checkedLongToInt32(context.transformArrays.size(), "transform arrays count", count)) {
                return false;
            }
            writeInt32LE(output, count[0]);

            if (!checkedLongToInt32(context.transformContexts.size(), "transform contexts count", count)) {
                return false;
            }
            writeInt32LE(output, count[0]);

            writeTag(output, "VEC3");
            for (int i = 0; i < context.vectors.size(); i++) {
                writeVector(output, context.vectors.get(i));
            }

            writeTag(output, "MTLS");
            for (int i = 0; i < context.materials.size(); i++) {
                if (!writeMaterialRecord(output, context.materials.get(i))) {
                    return false;
                }
            }

            writeTag(output, "COLR");
            for (int i = 0; i < context.colorContexts.size(); i++) {
                writeColorContextRecord(output, context.colorContexts.get(i));
            }

            writeTag(output, "RCTX");
            for (int i = 0; i < context.readerContexts.size(); i++) {
                if (!writeReaderContextRecord(output, context.readerContexts.get(i), context)) {
                    return false;
                }
            }

            writeTag(output, "XFAR");
            for (int i = 0; i < context.transformArrays.size(); i++) {
                writeTransformArrayRecord(output, context.transformArrays.get(i));
            }

            writeTag(output, "XFCT");
            for (int i = 0; i < context.transformContexts.size(); i++) {
                if (!writeTransformContextRecord(output, context.transformContexts.get(i), context)) {
                    return false;
                }
            }

            writeTag(output, "VRTX");
            for (int i = 0; i < context.vertices.size(); i++) {
                if (!writeVertexRecord(output, context.vertices.get(i), context)) {
                    return false;
                }
            }

            writeTag(output, "PTCH");
            for (int i = 0; i < context.patches.size(); i++) {
                if (!writePatchRecord(output, context.patches.get(i), context)) {
                    return false;
                }
            }

            writeTag(output, "GEOM");
            for (int i = 0; i < context.geometries.size(); i++) {
                if (!writeGeometryRecord(output, context.geometries.get(i), context)) {
                    return false;
                }
            }

            writeTag(output, "MODL");
            if (!writeModelRecord(output, model, context)) {
                return false;
            }

            return true;
        }
        catch (Throwable ignored) {
            Logger.reportMessage(null, Logger.ERROR, "BinaryModelSerializer::write", String.format("%s", "Unexpected failure while writing binary model"));
            return false;
        }
    }
}
