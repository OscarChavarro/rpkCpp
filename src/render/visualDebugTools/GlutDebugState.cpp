#include "render/visualDebugTools/GlutDebugState.h"

GlutDebugState GLOBAL_render_glutDebugState;

GlutDebugState::GlutDebugState():
        primarySelectedPatch(0),
        showSelectedPathOnly(true),
        angleAroundViewportU(0.0f),
        angleAroundViewportV(0.0f)
{
}
