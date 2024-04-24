/**
Saves the result of a radiosity computation as a VRML file
*/

#include "java/util/ArrayList.txx"
#include "common/linealAlgebra/Matrix4x4.h"
#include "common/options.h"
#include "common/RenderOptions.h"
#include "skin/Patch.h"
#include "io/writevrml.h"

static Matrix4x4 globalIdentityMatrix = {
    {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }
};

// Camera position etc. can be saved on a stack of size MAXIMUM_CAMERA_STACK
#define MAXIMUM_CAMERA_STACK 20

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
transformModelVRML(Camera *camera, Vector3D *modelRotationAxis, float *modelRotationAngle) {
    Vector3D upAxis;
    double cosA;

    upAxis.set(0.0, 1.0, 0.0);
    cosA = vectorDotProduct(camera->upDirection, upAxis);
    if ( cosA < 1.0 - EPSILON ) {
        *modelRotationAngle = (float)std::acos(cosA);
        vectorCrossProduct(camera->upDirection, upAxis, *modelRotationAxis);
        vectorNormalize(*modelRotationAxis);
        return createRotationMatrix(*modelRotationAngle, *modelRotationAxis);
    } else {
        modelRotationAxis->set(0.0, 1.0, 0.0);
        *modelRotationAngle = 0.0;
        return globalIdentityMatrix;
    }
}

/**
Write VRML ViewPoint node for the given camera position
*/
static void
writeVRMLViewPoint(FILE *fp, Matrix4x4 model_xf, Camera *cam, const char *viewPointName) {
    Vector3D X;
    Vector3D Y;
    Vector3D Z;
    Vector3D viewRotationAxis;
    Vector3D eyePosition;
    Matrix4x4 viewTransform{};
    float viewRotationAngle;

    vectorScale(1.0, cam->X, X); // cam->X positions right in window
    vectorScale(-1.0, cam->Y, Y); // cam->Y positions down in window, VRML wants y up
    vectorScale(-1.0, cam->Z, Z); // cam->Z positions away, VRML wants Z to point towards viewer

    // Apply model transform
    transformPoint3D(model_xf, X, X);
    transformPoint3D(model_xf, Y, Y);
    transformPoint3D(model_xf, Z, Z);

    // Construct view orientation transform and recover axis and angle
    viewTransform = globalIdentityMatrix;
    set3X3Matrix(viewTransform.m,
                 X.x, Y.x, Z.x,
                 X.y, Y.y, Z.y,
                 X.z, Y.z, Z.z);
    recoverRotationMatrix(viewTransform, &viewRotationAngle, &viewRotationAxis);

    // Apply model transform to eye point
    transformPoint3D(model_xf, cam->eyePosition, eyePosition);

    fprintf(fp,
            "Viewpoint {\n  position %g %g %g\n  orientation %g %g %g %g\n  fieldOfView %g\n  description \"%s\"\n}\n\n",
            eyePosition.x, eyePosition.y, eyePosition.z,
            viewRotationAxis.x, viewRotationAxis.y, viewRotationAxis.z, viewRotationAngle,
            (double) (2.0 * cam->fieldOfVision * M_PI / 180.0),
            viewPointName);
}

void
writeVRMLViewPoints(Camera *camera, FILE *fp, Matrix4x4 model_xf) {
    Camera *localCamera = nullptr;
    int count = 1;
    writeVRMLViewPoint(fp, model_xf, camera, "ViewPoint 1");
    while ( (localCamera = nextSavedCamera(localCamera)) != nullptr ) {
        char viewPointName[21];
        count++;
        snprintf(viewPointName, 21, "ViewPoint %d", count);
        writeVRMLViewPoint(fp, model_xf, localCamera, viewPointName);
    }
}

/**
Can also be used by radiance-method specific VRML savers.
*/
void
writeVrmlHeader(Camera *camera, FILE *fp, RenderOptions *renderOptions) {
    Matrix4x4 modelTransform{};
    Vector3D modelRotationAxis;
    float modelRotationAngle;

    fprintf(fp, "#VRML V2.0 utf8\n\n");

    fprintf(fp, "WorldInfo {\n  title \"%s\"\n  info [ \"Created using RenderPark (%s)\" ]\n}\n\n",
            "Some nice model",
            RPKHOME);

    fprintf(fp, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    modelTransform = transformModelVRML(camera, &modelRotationAxis, &modelRotationAngle);
    writeVRMLViewPoints(camera, fp, modelTransform);

    fprintf(fp, "Transform {\n  rotation %g %g %g %g\n  children [\n    Shape {\n      geometry IndexedFaceSet {\n",
            modelRotationAxis.x, modelRotationAxis.y, modelRotationAxis.z, modelRotationAngle);

    fprintf(fp, "\tsolid %s\n", GLOBAL_render_renderOptions.backfaceCulling ? "TRUE" : "FALSE");
}

void
writeVRMLTrailer(FILE *fp) {
    fprintf(fp, "      }\n    }\n  ]\n}\n\n");
}

static void
writeVRMLResetVertexId(java::ArrayList<Patch *> *scenePatches) {
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        for ( int j = 0; j < patch->numberOfVertices; j++ ) {
            patch->vertex[j]->tmp = -1;
        }
    }
}

static void
writeVRMLCoords(FILE *fp, java::ArrayList<Patch *> *scenePatches) {
    int id = 0, n = 0;

    writeVRMLResetVertexId(scenePatches);

    fprintf(fp, "\tcoord Coordinate {\n\t  point [ ");
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        for ( int j = 0; j < patch->numberOfVertices; j++ ) {
            Vertex *v = patch->vertex[j];
            if ( v->tmp == -1 ) {
                // Not yet written
                if ( n > 0 ) {
                    fprintf(fp, ", ");
                }
                n++;
                if ( n % 4 == 0 ) {
                    fprintf(fp, "\n\t  ");
                }
                fprintf(fp, "%g %g %g", v->point->x, v->point->y, v->point->z);
                v->tmp = id;
                id++;
            }
        }
    }
    fprintf(fp, " ] ");
    fprintf(fp, "\n\t}\n");
}

static void
writeVRMLVertexColors(FILE *fp, java::ArrayList<Patch *> *scenePatches) {
    int id = 0;
    int n = 0;

    writeVRMLResetVertexId(scenePatches);

    fprintf(fp, "\tcolor Color {\n\t  color [ ");
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch * patch = scenePatches->get(i);
        for ( int j = 0; j < patch->numberOfVertices; j++ ) {
            Vertex *v = patch->vertex[j];
            if ( v->tmp == -1 ) {
                // Not yet written
                if ( n > 0 ) {
                    fprintf(fp, ", ");
                }
                n++;
                if ( n % 4 == 0 ) {
                    fprintf(fp, "\n\t  ");
                }
                fprintf(fp, "%.3g %.3g %.3g", v->color.r, v->color.g, v->color.b);
                v->tmp = id;
                id++;
            }
        }
    }
    fprintf(fp, " ] ");
    fprintf(fp, "\n\t}\n");
}

static void
writeVRMLFaceColors(FILE *fp, java::ArrayList<Patch *> *scenePatches) {
    int n = 0;

    fprintf(fp, "\tcolor Color {\n\t  color [ ");
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        if ( n > 0 ) {
            fprintf(fp, ", ");
        }
        n++;
        if ( n % 4 == 0 ) {
            fprintf(fp, "\n\t  ");
        }
        fprintf(fp, "%.3g %.3g %.3g", patch->color.r, patch->color.g, patch->color.b);
    }
    fprintf(fp, " ] ");
    fprintf(fp, "\n\t}\n");
}

static void
writeVRMLColors(FILE *fp, java::ArrayList<Patch *> *scenePatches) {
    fprintf(fp, "\tcolorPerVertex %s\n", GLOBAL_render_renderOptions.smoothShading ? "TRUE" : "FALSE");
    if ( GLOBAL_render_renderOptions.smoothShading ) {
        writeVRMLVertexColors(fp, scenePatches);
    } else {
        writeVRMLFaceColors(fp, scenePatches);
    }
}

static void
writeVRMLCoordIndices(FILE *fp, java::ArrayList<Patch *> *scenePatches) {
    int n = 0;
    fprintf(fp, "\tcoordIndex [ ");
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        for ( int j = 0; j < patch->numberOfVertices; j++ ) {
            Vertex *v = patch->vertex[j];
            n++;
            if ( n % 20 == 0 ) {
                fprintf(fp, "\n\t  ");
            }
            fprintf(fp, "%d ", v->tmp);
        }
        n++;
        if ( n % 20 == 0 ) {
            fprintf(fp, "\n\t  ");
        }
        fprintf(fp, "-1 ");
    }
    fprintf(fp, " ]\n");
}

/**
Default method for saving VRML models (if the current radiance method
doesn't have its own one
*/
void
writeVRML(Camera *camera, FILE *fp, java::ArrayList<Patch *> *scenePatches, RenderOptions *renderOptions) {
    writeVrmlHeader(camera, fp, renderOptions);

    writeVRMLCoords(fp, scenePatches);
    writeVRMLColors(fp, scenePatches);
    writeVRMLCoordIndices(fp, scenePatches);

    writeVRMLTrailer(fp);
}
