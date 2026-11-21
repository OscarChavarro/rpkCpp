package vsdk.toolkit.app.rayCorpusReplay;

/**
The answer of one implementation to one corpus query: the blocker's patch
identifier (Patch.id, 0 when nothing was hit) and the hit distance.
An immutable value; two answers are identical when the blocker and the bits
of the distance are.

Java adaptation: the replay keeps the answers of a chunk in primitive arrays
(ReplayAnswerTable) and builds values only to compare and report them.
*/
public final class ReplayAnswer {
    private final int blockerPatchId;
    private final float distance;

    public ReplayAnswer() {
        this(0, 0.0f);
    }

    public ReplayAnswer(int inBlockerPatchId, float inDistance) {
        blockerPatchId = inBlockerPatchId;
        distance = inDistance;
    }

    public boolean hasHit() {
        return blockerPatchId != 0;
    }

    public int getBlockerPatchId() {
        return blockerPatchId;
    }

    public float getDistance() {
        return distance;
    }

    public boolean sameDistance(ReplayAnswer other) {
        return Float.floatToRawIntBits(distance) == Float.floatToRawIntBits(other.distance);
    }

    public boolean identical(ReplayAnswer other) {
        return blockerPatchId == other.blockerPatchId && (!hasHit() || sameDistance(other));
    }
}
