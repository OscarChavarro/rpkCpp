/**
Saves the result of a radiosity computation as a VRML file
*/

#include "java/util/ArrayList.txx"
#include "common/options.h"
#include "io/writevrml.h"

// Camera position etc. can be saved on a stack of size MAXIMUM_CAMERA_STACK
#define MAXIMUM_CAMERA_STACK 20

static const char* RPKHOME = "http://www.cs.kuleuven.ac.be/cwis/research/graphics/RENDERPARK/";

// A stack of virtual camera positions, used for temporary saving the camera and
// later restoring
static Camera globalCameraStack[MAXIMUM_CAMERA_STACK];
static Camera *globalCameraStackPtr = globalCameraStack;

/**
Returns pointer to the next saved camera. If previous==nullptr, the first saved
camera is returned. In subsequent calls, the previous camera returned
by this function should be passed as the parameter. If all saved cameras
have been iterated over, nullptr is returned
*/
static Camera *
nextSavedCamera(Camera *previous) {
    Camera *cam = previous ? previous : globalCameraStackPtr;
    cam--;
    return (cam < globalCameraStack) ? nullptr : cam;
}

/**
Compute a rotation that will rotate the current "up"-direction to the Y axis.
Y-axis positions up in VRML2.0
*/
Matrix4x4
transformModelVRML(const Camera *camera, Vector3D *modelRotationAxis, float *modelRotationAngle) {
    Vector3D upAxis;
    double cosA;

    upAxis.set(0.0, 1.0, 0.0);
    cosA = camera->upDirection.dotProduct(upAxis);
    if ( cosA < 1.0 - EPSILON ) {
        *modelRotationAngle = (float)java::Math::acos(cosA);
        modelRotationAxis->crossProduct(camera->upDirection, upAxis);
        modelRotationAxis->normalize(EPSILON_FLOAT);
        return Matrix4x4::createRotationMatrix(*modelRotationAngle, *modelRotationAxis);
    } else {
        modelRotationAxis->set(0.0, 1.0, 0.0);
        *modelRotationAngle = 0.0;
        Matrix4x4 identity;
        return identity;
    }
}

/**
Write VRML ViewPoint node for the given camera position
*/
static void
writeVRMLViewPoint(FILE *fp, const Matrix4x4 *modelTransform, const Camera *camera, const char *viewPointName) {
    Vector3D X;
    Vector3D Y;
    Vector3D Z;
    Vector3D viewRotationAxis;
    Vector3D eyePosition;
    Matrix4x4 viewTransform;
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
    viewTransform = identity;
    viewTransform.set3X3Matrix(
        X.x, Y.x, Z.x,
        X.y, Y.y, Z.y,
        X.z, Y.z, Z.z);
    viewTransform.recoverRotationParameters(&viewRotationAngle, &viewRotationAxis);

    // Apply model transform to eye point
    modelTransform->transformPoint3D(camera->eyePosition, eyePosition);

    fprintf(fp,
            "Viewpoint {\n  position %g %g %g\n  orientation %g %g %g %g\n  fieldOfView %g\n  description \"%s\"\n}\n\n",
            eyePosition.x, eyePosition.y, eyePosition.z,
            viewRotationAxis.x, viewRotationAxis.y, viewRotationAxis.z, viewRotationAngle,
            2.0 * camera->fieldOfVision * M_PI / 180.0,
            viewPointName);
}

static void
writeVRMLViewPoints(const Camera *camera, FILE *fp, const Matrix4x4 *modelTransform) {
    Camera *localCamera = nullptr;
    int count = 1;
    writeVRMLViewPoint(fp, modelTransform, camera, "ViewPoint 1");
    while ( (localCamera = nextSavedCamera(localCamera)) != nullptr ) {
        char viewPointName[21];
        count++;
        snprintf(viewPointName, 21, "ViewPoint %d", count);
        writeVRMLViewPoint(fp, modelTransform, localCamera, viewPointName);
    }
}

/**
Can also be used by radiance-method specific VRML savers.
*/
void
writeVrmlHeader(const Camera *camera, FILE *fp, const RenderOptions *renderOptions) {
    Matrix4x4 modelTransform{};
    Vector3D modelRotationAxis;
    float modelRotationAngle;

    fprintf(fp, "#VRML V2.0 utf8\n\n");

    fprintf(fp, "WorldInfo {\n  title \"%s\"\n  info [ \"Created using RenderPark (%s)\" ]\n}\n\n",
            "Some nice model",
            RPKHOME);

    fprintf(fp, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    modelTransform = transformModelVRML(camera, &modelRotationAxis, &modelRotationAngle);
    writeVRMLViewPoints(camera, fp, &modelTransform);

    fprintf(fp, "Transform {\n  rotation %g %g %g %g\n  children [\n    Shape {\n      geometry IndexedFaceSet {\n",
            modelRotationAxis.x, modelRotationAxis.y, modelRotationAxis.z, modelRotationAngle);

    fprintf(fp, "\tsolid %s\n", renderOptions->backfaceCulling ? "TRUE" : "FALSE");
}

void
writeVRMLTrailer(FILE *fp) {
    fprintf(fp, "      }\n    }\n  ]\n}\n\n");
}
