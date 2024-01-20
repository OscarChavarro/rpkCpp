/**
Saves the result of a radiosity computation as a VRML file
*/

#include "scene/scene.h"
#include "shared/defaults.h"
#include "shared/render.h"
#include "shared/writevrml.h"

/**
Compute a rotation that will rotate the current "up"-direction to the Y axis.
Y-axis positions up in VRML2.0
*/
Matrix4x4
transformModelVRML(Vector3D *model_rotaxis, float *model_rotangle) {
    Vector3D up_axis;
    double cosA;

    VECTORSET(up_axis, 0.0, 1.0, 0.0);
    cosA = VECTORDOTPRODUCT(GLOBAL_camera_mainCamera.upDirection, up_axis);
    if ( cosA < 1. - EPSILON ) {
        *model_rotangle = (float)acos(cosA);
        VECTORCROSSPRODUCT(GLOBAL_camera_mainCamera.upDirection, up_axis, *model_rotaxis);
        VECTORNORMALIZE(*model_rotaxis);
        return Rotate(*model_rotangle, *model_rotaxis);
    } else {
        VECTORSET(*model_rotaxis, 0., 1., 0.);
        *model_rotangle = 0.;
        return GLOBAL_matrix_identityTransform4x4;
    }
}

/**
Write VRML ViewPoint node for the given camera position
*/
void
writeVRMLViewPoint(FILE *fp, Matrix4x4 model_xf, Camera *cam, const char *vpname) {
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
    TRANSFORM_VECTOR_3D(model_xf, X, X);
    TRANSFORM_VECTOR_3D(model_xf, Y, Y);
    TRANSFORM_VECTOR_3D(model_xf, Z, Z);

    // Construct view orientation transform and recover axis and angle
    viewTransform = GLOBAL_matrix_identityTransform4x4;
    SET_3X3MATRIX(viewTransform.m,
                  X.x, Y.x, Z.x,
                  X.y, Y.y, Z.y,
                  X.z, Y.z, Z.z);
    RecoverRotation(viewTransform, &viewRotationAngle, &viewRotationAxis);

    // Apply model transform to eye point
    TRANSFORM_POINT_3D(model_xf, cam->eyePosition, eyePosition);

    fprintf(fp,
            "Viewpoint {\n  position %g %g %g\n  orientation %g %g %g %g\n  fieldOfView %g\n  description \"%s\"\n}\n\n",
            eyePosition.x, eyePosition.y, eyePosition.z,
            viewRotationAxis.x, viewRotationAxis.y, viewRotationAxis.z, viewRotationAngle,
            (double) (2. * cam->fov * M_PI / 180.),
            vpname);
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
writeVRMLResetVertexId() {
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        for ( int i = 0; i < window->patch->numberOfVertices; i++ ) {
            window->patch->vertex[i]->tmp = -1;
        }
    }
}

static void
writeVRMLCoords(FILE *fp) {
    int id = 0, n = 0;

    writeVRMLResetVertexId();

    fprintf(fp, "\tcoord Coordinate {\n\t  point [ ");
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        for ( int i = 0; i < window->patch->numberOfVertices; i++ ) {
            Vertex *v = window->patch->vertex[i];
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
writeVRMLVertexColors(FILE *fp) {
    int id = 0;
    int n = 0;

    writeVRMLResetVertexId();

    fprintf(fp, "\tcolor Color {\n\t  color [ ");
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        int i;
        for ( i = 0; i < window->patch->numberOfVertices; i++ ) {
            Vertex *v = window->patch->vertex[i];
            if ( v->tmp == -1 ) {
                /* not yet written */
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
writeVRMLFaceColors(FILE *fp) {
    int n = 0;

    fprintf(fp, "\tcolor Color {\n\t  color [ ");
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        if ( n > 0 ) {
            fprintf(fp, ", ");
        }
        n++;
        if ( n % 4 == 0 ) {
            fprintf(fp, "\n\t  ");
        }
        fprintf(fp, "%.3g %.3g %.3g", window->patch->color.r, window->patch->color.g, window->patch->color.b);
    }
    fprintf(fp, " ] ");
    fprintf(fp, "\n\t}\n");
}

static void
writeVRMLColors(FILE *fp) {
    fprintf(fp, "\tcolorPerVertex %s\n", GLOBAL_render_renderOptions.smooth_shading ? "TRUE" : "FALSE");
    if ( GLOBAL_render_renderOptions.smooth_shading ) {
        writeVRMLVertexColors(fp);
    } else {
        writeVRMLFaceColors(fp);
    }
}

static void
writeVRMLCoordIndices(FILE *fp) {
    int n = 0;
    fprintf(fp, "\tcoordIndex [ ");
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        int i;
        for ( i = 0; i < window->patch->numberOfVertices; i++ ) {
            Vertex *v = window->patch->vertex[i];
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
writeVRML(FILE *fp) {
    writeVrmlHeader(fp);

    writeVRMLCoords(fp);
    writeVRMLColors(fp);
    writeVRMLCoordIndices(fp);

    writeVRMLTrailer(fp);
}
