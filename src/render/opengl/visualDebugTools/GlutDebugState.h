#ifndef __GLUT_DEBUG_STATE__
#define __GLUT_DEBUG_STATE__

class GlutDebugState {
  public:
    int primarySelectedPatch;
    int selectedSelectedPatch;
    bool showSelectedPathOnly;
    float angleAroundViewportU;
    float angleAroundViewportV;

    GlutDebugState();
};

#endif
