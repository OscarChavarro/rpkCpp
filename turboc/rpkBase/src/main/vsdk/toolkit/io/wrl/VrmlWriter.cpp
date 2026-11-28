/**
Saves the result of a radiosity computation as a VRML file
*/

#include <stdarg.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "java/util/Formatter.h"
#include "vsdk/toolkit/io/PersistenceElement.h"
#include "vsdk/toolkit/io/wrl/VrmlWriter.h"

#ifndef va_copy
#if defined(__va_copy)
#define va_copy(dst, src) __va_copy((dst), (src))
#else
#define va_copy(dst, src) ((dst) = (src))
#endif
#endif

const char *const VrmlWriter::RPK_HOME = "http://www.cs.kuleuven.ac.be/cwis/research/graphics/RENDERPARK/";
Camera VrmlWriter::cameraStack[VrmlWriter::MAXIMUM_CAMERA_STACK];

void
VrmlWriter::writeFormatted(OutputStream *outputStream, const char *format, ...) {
    if ( outputStream == NULL || format == NULL ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = Formatter::vformat(localBuffer, ((int)(sizeof(localBuffer))), format, argumentsCopy);
    va_end(argumentsCopy);

    String text;
    if ( required >= 0 ) {
        if ( required < ((int)(sizeof(localBuffer))) ) {
            text = String(localBuffer);
        } else {
            char *dynamicBuffer = new char[required + 1];
            va_copy(argumentsCopy, arguments);
            Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
            va_end(argumentsCopy);
            text = String(dynamicBuffer);
            delete[] dynamicBuffer;
        }
    }
    va_end(arguments);

    if ( text.isEmpty() ) {
        return;
    }

    PersistenceElement::writeBytes(
        *outputStream,
        ((const unsigned char *)(text.toCString())),
        text.length());
}

/**
Returns pointer to the next saved camera. If previous==NULL, the first saved
camera is returned. In subsequent calls, the previous camera returned
by this function should be passed as the parameter. If all saved cameras
have been iterated over, NULL is returned
*/
Camera *
VrmlWriter::nextSavedCamera(Camera *previous) {
    Camera *cam = previous ? previous : cameraStack;
    cam--;
    return (cam < cameraStack) ? NULL : cam;
}

/**
Compute a rotation that will rotate the current "up"-direction to the Y axis.
Y-axis positions up in VRML2.0
*/
Matrix4x4d
VrmlWriter::transformModel(const Camera *camera, Vector3Dd *modelRotationAxis, double *modelRotationAngle) {
    Vector3Dd upAxis;

    upAxis = Vector3Dd(0.0, 1.0, 0.0);
    const double cosA = camera->upDirection.dotProduct(upAxis);
    if ( cosA < 1.0 - VSDK::EPSILON ) {
        *modelRotationAngle = ((double)(Math::acos(cosA)));
        *modelRotationAxis = camera->upDirection.crossProduct(upAxis);
        *modelRotationAxis = modelRotationAxis->normalized(Numeric::EPSILON);
        return Matrix4x4d().axisRotation(*modelRotationAngle, *modelRotationAxis);
    }

    *modelRotationAxis = Vector3Dd(0.0, 1.0, 0.0);
    *modelRotationAngle = 0.0;
    Matrix4x4d identity;
    return identity;
}

/**
Recovers the rotation axis and angle from the given rotation matrix.
There is no check whether the transform really is a rotation.
*/
void
VrmlWriter::recoverRotationParameters(const Matrix4x4d &rotation, double *angle, Vector3Dd *axis) {
    const double c = (rotation.get(0, 0) + rotation.get(1, 1) + rotation.get(2, 2) - 1.0) * 0.5;
    if ( c > 1.0 - VSDK::EPSILON ) {
        *angle = 0.0;
        *axis = Vector3Dd(0.0, 0.0, 1.0);
    } else if ( c < -1.0 + VSDK::EPSILON ) {
        *angle = ((double)(M_PI));
        double x = Math::sqrt((rotation.get(0, 0) + 1.0) * 0.5);
        double y = Math::sqrt((rotation.get(1, 1) + 1.0) * 0.5);
        double z = Math::sqrt((rotation.get(2, 2) + 1.0) * 0.5);

        // Assume x positive, determine sign of y and z
        if ( rotation.get(1, 0) < 0.0 ) {
            y = -y;
        }
        if ( rotation.get(2, 0) < 0.0 ) {
            z = -z;
        }
        *axis = Vector3Dd(x, y, z);
    } else {
        *angle = Math::acos(c);
        const double s = Math::sqrt(1.0 - c * c);
        const double r = 1.0 / (2.0 * s);
        *axis = Vector3Dd(
            (rotation.get(2, 1) - rotation.get(1, 2)) * r,
            (rotation.get(0, 2) - rotation.get(2, 0)) * r,
            (rotation.get(1, 0) - rotation.get(0, 1)) * r);
    }
}

/**
Write VRML ViewPoint node for the given camera position
*/
void
VrmlWriter::writeViewPoint(
    OutputStream *outputStream,
    const Matrix4x4d *modelTransform,
    const Camera *camera,
    const char *viewPointName)
{
    Vector3Dd X;
    Vector3Dd Y;
    Vector3Dd Z;
    Vector3Dd viewRotationAxis;
    Vector3Dd eyePosition;
    double viewRotationAngle;

    X = camera->X.multiply(1.0); // camera->X positions right in window
    Y = camera->Y.multiply(-1.0); // camera->Y positions down in window, VRML wants y up
    Z = camera->Z.multiply(-1.0); // camera->Z positions away, VRML wants Z to point towards viewer

    // Apply model transform (its rotation part)
    const Matrix4x4d modelRotation = modelTransform->withoutTranslation();
    X = modelRotation.multiply(X);
    Y = modelRotation.multiply(Y);
    Z = modelRotation.multiply(Z);

    // Construct view orientation transform and recover axis and angle
    const double orientation[4][4] = {
        {X.x(), Y.x(), Z.x(), 0.0},
        {X.y(), Y.y(), Z.y(), 0.0},
        {X.z(), Y.z(), Z.z(), 0.0},
        {0.0, 0.0, 0.0, 1.0}
    };
    recoverRotationParameters(Matrix4x4d(orientation), &viewRotationAngle, &viewRotationAxis);

    // Apply model transform to eye point
    eyePosition = modelRotation.multiply(camera->eyePosition);

    writeFormatted(
        outputStream,
        "Viewpoint {\n  position %g %g %g\n  orientation %g %g %g %g\n  fieldOfView %g\n  description \"%s\"\n}\n\n",
        eyePosition.x(),
        eyePosition.y(),
        eyePosition.z(),
        viewRotationAxis.x(),
        viewRotationAxis.y(),
        viewRotationAxis.z(),
        viewRotationAngle,
        2.0 * camera->fieldOfVision * Math::PI / 180.0,
        viewPointName);
}

void
VrmlWriter::writeViewPoints(
    const Camera *camera,
    OutputStream *outputStream,
    const Matrix4x4d *modelTransform)
{
    Camera *localCamera = NULL;
    int count = 1;
    writeViewPoint(outputStream, modelTransform, camera, "ViewPoint 1");
    while ( (localCamera = nextSavedCamera(localCamera)) != NULL ) {
        count++;
        char viewPointNameBuffer[32];
        Formatter::format(viewPointNameBuffer, ((int)(sizeof(viewPointNameBuffer))), "ViewPoint %d", count);
        const String viewPointName(viewPointNameBuffer);
        writeViewPoint(outputStream, modelTransform, localCamera, viewPointName.toCString());
    }
}

/**
Can also be used by radiance-method specific VRML savers.
*/
void
VrmlWriter::writeHeader(
    const Camera *camera,
    OutputStream *outputStream,
    const RenderOptions *renderOptions)
{
    Vector3Dd modelRotationAxis;
    double modelRotationAngle;

    writeFormatted(outputStream, "#VRML V2.0 utf8\n\n");

    writeFormatted(
        outputStream,
        "WorldInfo {\n  title \"%s\"\n  info [ \"Created using RenderPark (%s)\" ]\n}\n\n",
        "Some nice model",
        RPK_HOME);

    writeFormatted(outputStream, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    const Matrix4x4d modelTransform = transformModel(camera, &modelRotationAxis, &modelRotationAngle);
    writeViewPoints(camera, outputStream, &modelTransform);

    writeFormatted(
        outputStream,
        "Transform {\n  rotation %g %g %g %g\n  children [\n    Shape {\n      geometry IndexedFaceSet {\n",
        modelRotationAxis.x(),
        modelRotationAxis.y(),
        modelRotationAxis.z(),
        modelRotationAngle);

    writeFormatted(outputStream, "\tsolid %s\n", renderOptions->isBackfaceCulling() ? "TRUE" : "FALSE");
}

void
VrmlWriter::writeTrailer(OutputStream *outputStream) {
    writeFormatted(outputStream, "      }\n    }\n  ]\n}\n\n");
}
