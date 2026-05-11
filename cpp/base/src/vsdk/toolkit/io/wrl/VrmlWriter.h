/**
Saves the result of a radiosity computation as a VRML file
*/

#ifndef VRML_WRITER__
#define VRML_WRITER__

#include "vsdk/toolkit/java/io/OutputStream.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/Camera.h"

class VrmlWriter {
  public:
    static void
    writeHeader(
        const Camera *camera,
        java::OutputStream *outputStream,
        const RendererConfiguration *renderOptions);

    static void writeTrailer(java::OutputStream *outputStream);

  private:
    static constexpr int MAXIMUM_CAMERA_STACK = 20;
    static const char *const RPK_HOME;

    static Camera cameraStack[MAXIMUM_CAMERA_STACK];

    static Camera *nextSavedCamera(Camera *previous);

    static Matrix4x4 transformModel(
        const Camera *camera,
        Vector3D *modelRotationAxis,
        float *modelRotationAngle);

    static void writeViewPoint(
        java::OutputStream *outputStream,
        const Matrix4x4 *modelTransform,
        const Camera *camera,
        const char *viewPointName);

    static void writeViewPoints(
        const Camera *camera,
        java::OutputStream *outputStream,
        const Matrix4x4 *modelTransform);

    static void writeFormatted(java::OutputStream *outputStream, const char *format, ...);
};

#endif
