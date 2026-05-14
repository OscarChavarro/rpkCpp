#ifndef VISUAL_DEBUG_TOOLS_GLUT_DEBUG_MODE__
#define VISUAL_DEBUG_TOOLS_GLUT_DEBUG_MODE__

enum class GlutDebugMode {
    RADIANCE_SCENE,
    GALERKIN_ELEMENT_HIERARCHY
};

class GlutDebugModeTools final {
  public:
    static GlutDebugMode nextMode(GlutDebugMode mode);
    static const char *modeName(GlutDebugMode mode);

  private:
    static GlutDebugMode nextGlutDebugMode(GlutDebugMode mode);
    static const char *glutDebugModeName(GlutDebugMode mode);
};

#endif
