#include "render/opengl/visualDebugTools/PatchHitCandidate.h"

PatchHitCandidate::PatchHitCandidate():
    patchIndex(-1),
    distance(0.0f),
    frontFacing(false)
{
}

PatchHitCandidate::PatchHitCandidate(int patchIndex, float distance, bool frontFacing):
    patchIndex(patchIndex),
    distance(distance),
    frontFacing(frontFacing)
{
}
