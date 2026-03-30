#include "render/visualDebugTools/GlutDebugState.h"

GlutDebugState GLOBAL_render_glutDebugState;

GlutDebugState::GlutDebugState():
    selectedPatch(0),
    showSelectedPathOnly(false),
    angleAroundViewportU(0.0f),
    angleAroundViewportV(0.0f)
{
}
