/**
Saves the result of a radiosity computation as a VRML file
*/

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/util/Formatter.h"
#include "vsdk/toolkit/io/wrapper/PersistenceElement.h"
#include "vsdk/toolkit/io/wrl/VrmlWriter.h"

const char *const VrmlWriter::RPK_HOME = "http://www.cs.kuleuven.ac.be/cwis/research/graphics/RENDERPARK/";
Camera VrmlWriter::cameraStack[VrmlWriter::MAXIMUM_CAMERA_STACK];

void
VrmlWriter::writeFormatted(java::OutputStream *outputStream, const char *format, ...) {
    if ( outputStream == nullptr || format == nullptr ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = java::Formatter::vformat(localBuffer, static_cast<int>(sizeof(localBuffer)), format, argumentsCopy);
    va_end(argumentsCopy);

    java::String text;
    if ( required >= 0 ) {
        if ( required < static_cast<int>(sizeof(localBuffer)) ) {
            text = java::String(localBuffer);
        } else {
            char *dynamicBuffer = new char[required + 1];
            va_copy(argumentsCopy, arguments);
            java::Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
            va_end(argumentsCopy);
            text = java::String(dynamicBuffer);
            delete[] dynamicBuffer;
        }
    }
    va_end(arguments);

    if ( text.isEmpty() ) {
        return;
    }

    PersistenceElement::writeBytes(
        *outputStream,
        reinterpret_cast<const unsigned char *>(text.toCString()),
        text.length());
}

/**
Returns pointer to the next saved camera. If previous==nullptr, the first saved
camera is returned. In subsequent calls, the previous camera returned
by this function should be passed as the parameter. If all saved cameras
have been iterated over, nullptr is returned
*/
Camera *
VrmlWriter::nextSavedCamera(Camera *previous) {
    Camera *cam = previous ? previous : cameraStack;
    cam--;
    return (cam < cameraStack) ? nullptr : cam;
}

/**
Compute a rotation that will rotate the current "up"-direction to the Y axis.
Y-axis positions up in VRML2.0
*/
Matrix4x4f
VrmlWriter::transformModel(const Camera *camera, Vector3Df *modelRotationAxis, float *modelRotationAngle) {
    Vector3Df upAxis;

    upAxis = Vector3Df(0.0, 1.0, 0.0);
    const double cosA = camera->upDirection.dotProduct(upAxis);
    if ( cosA < 1.0 - VSDK::EPSILON ) {
        *modelRotationAngle = static_cast<float>(java::Math::acos(cosA));
        *modelRotationAxis = camera->upDirection.crossProduct(upAxis);
        *modelRotationAxis = modelRotationAxis->normalized(Numeric::EPSILON_FLOAT);
        return Matrix4x4f().axisRotation(*modelRotationAngle, *modelRotationAxis);
    }

    *modelRotationAxis = Vector3Df(0.0, 1.0, 0.0);
    *modelRotationAngle = 0.0;
    Matrix4x4f identity;
    return identity;
}

/**
Recovers the rotation axis and angle from the given rotation matrix.
There is no check whether the transform really is a rotation.
*/
void
VrmlWriter::recoverRotationParameters(const Matrix4x4f &rotation, float *angle, Vector3Df *axis) {
    const float c = (rotation.get(0, 0) + rotation.get(1, 1) + rotation.get(2, 2) - 1.0F) * 0.5F;
    if ( c > 1.0F - VSDK::EPSILON ) {
        *angle = 0.0F;
        *axis = Vector3Df(0.0F, 0.0F, 1.0F);
    } else if ( c < -1.0F + VSDK::EPSILON ) {
        *angle = static_cast<float>(M_PI);
        float x = java::Math::sqrt((rotation.get(0, 0) + 1.0F) * 0.5F);
        float y = java::Math::sqrt((rotation.get(1, 1) + 1.0F) * 0.5F);
        float z = java::Math::sqrt((rotation.get(2, 2) + 1.0F) * 0.5F);

        // Assume x positive, determine sign of y and z
        if ( rotation.get(1, 0) < 0.0F ) {
            y = -y;
        }
        if ( rotation.get(2, 0) < 0.0F ) {
            z = -z;
        }
        *axis = Vector3Df(x, y, z);
    } else {
        *angle = java::Math::acos(c);
        const float s = java::Math::sqrt(1.0F - c * c);
        const float r = 1.0F / (2.0F * s);
        *axis = Vector3Df(
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
    java::OutputStream *outputStream,
    const Matrix4x4f *modelTransform,
    const Camera *camera,
    const char *viewPointName)
{
    Vector3Df X;
    Vector3Df Y;
    Vector3Df Z;
    Vector3Df viewRotationAxis;
    Vector3Df eyePosition;
    float viewRotationAngle;

    X = camera->X.multiply(1.0); // camera->X positions right in window
    Y = camera->Y.multiply(-1.0); // camera->Y positions down in window, VRML wants y up
    Z = camera->Z.multiply(-1.0); // camera->Z positions away, VRML wants Z to point towards viewer

    // Apply model transform (its rotation part)
    const Matrix4x4f modelRotation = modelTransform->withoutTranslation();
    X = modelRotation.multiply(X);
    Y = modelRotation.multiply(Y);
    Z = modelRotation.multiply(Z);

    // Construct view orientation transform and recover axis and angle
    const float orientation[4][4] = {
        {X.x(), Y.x(), Z.x(), 0.0F},
        {X.y(), Y.y(), Z.y(), 0.0F},
        {X.z(), Y.z(), Z.z(), 0.0F},
        {0.0F, 0.0F, 0.0F, 1.0F}
    };
    recoverRotationParameters(Matrix4x4f(orientation), &viewRotationAngle, &viewRotationAxis);

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
        2.0 * camera->fieldOfVision * java::Math::PI / 180.0,
        viewPointName);
}

void
VrmlWriter::writeViewPoints(
    const Camera *camera,
    java::OutputStream *outputStream,
    const Matrix4x4f *modelTransform)
{
    Camera *localCamera = nullptr;
    int count = 1;
    writeViewPoint(outputStream, modelTransform, camera, "ViewPoint 1");
    while ( (localCamera = nextSavedCamera(localCamera)) != nullptr ) {
        count++;
        char viewPointNameBuffer[32];
        java::Formatter::format(viewPointNameBuffer, static_cast<int>(sizeof(viewPointNameBuffer)), "ViewPoint %d", count);
        const java::String viewPointName(viewPointNameBuffer);
        writeViewPoint(outputStream, modelTransform, localCamera, viewPointName.toCString());
    }
}

/**
Can also be used by radiance-method specific VRML savers.
*/
void
VrmlWriter::writeHeader(
    const Camera *camera,
    java::OutputStream *outputStream,
    const RendererConfiguration *renderOptions)
{
    Vector3Df modelRotationAxis;
    float modelRotationAngle;

    writeFormatted(outputStream, "#VRML V2.0 utf8\n\n");

    writeFormatted(
        outputStream,
        "WorldInfo {\n  title \"%s\"\n  info [ \"Created using RenderPark (%s)\" ]\n}\n\n",
        "Some nice model",
        RPK_HOME);

    writeFormatted(outputStream, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    const Matrix4x4f modelTransform = transformModel(camera, &modelRotationAxis, &modelRotationAngle);
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
VrmlWriter::writeTrailer(java::OutputStream *outputStream) {
    writeFormatted(outputStream, "      }\n    }\n  ]\n}\n\n");
}
