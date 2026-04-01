#ifndef __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_MODE__
#define __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_MODE__

enum class GlutDebugMode {
    RADIANCE_SCENE,
    GALERKIN_ELEMENT_HIERARCHY
};

GlutDebugMode nextGlutDebugMode(GlutDebugMode mode);
const char *glutDebugModeName(GlutDebugMode mode);

#endif
