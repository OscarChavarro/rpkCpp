/**
Saves the result of a radiosity computation as a VRML file
*/

#include "java/util/ArrayList.txx"
#include "shared/render.h"
#include "shared/writevrml.h"

/**
Compute a rotation that will rotate the current "up"-direction to the Y axis.
Y-axis positions up in VRML2.0
*/
Matrix4x4
transformModelVRML(Vector3D *modelRotationAxis, float *modelRotationAngle) {
    Vector3D up_axis;
    double cosA;

    VECTORSET(up_axis, 0.0, 1.0, 0.0);
    cosA = VECTORDOTPRODUCT(GLOBAL_camera_mainCamera.upDirection, up_axis);
    if ( cosA < 1. - EPSILON ) {
        *modelRotationAngle = (float)acos(cosA);
        VECTORCROSSPRODUCT(GLOBAL_camera_mainCamera.upDirection, up_axis, *modelRotationAxis);
        VECTORNORMALIZE(*modelRotationAxis);
        return rotateMatrix(*modelRotationAngle, *modelRotationAxis);
    } else {
        VECTORSET(*modelRotationAxis, 0., 1., 0.);
        *modelRotationAngle = 0.;
        return GLOBAL_matrix_identityTransform4x4;
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

    VECTORSCALE(1.0, cam->X, X); // cam->X positions right in window
    VECTORSCALE(-1.0, cam->Y, Y); // cam->Y positions down in window, VRML wants y up
    VECTORSCALE(-1.0, cam->Z, Z); // cam->Z positions away, VRML wants Z to point towards viewer

    // Apply model transform
    transformPoint3D(model_xf, X, X);
    transformPoint3D(model_xf, Y, Y);
    transformPoint3D(model_xf, Z, Z);

    // Construct view orientation transform and recover axis and angle
    viewTransform = GLOBAL_matrix_identityTransform4x4;
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
            (double) (2. * cam->fov * M_PI / 180.),
            viewPointName);
}

void
writeVRMLViewPoints(FILE *fp, Matrix4x4 model_xf) {
    Camera *cam = (Camera *) nullptr;
    int count = 1;
    writeVRMLViewPoint(fp, model_xf, &GLOBAL_camera_mainCamera, "ViewPoint 1");
    while ((cam = nextSavedCamera(cam)) != (Camera *) nullptr) {
        char vpname[21];
        count++;
        snprintf(vpname, 21, "ViewPoint %d", count);
        writeVRMLViewPoint(fp, model_xf, cam, vpname);
    }
}

/**
Can also be used by radiance-method specific VRML savers.
*/
void
writeVrmlHeader(FILE *fp) {
    Matrix4x4 modelTransform{};
    Vector3D modelRotationAxis;
    float modelRotationAngle;

    fprintf(fp, "#VRML V2.0 utf8\n\n");

    fprintf(fp, "WorldInfo {\n  title \"%s\"\n  info [ \"Created using RenderPark (%s)\" ]\n}\n\n",
            "Some nice model",
            RPKHOME);

    fprintf(fp, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    modelTransform = transformModelVRML(&modelRotationAxis, &modelRotationAngle);
    writeVRMLViewPoints(fp, modelTransform);

    fprintf(fp, "Transform {\n  rotation %g %g %g %g\n  children [\n    Shape {\n      geometry IndexedFaceSet {\n",
            modelRotationAxis.x, modelRotationAxis.y, modelRotationAxis.z, modelRotationAngle);

    fprintf(fp, "\tsolid %s\n", GLOBAL_render_renderOptions.backface_culling ? "TRUE" : "FALSE");
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
    fprintf(fp, "\tcolorPerVertex %s\n", GLOBAL_render_renderOptions.smooth_shading ? "TRUE" : "FALSE");
    if ( GLOBAL_render_renderOptions.smooth_shading ) {
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
writeVRML(FILE *fp, java::ArrayList<Patch *> *scenePatches) {
    writeVrmlHeader(fp);

    writeVRMLCoords(fp, scenePatches);
    writeVRMLColors(fp, scenePatches);
    writeVRMLCoordIndices(fp, scenePatches);

    writeVRMLTrailer(fp);
}
