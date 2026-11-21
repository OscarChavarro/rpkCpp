package vsdk.toolkit.app.rayCorpusReplay;

import java.nio.ByteBuffer;

/**
One QUERY record of a ray corpus (format in VisibilityQueryRecorder): the
query as the program issued it and the answer it received. An immutable value
once read.

Java adaptation: the reader fills a record in place (read), so that a chunk
of records is allocated once and reused.
*/
public final class VisibilityQueryRecord {
    public static final int NUMBER_OF_EXCLUDED_PATCHES = 4;
    public static final int NUMBER_OF_EXCLUDED_GEOMETRIES = 2;

    /** Bytes that follow the QUERY tag */
    public static final int SIZE_IN_BYTES = 4 * 18;

    private int path;
    private int hitFlags;
    private final float[] origin;
    private final float[] direction;
    private float minimumDistance;
    private float maximumDistance;
    private final int[] excludedPatchId;
    private final int[] excludedGeometryId;
    private int blockerPatchId;
    private float blockerDistance;

    public VisibilityQueryRecord() {
        path = 0;
        hitFlags = 0;
        origin = new float[3];
        direction = new float[3];
        minimumDistance = 0.0f;
        maximumDistance = 0.0f;
        excludedPatchId = new int[NUMBER_OF_EXCLUDED_PATCHES];
        excludedGeometryId = new int[NUMBER_OF_EXCLUDED_GEOMETRIES];
        blockerPatchId = 0;
        blockerDistance = 0.0f;
    }

    /**
    Reads the fields that follow the QUERY tag from a little-endian buffer
    holding at least SIZE_IN_BYTES bytes
    */
    void read(ByteBuffer buffer) {
        path = buffer.getInt();
        hitFlags = buffer.getInt();
        for (int axis = 0; axis < 3; axis++) {
            origin[axis] = buffer.getFloat();
        }
        for (int axis = 0; axis < 3; axis++) {
            direction[axis] = buffer.getFloat();
        }
        minimumDistance = buffer.getFloat();
        maximumDistance = buffer.getFloat();
        for (int i = 0; i < NUMBER_OF_EXCLUDED_PATCHES; i++) {
            excludedPatchId[i] = buffer.getInt();
        }
        for (int i = 0; i < NUMBER_OF_EXCLUDED_GEOMETRIES; i++) {
            excludedGeometryId[i] = buffer.getInt();
        }
        blockerPatchId = buffer.getInt();
        blockerDistance = buffer.getFloat();
    }

    public int getPath() {
        return path;
    }

    public int getHitFlags() {
        return hitFlags;
    }

    public float getOrigin(int axis) {
        return origin[axis];
    }

    public float getDirection(int axis) {
        return direction[axis];
    }

    public float getMinimumDistance() {
        return minimumDistance;
    }

    public float getMaximumDistance() {
        return maximumDistance;
    }

    public int getExcludedPatchId(int index) {
        return excludedPatchId[index];
    }

    public int getExcludedGeometryId(int index) {
        return excludedGeometryId[index];
    }

    public int getBlockerPatchId() {
        return blockerPatchId;
    }

    public float getBlockerDistance() {
        return blockerDistance;
    }
}
