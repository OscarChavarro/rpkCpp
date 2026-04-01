#include "render/opengl/visualDebugTools/GlutDebugState.h"

GlutDebugState GLOBAL_render_glutDebugState;

GlutDebugState::GlutDebugState():
        primarySelectedPatch(-1),
        selectedSelectedPatch(-1),
        showSelectedPathOnly(true),
        angleAroundViewportU(0.0f),
        angleAroundViewportV(0.0f)
{
}
