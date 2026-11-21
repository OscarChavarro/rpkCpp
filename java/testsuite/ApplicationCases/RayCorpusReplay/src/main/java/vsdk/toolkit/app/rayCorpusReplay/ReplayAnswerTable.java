package vsdk.toolkit.app.rayCorpusReplay;

/**
The answers of every implementation to a chunk of corpus queries, slot
numberOfImplementations * record + implementation, as primitive arrays (the
C++ replay's ReplayAnswer array). Each slot is written by one thread only.
*/
final class ReplayAnswerTable {
    private final int[] blockerPatchId;
    private final float[] distance;

    ReplayAnswerTable(int numberOfSlots) {
        blockerPatchId = new int[numberOfSlots];
        distance = new float[numberOfSlots];
    }

    void set(int slot, int inBlockerPatchId, float inDistance) {
        blockerPatchId[slot] = inBlockerPatchId;
        distance[slot] = inDistance;
    }

    ReplayAnswer get(int slot) {
        return new ReplayAnswer(blockerPatchId[slot], distance[slot]);
    }
}
