package vsdk.toolkit.app.rayCorpusReplay;

import java.io.BufferedInputStream;
import java.io.EOFException;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;

/**
Sequential reader of a ray corpus file written by VisibilityQueryRecorder
(format in its documentation; little-endian, as the C++ and Java recorders
write it). GEOMETRY_LIST records are skipped and counted: the restricted-list
queries they precede are not index queries, and the shaft culling patch sets
they name exist only during the run that wrote them.

Java adaptation: close() releases the file that the C++ destructor closes.
*/
public final class RayCorpusReader {
    private static final int READ_BUFFER_SIZE = 16 * 1024 * 1024;
    private static final int TAG_GEOMETRY_LIST = 1;
    private static final int TAG_QUERY = 2;
    private static final byte[] MAGIC = "RPKVQC01".getBytes(StandardCharsets.US_ASCII);

    private InputStream file;
    private final byte[] bytes;
    private final ByteBuffer buffer;
    private long geometryLists;
    private boolean valid;

    public RayCorpusReader(String fileName) {
        file = null;
        bytes = new byte[Math.max(VisibilityQueryRecord.SIZE_IN_BYTES, MAGIC.length + 4)];
        buffer = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
        geometryLists = 0;
        valid = false;
        try {
            file = new BufferedInputStream(new FileInputStream(fileName), READ_BUFFER_SIZE);
        }
        catch (IOException e) {
            file = null;
            return;
        }
        boolean header = readBytes(MAGIC.length + 4) == MAGIC.length + 4;
        for (int i = 0; header && i < MAGIC.length; i++) {
            header = bytes[i] == MAGIC[i];
        }
        valid = header && buffer.getInt(MAGIC.length) == 1;
    }

    /**
    Reads up to count bytes into the buffer
    @return the bytes read: fewer than count only at the end of the file
    */
    private int readBytes(int count) {
        int total = 0;
        try {
            while (total < count) {
                final int n = file.read(bytes, total, count - total);
                if (n < 0) {
                    break;
                }
                total += n;
            }
        }
        catch (IOException e) {
            valid = false;
            return -1;
        }
        buffer.clear();
        return total;
    }

    /** True when the file opened and carries the RPKVQC01 version 1 header */
    public boolean isValid() {
        return valid;
    }

    /**
    Reads the next QUERY record into record.
    @return false at the end of the file, or on a malformed one (isValid
    then turns false)
    */
    public boolean next(VisibilityQueryRecord record) {
        while (valid) {
            if (readBytes(4) != 4) {
                return false;
            }
            final int tag = buffer.getInt();
            if (tag == TAG_QUERY) {
                if (readBytes(VisibilityQueryRecord.SIZE_IN_BYTES) != VisibilityQueryRecord.SIZE_IN_BYTES) {
                    valid = false;
                    return false;
                }
                record.read(buffer);
                return true;
            }
            if (tag != TAG_GEOMETRY_LIST) {
                valid = false;
                return false;
            }
            if (readBytes(4) != 4) {
                valid = false;
                return false;
            }
            final int count = buffer.getInt();
            if (count < 0) {
                valid = false;
                return false;
            }
            try {
                file.skipNBytes(4L * count);
            }
            catch (EOFException e) {
                return false;
            }
            catch (IOException e) {
                valid = false;
                return false;
            }
            geometryLists++;
        }
        return false;
    }

    public long getGeometryLists() {
        return geometryLists;
    }

    public void close() {
        if (file != null) {
            try {
                file.close();
            }
            catch (IOException e) {
                valid = false;
            }
            file = null;
        }
    }
}
