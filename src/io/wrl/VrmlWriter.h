/**
Saves the result of a radiosity computation as a VRML file
*/

#ifndef __VRML_WRITER__
#define __VRML_WRITER__

#include "common/RenderOptions.h"
#include "common/linealAlgebra/Matrix4x4.h"
#include "scene/Camera.h"
#include "java/io/OutputStream.h"

class VrmlWriter {
  public:
    static void
    writeHeader(
        const Camera *camera,
        java::io::OutputStream *outputStream,
        const RenderOptions *renderOptions);

    static void writeTrailer(java::io::OutputStream *outputStream);

  private:
    static constexpr int MAXIMUM_CAMERA_STACK = 20;
    static const char *const RPK_HOME;

    static Camera globalCameraStack[MAXIMUM_CAMERA_STACK];
    static Camera *globalCameraStackPtr;

    static Camera *nextSavedCamera(Camera *previous);

    static Matrix4x4 transformModel(
        const Camera *camera,
        Vector3D *modelRotationAxis,
        float *modelRotationAngle);

    static void writeViewPoint(
        java::io::OutputStream *outputStream,
        const Matrix4x4 *modelTransform,
        const Camera *camera,
        const char *viewPointName);

    static void writeViewPoints(
        const Camera *camera,
        java::io::OutputStream *outputStream,
        const Matrix4x4 *modelTransform);
};

#endif
