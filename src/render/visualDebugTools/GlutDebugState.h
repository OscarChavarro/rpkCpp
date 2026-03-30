#ifndef __GLUT_DEBUG_STATE__
#define __GLUT_DEBUG_STATE__

class GlutDebugState {
  public:
    int selectedPatch;
    bool showSelectedPathOnly;
    float angleAroundViewportU;
    float angleAroundViewportV;

    GlutDebugState();
};

extern GlutDebugState GLOBAL_render_glutDebugState;

#endif
