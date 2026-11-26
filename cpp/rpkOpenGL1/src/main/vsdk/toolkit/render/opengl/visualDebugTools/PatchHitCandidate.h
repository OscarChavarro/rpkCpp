#ifndef __PATCH_HIT_CANDIDATE__
#define __PATCH_HIT_CANDIDATE__

class PatchHitCandidate final {
  public:
    PatchHitCandidate();
    PatchHitCandidate(int patchIndex, double distance, bool frontFacing);

    int patchIndex;
    double distance;
    bool frontFacing;
};

#endif
