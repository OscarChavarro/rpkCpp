#ifndef VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_MOUSE_CONTROL__
#define VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_MOUSE_CONTROL__

#include "vsdk/toolkit/common/linealAlgebra/Ray.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugToolsModel.h"
#include "vsdk/toolkit/scene/Scene.h"

class GlutDebugToolsMouseControl final {
  public:
    static bool handleMouseButton(int button, int state, int x, int y, GlutDebugToolsModel &model);
    static bool handleMouseMotion(int x, int y, GlutDebugToolsModel &model);

  private:
    static constexpr int DRAG_START_THRESHOLD_PIXELS = 3;
    static constexpr float DRAG_ROTATION_DEGREES_PER_PIXEL = 0.25F;
    static constexpr float DRAG_DOLLY_VIEW_DISTANCE_FACTOR_PER_PIXEL = 0.01F;
    static constexpr float DEGREES_TO_RADIANS = 3.14159265358979323846F / 180.0F;

    static bool leftButtonDown;
    static bool rightButtonDown;
    static bool dragging;
    static bool pressWithShift;
    static int activeDragButton;
    static int pressX;
    static int pressY;
    static int lastX;
    static int lastY;

    static bool applyPatchSelection(int pickedPatchIndex, int *selectedPatch);
    static void syncCameraToViewport(const GlutDebugToolsModel &model);
    static void buildPickRay(const GlutDebugToolsModel &model, int x, int y, Ray *ray);
    static bool pickPatchAtMousePosition(const GlutDebugToolsModel &model, int x, int y, int *patchIndex);
    static void applyInverseDebugRotationToRay(const GlutDebugToolsModel &model, Ray *ray);
    static Vector3D sceneRotationPivot(const Scene *scene);
    static void viewportAxesInWorld(const Scene *scene, Vector3D *axisU, Vector3D *axisV);
    static void rotateVectorAroundAxis(Vector3D *vector, const Vector3D &axis, float angleDegrees);
    static bool dollyCameraAlongFront(GlutDebugToolsModel &model, int deltaY);
    static int clampCoord(int value, int maxExclusive);
    static void clampSelectedHierarchyLevel(GlutDebugToolsModel &model);
};

#endif
