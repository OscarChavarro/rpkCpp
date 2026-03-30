#ifndef __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_MOUSE_CONTROL__
#define __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_MOUSE_CONTROL__

#include "common/linealAlgebra/Ray.h"
#include "common/linealAlgebra/Vector3D.h"
#include "render/visualDebugTools/GlutDebugToolsModel.h"

class Scene;

class GlutDebugToolsMouseControl final {
  public:
    static bool handleMouseButton(int button, int state, int x, int y, GlutDebugToolsModel &model);
    static bool handleMouseMotion(int x, int y, GlutDebugToolsModel &model);

  private:
    static void buildPickRay(const GlutDebugToolsModel &model, int x, int y, Ray *ray);
    static bool pickPatchAtMousePosition(const GlutDebugToolsModel &model, int x, int y, int *patchIndex);
    static void applyInverseDebugRotationToRay(const GlutDebugToolsModel &model, Ray *ray);
    static Vector3D sceneRotationPivot(const Scene *scene);
    static void viewportAxesInWorld(const Scene *scene, Vector3D *axisU, Vector3D *axisV);
    static void rotateVectorAroundAxis(Vector3D *vector, const Vector3D &axis, float angleDegrees);
    static int clampCoord(int value, int maxExclusive);
    static void clampSelectedHierarchyLevel(GlutDebugToolsModel &model);
};

#endif
