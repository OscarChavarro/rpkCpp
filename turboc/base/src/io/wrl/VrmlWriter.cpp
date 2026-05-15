/**
Saves the result of a radiosity computation as a VRML file
*/

#include "java/util/Formatter.h"
#include "io/wrapper/PersistenceElement.h"
#include "io/wrl/VrmlWriter.h"

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
Matrix4x4
VrmlWriter::transformModel(const Camera *camera, Vector3D *modelRotationAxis, float *modelRotationAngle) {
    Vector3D upAxis;

    upAxis.set(0.0, 1.0, 0.0);
    const double cosA = camera->upDirection.dotProduct(upAxis);
    if ( cosA < 1.0 - Numeric::EPSILON ) {
        *modelRotationAngle = ((float)(Math::acos(cosA)));
        modelRotationAxis->crossProduct(camera->upDirection, upAxis);
        modelRotationAxis->normalize(Numeric::EPSILON_FLOAT);
        return Matrix4x4::createRotationMatrix(*modelRotationAngle, *modelRotationAxis);
    }

    modelRotationAxis->set(0.0, 1.0, 0.0);
    *modelRotationAngle = 0.0;
    Matrix4x4 identity;
    return identity;
}

/**
Write VRML ViewPoint node for the given camera position
*/
void
VrmlWriter::writeViewPoint(
    OutputStream *outputStream,
    const Matrix4x4 *modelTransform,
    const Camera *camera,
    const char *viewPointName)
{
    Vector3D X;
    Vector3D Y;
    Vector3D Z;
    Vector3D viewRotationAxis;
    Vector3D eyePosition;
    float viewRotationAngle;

    X.scaledCopy(1.0, camera->X); // camera->X positions right in window
    Y.scaledCopy(-1.0, camera->Y); // camera->Y positions down in window, VRML wants y up
    Z.scaledCopy(-1.0, camera->Z); // camera->Z positions away, VRML wants Z to point towards viewer

    // Apply model transform
    modelTransform->transformPoint3D(X, X);
    modelTransform->transformPoint3D(Y, Y);
    modelTransform->transformPoint3D(Z, Z);

    // Construct view orientation transform and recover axis and angle
    Matrix4x4 identity;
    Matrix4x4 viewTransform = identity;
    viewTransform.set3X3Matrix(
        X.x, Y.x, Z.x,
        X.y, Y.y, Z.y,
        X.z, Y.z, Z.z);
    viewTransform.recoverRotationParameters(&viewRotationAngle, &viewRotationAxis);

    // Apply model transform to eye point
    modelTransform->transformPoint3D(camera->eyePosition, eyePosition);

    writeFormatted(
        outputStream,
        "Viewpoint {\n  position %g %g %g\n  orientation %g %g %g %g\n  fieldOfView %g\n  description \"%s\"\n}\n\n",
        eyePosition.x,
        eyePosition.y,
        eyePosition.z,
        viewRotationAxis.x,
        viewRotationAxis.y,
        viewRotationAxis.z,
        viewRotationAngle,
        2.0 * camera->fieldOfVision * PI / 180.0,
        viewPointName);
}

void
VrmlWriter::writeViewPoints(
    const Camera *camera,
    OutputStream *outputStream,
    const Matrix4x4 *modelTransform)
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
    Vector3D modelRotationAxis;
    float modelRotationAngle;

    writeFormatted(outputStream, "#VRML V2.0 utf8\n\n");

    writeFormatted(
        outputStream,
        "WorldInfo {\n  title \"%s\"\n  info [ \"Created using RenderPark (%s)\" ]\n}\n\n",
        "Some nice model",
        RPK_HOME);

    writeFormatted(outputStream, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    const Matrix4x4 modelTransform = transformModel(camera, &modelRotationAxis, &modelRotationAngle);
    writeViewPoints(camera, outputStream, &modelTransform);

    writeFormatted(
        outputStream,
        "Transform {\n  rotation %g %g %g %g\n  children [\n    Shape {\n      geometry IndexedFaceSet {\n",
        modelRotationAxis.x,
        modelRotationAxis.y,
        modelRotationAxis.z,
        modelRotationAngle);

    writeFormatted(outputStream, "\tsolid %s\n", renderOptions->isBackfaceCulling() ? "TRUE" : "FALSE");
}

void
VrmlWriter::writeTrailer(OutputStream *outputStream) {
    writeFormatted(outputStream, "      }\n    }\n  ]\n}\n\n");
}
