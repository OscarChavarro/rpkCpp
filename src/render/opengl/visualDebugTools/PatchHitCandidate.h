#ifndef __VISUAL_DEBUG_TOOLS_PATCH_HIT_CANDIDATE__
#define __VISUAL_DEBUG_TOOLS_PATCH_HIT_CANDIDATE__

class PatchHitCandidate final {
  public:
    PatchHitCandidate();
    PatchHitCandidate(int patchIndex, float distance, bool frontFacing);

    int patchIndex;
    float distance;
    bool frontFacing;
};

#endif
