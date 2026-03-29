#include "render/GlutDebugState.h"

GlutDebugState GLOBAL_render_glutDebugState;

GlutDebugState::GlutDebugState():
    selectedPatch(0),
    showSelectedPathOnly(false),
    angle(0.0f)
{
}
