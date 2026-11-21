#ifndef REPLAY_ANSWER__
#define REPLAY_ANSWER__

#include <cstring>

/**
The answer of one implementation to one corpus query: the blocker's patch
identifier (Patch::getId(), 0 when nothing was hit) and the hit distance.
An immutable value; two answers are identical when the blocker and the bits
of the distance are.
*/
class ReplayAnswer {
  private:
    int blockerPatchId;
    float distance;

  public:
    ReplayAnswer(): blockerPatchId(0), distance(0.0F) {}
    ReplayAnswer(const int inBlockerPatchId, const float inDistance):
        blockerPatchId(inBlockerPatchId), distance(inDistance) {}

    bool hasHit() const { return blockerPatchId != 0; }
    int getBlockerPatchId() const { return blockerPatchId; }
    float getDistance() const { return distance; }

    bool sameDistance(const ReplayAnswer &other) const {
        return memcmp(&distance, &other.distance, sizeof(float)) == 0;
    }

    bool identical(const ReplayAnswer &other) const {
        return blockerPatchId == other.blockerPatchId && (!hasHit() || sameDistance(other));
    }
};

#endif
