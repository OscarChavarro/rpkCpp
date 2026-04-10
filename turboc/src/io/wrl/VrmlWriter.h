#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
Saves the result of a radiosity computation as a VRML file
*/

#ifndef __VRML_WRITER__
#define __VRML_WRITER__

#include "java/io/OutputStream.h"
#include "common/linealAlgebra/Matrix4x4.h"
#include "common/RenderOptions.h"
#include "scene/Camera.h"

class VrmlWriter {
  public:
    static void
    writeHeader(
        const Camera *camera,
        OutputStream *outputStream,
        const RenderOptions *renderOptions);

    static void writeTrailer(OutputStream *outputStream);

  private:
    enum{
        MAXIMUM_CAMERA_STACK = 20
    };
    static const char *const RPK_HOME;

    static Camera cameraStack[MAXIMUM_CAMERA_STACK];

    static Camera *nextSavedCamera(Camera *previous);

    static Matrix4x4 transformModel(
        const Camera *camera,
        Vector3D *modelRotationAxis,
        float *modelRotationAngle);

    static void writeViewPoint(
        OutputStream *outputStream,
        const Matrix4x4 *modelTransform,
        const Camera *camera,
        const char *viewPointName);

    static void writeViewPoints(
        const Camera *camera,
        OutputStream *outputStream,
        const Matrix4x4 *modelTransform);

    static void writeFormatted(OutputStream *outputStream, const char *format, ...);
};

#endif
