#ifndef __GLUT_DEBUG_MODE__
#define __GLUT_DEBUG_MODE__

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
