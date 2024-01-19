/**
Saves the result of a radiosity computation as a VRML file
*/

#include "scene/scene.h"
#include "shared/writevrml.h"
#include "shared/defaults.h"
#include "shared/camera.h"
#include "shared/render.h"

/**
Compute a rotation that will rotate the current "up"-direction to the Y axis.
Y-axis positions up in VRML2.0
*/
Matrix4x4
VRMLModelTransform(Vector3D *model_rotaxis, float *model_rotangle) {
    Vector3D up_axis;
    double cosA;

    VECTORSET(up_axis, 0.0, 1.0, 0.0);
    cosA = VECTORDOTPRODUCT(GLOBAL_camera_mainCamera.updir, up_axis);
    if ( cosA < 1. - EPSILON ) {
        *model_rotangle = (float)acos(cosA);
        VECTORCROSSPRODUCT(GLOBAL_camera_mainCamera.updir, up_axis, *model_rotaxis);
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
WriteVRMLViewPoint(FILE *fp, Matrix4x4 model_xf, CAMERA *cam, const char *vpname) {
    Vector3D X, Y, Z, view_rotaxis, eyep;
    Matrix4x4 view_xf;
    float view_rotangle;

    VECTORSCALE(1., cam->X, X); /* cam->X positions right in window */
    VECTORSCALE(-1., cam->Y, Y); /* cam->Y positions down in window, VRML wants y up */
    VECTORSCALE(-1., cam->Z, Z); /* cam->Z positions away, VRML wants Z to point towards viewer */

    /* apply model transform */
    TRANSFORM_VECTOR_3D(model_xf, X, X);
    TRANSFORM_VECTOR_3D(model_xf, Y, Y);
    TRANSFORM_VECTOR_3D(model_xf, Z, Z);

    /* construct view orientation transform and recover axis and angle */
    view_xf = GLOBAL_matrix_identityTransform4x4;
    SET_3X3MATRIX(view_xf.m,
                  X.x, Y.x, Z.x,
                  X.y, Y.y, Z.y,
                  X.z, Y.z, Z.z);
    RecoverRotation(view_xf, &view_rotangle, &view_rotaxis);

    /* apply model transform to eye point */
    TRANSFORM_POINT_3D(model_xf, cam->eyep, eyep);

    fprintf(fp,
            "Viewpoint {\n  position %g %g %g\n  orientation %g %g %g %g\n  fieldOfView %g\n  description \"%s\"\n}\n\n",
            eyep.x, eyep.y, eyep.z,
            view_rotaxis.x, view_rotaxis.y, view_rotaxis.z, view_rotangle,
            (double) (2. * cam->fov * M_PI / 180.),
            vpname);
}

void
WriteVRMLViewPoints(FILE *fp, Matrix4x4 model_xf) {
    CAMERA *cam = (CAMERA *) nullptr;
    int count = 1;
    WriteVRMLViewPoint(fp, model_xf, &GLOBAL_camera_mainCamera, "ViewPoint 1");
    while ((cam = NextSavedCamera(cam)) != (CAMERA *) nullptr) {
        char vpname[21];
        count++;
        snprintf(vpname, 21, "ViewPoint %d", count);
        WriteVRMLViewPoint(fp, model_xf, cam, vpname);
    }
}

void
WriteVRMLHeader(FILE *fp) {
    Matrix4x4 model_xf;
    Vector3D model_rotaxis;
    float model_rotangle;

    fprintf(fp, "#VRML V2.0 utf8\n\n");

    fprintf(fp, "WorldInfo {\n  title \"%s\"\n  info [ \"Created using RenderPark (%s)\" ]\n}\n\n",
            "Some nice model",
            RPKHOME);

    fprintf(fp, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    model_xf = VRMLModelTransform(&model_rotaxis, &model_rotangle);
    WriteVRMLViewPoints(fp, model_xf);

    fprintf(fp, "Transform {\n  rotation %g %g %g %g\n  children [\n    Shape {\n      geometry IndexedFaceSet {\n",
            model_rotaxis.x, model_rotaxis.y, model_rotaxis.z, model_rotangle);

    fprintf(fp, "\tsolid %s\n", GLOBAL_render_renderOptions.backface_culling ? "TRUE" : "FALSE");
}

void WriteVRMLTrailer(FILE *fp) {
    fprintf(fp, "      }\n    }\n  ]\n}\n\n");
}

static void
ResetVertexId() {
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        for ( int i = 0; i < window->patch->numberOfVertices; i++ ) {
            window->patch->vertex[i]->tmp = -1;
        }
    }
}

static void
WriteCoords(FILE *fp) {
    int id = 0, n = 0;

    ResetVertexId();

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
WriteVertexColors(FILE *fp) {
    int id = 0, n = 0;

    ResetVertexId();

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
WriteFaceColors(FILE *fp) {
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
WriteColors(FILE *fp) {
    fprintf(fp, "\tcolorPerVertex %s\n", GLOBAL_render_renderOptions.smooth_shading ? "TRUE" : "FALSE");
    if ( GLOBAL_render_renderOptions.smooth_shading ) {
        WriteVertexColors(fp);
    } else {
        WriteFaceColors(fp);
    }
}

static void
WriteCoordIndices(FILE *fp) {
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

void
WriteVRML(FILE *fp) {
    WriteVRMLHeader(fp);

    WriteCoords(fp);
    WriteColors(fp);
    WriteCoordIndices(fp);

    WriteVRMLTrailer(fp);
}
