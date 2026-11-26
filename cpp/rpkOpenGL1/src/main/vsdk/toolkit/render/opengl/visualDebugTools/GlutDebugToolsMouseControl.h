#ifndef __GLUT_DEBUG_TOOLS_MOUSE_CONTROL__
#define __GLUT_DEBUG_TOOLS_MOUSE_CONTROL__

#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugToolsModel.h"
#include "vsdk/toolkit/scene/Scene.h"

class GlutDebugToolsMouseControl final {
  public:
    static bool handleMouseButton(int button, int state, int x, int y, GlutDebugToolsModel &model);
    static bool handleMouseMotion(int x, int y, GlutDebugToolsModel &model);

  private:
    static constexpr int DRAG_START_THRESHOLD_PIXELS = 3;
    static constexpr double DRAG_ROTATION_DEGREES_PER_PIXEL = 0.25;
    static constexpr double DRAG_DOLLY_VIEW_DISTANCE_FACTOR_PER_PIXEL = 0.01;
    static constexpr double DEGREES_TO_RADIANS = 3.14159265358979323846 / 180.0;

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
    static Vector3Dd sceneRotationPivot(const Scene *scene);
    static void viewportAxesInWorld(const Scene *scene, Vector3Dd *axisU, Vector3Dd *axisV);
    static void rotateVectorAroundAxis(Vector3Dd *vector, const Vector3Dd &axis, double angleDegrees);
    static bool dollyCameraAlongFront(GlutDebugToolsModel &model, int deltaY);
    static int clampCoord(int value, int maxExclusive);
    static void clampSelectedHierarchyLevel(GlutDebugToolsModel &model);
};

#endif
