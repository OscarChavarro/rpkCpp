package vsdk.toolkit.io.bin.reader;

import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.io.PersistenceElement;
import vsdk.toolkit.io.context.ParseSnapshotContext;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.BoundingBoxCoordinateIndex;

public class BinaryModelReadPrimitives {
    public static class Out<T> {
        public T value;
    }

    private static final byte[] BINARY_MODEL_MAGIC = new byte[] {
        'R', 'P', 'K', '_', 'M', 'G', 'F', '_',
        'B', 'I', 'N', '_', '1', 0, 0, 0
    };

    private static final int BINARY_MODEL_VERSION = 2;
    private static final int BINARY_MODEL_POINTER_SIZE = 8;
    private static final int BINARY_MODEL_LONG_SIZE = 8;
    private static final int BINARY_MODEL_PARSE_SNAPSHOT_CONTEXT_SIZE = 120;

    public static boolean reportReadError(String routine, String message) {
        Logger.error(routine, "%s", message);
        return false;
    }

    public static <T> boolean initializeArrayList(ArrayList<T> list, int count, T initialValue, String what) {
        if (list == null) {
            return reportReadError("BinaryModelReadPrimitives::initializeArrayList", "Null list pointer");
        }
        if (count < 0) {
            Logger.error("BinaryModelReadPrimitives::initializeArrayList", "Negative count while reading binary model (%s)", what);
            return false;
        }
        for (int i = 0; i < count; i++) {
            list.add(initialValue);
        }
        return true;
    }

    public static void releaseIndexListRecord(BinaryModelIndexListRef record) {
        if (record == null) {
            return;
        }
        if (record.indices != null) {
            record.indices.clear();
            record.indices = null;
        }
        record.isNull = true;
    }

    public static void readBytes(InputStream input, byte[] buffer, int length) {
        if (length <= 0 || buffer == null) {
            return;
        }
        PersistenceElement.readBytes(input, buffer, length);
    }

    public static boolean readBytesChunked(InputStream input, byte[] buffer, long length) {
        if (length <= 0) {
            return true;
        }
        if (buffer == null) {
            return reportReadError("BinaryModelReadPrimitives::readBytesChunked", "Null input buffer");
        }
        if (length > buffer.length) {
            return reportReadError("BinaryModelReadPrimitives::readBytesChunked", "Requested length exceeds destination buffer");
        }

        long offset = 0;
        long maxChunk = Integer.MAX_VALUE;
        while (offset < length) {
            long remaining = length - offset;
            int chunk = (int)(remaining < maxChunk ? remaining : maxChunk);
            byte[] chunkBuffer = new byte[chunk];
            readBytes(input, chunkBuffer, chunk);
            System.arraycopy(chunkBuffer, 0, buffer, (int)offset, chunk);
            offset += chunk;
        }
        return true;
    }

    public static byte readByte(InputStream input) {
        byte[] value = new byte[1];
        readBytes(input, value, 1);
        return value[0];
    }

    public static boolean readBool(InputStream input) {
        return readByte(input) != 0;
    }

    public static short readInt16LE(InputStream input) {
        byte[] value = new byte[2];
        readBytes(input, value, 2);
        return (short)PersistenceElement.byteArray2signedShortLE(value, 0);
    }

    public static int readInt32LE(InputStream input) {
        byte[] value = new byte[4];
        readBytes(input, value, 4);
        return (int)PersistenceElement.byteArray2longLE(value, 0);
    }

    public static long readInt64LE(InputStream input) {
        byte[] bytes = new byte[8];
        readBytes(input, bytes, 8);
        long value = 0;
        for (int i = 0; i < 8; i++) {
            value |= (((long)bytes[i]) & 0xFFL) << (8 * i);
        }
        return value;
    }

    public static float readFloatLE(InputStream input) {
        byte[] value = new byte[4];
        readBytes(input, value, 4);
        return PersistenceElement.byteArray2floatLE(value, 0);
    }

    public static double readDoubleLE(InputStream input) {
        byte[] value = new byte[8];
        readBytes(input, value, 8);
        return PersistenceElement.byteArray2doubleLE(value, 0);
    }

    public static boolean expectTag(InputStream input, String expected) {
        byte[] tag = new byte[4];
        readBytes(input, tag, 4);
        if (expected == null || expected.length() != 4) {
            return reportReadError("BinaryModelReadPrimitives::expectTag", "Unexpected section tag while reading binary model");
        }
        if (tag[0] != (byte)expected.charAt(0)
            || tag[1] != (byte)expected.charAt(1)
            || tag[2] != (byte)expected.charAt(2)
            || tag[3] != (byte)expected.charAt(3)) {
            return reportReadError("BinaryModelReadPrimitives::expectTag", "Unexpected section tag while reading binary model");
        }
        return true;
    }

    public static boolean readNonNegativeCount(InputStream input, String what, int[] count) {
        if (count == null || count.length == 0) {
            return reportReadError("BinaryModelReadPrimitives::readNonNegativeCount", "Null output count pointer");
        }
        count[0] = readInt32LE(input);
        if (count[0] < 0) {
            Logger.error("BinaryModelReadPrimitives::readNonNegativeCount", "Negative count while reading binary model (%s)", what);
            return false;
        }
        return true;
    }

    public static boolean readNullableString(InputStream input, String[] value, boolean[] hasValue) {
        if (value == null || value.length == 0 || hasValue == null || hasValue.length == 0) {
            return reportReadError("BinaryModelReadPrimitives::readNullableString", "Null string output pointer");
        }
        value[0] = null;
        hasValue[0] = false;

        int size = readInt32LE(input);
        if (size == -1) {
            return true;
        }
        if (size < -1) {
            return reportReadError("BinaryModelReadPrimitives::readNullableString", "Invalid negative string size");
        }

        byte[] bytes = new byte[size];
        if (size > 0) {
            readBytes(input, bytes, size);
        }

        value[0] = new String(bytes, StandardCharsets.UTF_8);
        hasValue[0] = true;
        return true;
    }

    public static boolean duplicateNullableString(boolean hasValue, String value, String[] text) {
        if (text == null || text.length == 0) {
            return reportReadError("BinaryModelReadPrimitives::duplicateNullableString", "Null output string pointer");
        }
        text[0] = null;
        if (!hasValue || value == null) {
            return true;
        }
        text[0] = new String(value);
        return true;
    }

    public static boolean readColor(InputStream input, ColorRgb color) {
        if (color == null) {
            return reportReadError("BinaryModelReadPrimitives::readColor", "Null color output pointer");
        }
        color.r = readFloatLE(input);
        color.g = readFloatLE(input);
        color.b = readFloatLE(input);
        return true;
    }

    public static boolean readVector(InputStream input, Vector3D vector) {
        if (vector == null) {
            return reportReadError("BinaryModelReadPrimitives::readVector", "Null vector output pointer");
        }
        vector.x = readFloatLE(input);
        vector.y = readFloatLE(input);
        vector.z = readFloatLE(input);
        return true;
    }

    public static boolean readBoundingBoxCoordinates(InputStream input, float[] coordinates) {
        if (coordinates == null || coordinates.length < 6) {
            return reportReadError("BinaryModelReadPrimitives::readBoundingBoxCoordinates", "Null bounding box coordinate buffer");
        }
        for (int i = 0; i < 6; i++) {
            coordinates[i] = readFloatLE(input);
        }
        return true;
    }

    public static boolean setBoundingBoxFromCoordinates(BoundingBox boundingBox, float[] coordinates) {
        if (boundingBox == null || coordinates == null || coordinates.length < 6) {
            return reportReadError("BinaryModelReadPrimitives::setBoundingBoxFromCoordinates", "Invalid bounding box assignment");
        }

        BoundingBox parsed = new BoundingBox();
        Vector3D minPoint = new Vector3D();
        Vector3D maxPoint = new Vector3D();
        minPoint.set(
            coordinates[BoundingBoxCoordinateIndex.MIN_X],
            coordinates[BoundingBoxCoordinateIndex.MIN_Y],
            coordinates[BoundingBoxCoordinateIndex.MIN_Z]);
        maxPoint.set(
            coordinates[BoundingBoxCoordinateIndex.MAX_X],
            coordinates[BoundingBoxCoordinateIndex.MAX_Y],
            coordinates[BoundingBoxCoordinateIndex.MAX_Z]);
        parsed.enlargeToIncludePoint(minPoint);
        parsed.enlargeToIncludePoint(maxPoint);
        boundingBox.copyFrom(parsed);
        return true;
    }

    public static boolean readIndexList(InputStream input, String what, BinaryModelIndexListRef record) {
        if (record == null) {
            return reportReadError("BinaryModelReadPrimitives::readIndexList", "Null output record");
        }

        record.isNull = false;
        record.indices = null;

        int count = readInt32LE(input);
        if (count == -1) {
            record.isNull = true;
            return true;
        }
        if (count < -1) {
            Logger.error("BinaryModelReadPrimitives::readIndexList", "Negative index list count while reading binary model (%s)", what);
            return false;
        }

        record.indices = new ArrayList<>(count > 0 ? count : 1);
        for (int i = 0; i < count; i++) {
            record.indices.add(readInt32LE(input));
        }

        return true;
    }

    public static <T> boolean pointerFromIndex(ArrayList<T> values, int index, String what, Out<T> result) {
        if (result == null) {
            return reportReadError("BinaryModelReadPrimitives::pointerFromIndex", "Null output pointer");
        }
        result.value = null;
        if (index == -1) {
            return true;
        }
        if (values == null || index < 0 || index >= values.size()) {
            Logger.error("BinaryModelReadPrimitives::pointerFromIndex", "Out of range index while reading binary model (%s)", what);
            return false;
        }
        result.value = values.get(index);
        return true;
    }

    public static <T> boolean arrayListFromIndices(
        BinaryModelIndexListRef record,
        ArrayList<T> values,
        String what,
        Out<ArrayList<T>> result) {
        if (result == null) {
            return reportReadError("BinaryModelReadPrimitives::arrayListFromIndices", "Null output pointer");
        }
        result.value = null;
        if (record == null || record.isNull) {
            return true;
        }
        if (record.indices == null) {
            return reportReadError("BinaryModelReadPrimitives::arrayListFromIndices", "Missing index list while reading binary model");
        }

        ArrayList<T> list = new ArrayList<>();
        Out<T> element = new Out<>();
        for (int i = 0; i < record.indices.size(); i++) {
            if (!pointerFromIndex(values, record.indices.get(i), what, element)) {
                return false;
            }
            list.add(element.value);
        }
        result.value = list;
        return true;
    }

    public static boolean validateBinaryHeader(InputStream input) {
        byte[] magic = new byte[16];
        readBytes(input, magic, 16);
        for (int i = 0; i < 16; i++) {
            if (magic[i] != BINARY_MODEL_MAGIC[i]) {
                return reportReadError("BinaryModelReadPrimitives::validateBinaryHeader", "Invalid binary model magic header");
            }
        }

        int version = readInt32LE(input);
        if (version != BINARY_MODEL_VERSION) {
            return reportReadError("BinaryModelReadPrimitives::validateBinaryHeader", "Unsupported binary model version");
        }

        int pointerSize = readInt32LE(input);
        int longSize = readInt32LE(input);
        int modelSize = readInt32LE(input);

        if (pointerSize != BINARY_MODEL_POINTER_SIZE
            || longSize != BINARY_MODEL_LONG_SIZE
            || modelSize != BINARY_MODEL_PARSE_SNAPSHOT_CONTEXT_SIZE) {
            return reportReadError("BinaryModelReadPrimitives::validateBinaryHeader", "Incompatible binary model platform/type sizes");
        }
        return true;
    }

    public static boolean populateModelStrings(ParseSnapshotContext model, BinaryModelSnapshotRecordData record) {
        if (model == null) {
            return reportReadError("BinaryModelReadPrimitives::populateModelStrings", "Null model in string population");
        }

        String[] tmp = new String[1];

        if (!duplicateNullableString(record.hasCurrentMaterialName, record.currentMaterialName, tmp)) {
            return false;
        }
        model.currentMaterialName = tmp[0];

        if (!duplicateNullableString(record.hasCurrentObjectName, record.currentObjectName, tmp)) {
            return false;
        }
        model.currentObjectName = tmp[0];

        if (!duplicateNullableString(record.hasCurrentVertexName, record.currentVertexName, tmp)) {
            return false;
        }
        model.currentVertexName = tmp[0];

        return true;
    }
}
