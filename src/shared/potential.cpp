/*
routines dealing with view potential
*/

#include <ctime>
#include <cstdlib>

#include "common/error.h"
#include "skin/Patch.h"
#include "scene/scene.h"
#include "material/statistics.h"
#include "shared/potential.h"
#include "shared/camera.h"
#include "shared/render.h"
#include "shared/canvas.h"
#include "shared/softids.h"

/* In analogy with Smits, "Importance-driven Radiosity", SIGGRAPH '92, we
 * call the integral of potential over surface area "importance". */

/* Updates directly received potential for all patches. */
void UpdateDirectPotential() {
    Patch **id2patch;
    unsigned long *ids, *id;
    long i;
    long j;
    long x;
    long y;
    long maxpatchid;
    long lostpixels;
    Vector3D pixdir;
    float v, h, xsample, ysample;
    float *new_direct_importance, delta_importance, pixarea;

    canvasPushMode();

    // Get the patch IDs for each pixel
    ids = renderIds(&x, &y);

    canvasPullMode();

    if ( !ids ) {
        return;
    }
    lostpixels = 0;

    /* build a table to convert a patch ID to the corresponding Patch * */
    maxpatchid = patchGetNextId() - 1;
    id2patch = (Patch **)malloc((int) (maxpatchid + 1) * sizeof(Patch *));
    for ( i = 0; i <= maxpatchid; i++ ) {
        id2patch[i] = (Patch *) nullptr;
    }
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        id2patch[window->patch->id] = window->patch;
    }

    /* allocate space for an array to hold the new direct potential of the
     * patches. */
    new_direct_importance = (float *)malloc((int) (maxpatchid + 1) * sizeof(float));
    for ( i = 0; i <= maxpatchid; i++ ) {
        new_direct_importance[i] = 0.;
    }

    /* h and v are the horizontal resp. vertical distance between two
     * neighbooring pixels on the screen. */
    h = 2. * tan(GLOBAL_camera_mainCamera.hfov * M_PI / 180.) / (float) x;
    v = 2. * tan(GLOBAL_camera_mainCamera.vfov * M_PI / 180.) / (float) y;
    pixarea = h * v;

    for ( j = y - 1, ysample = -v * (float) (y - 1) / 2.; j >= 0; j--, ysample += v ) {
        id = ids + j * x;
        for ( i = 0, xsample = -h * (float) (x - 1) / 2.; i < x; i++, id++, xsample += h ) {
            unsigned long the_id = (*id) & 0xffffff;

            if ( the_id > 0 && the_id <= maxpatchid ) {
                /* compute direction to center of pixel */
                VECTORCOMB3(GLOBAL_camera_mainCamera.Z, xsample, GLOBAL_camera_mainCamera.X, ysample, GLOBAL_camera_mainCamera.Y, pixdir);

                /* delta_importance = (cosine of the angle between the direction to
                 * the pixel and the viewing direction, over the distance from the
                 * eye point to the pixel) squared, times area of the pixel. */
                delta_importance = VECTORDOTPRODUCT(GLOBAL_camera_mainCamera.Z, pixdir) / VECTORDOTPRODUCT(pixdir, pixdir);
                delta_importance *= delta_importance * pixarea;

                new_direct_importance[the_id] += delta_importance;
            } else if ( the_id > maxpatchid ) {
                lostpixels++;
            }
        }
    }

    if ( lostpixels > 0 ) {
        logWarning(nullptr, "%d lost pixels", lostpixels);
    }

    GLOBAL_statistics_averageDirectPotential = GLOBAL_statistics_totalDirectPotential =
    GLOBAL_statistics_maxDirectPotential = GLOBAL_statistics_maxDirectImportance = 0.;
    for ( i = 1; i <= maxpatchid; i++ ) {
        Patch *patch = id2patch[i];

        patch->directPotential = new_direct_importance[i] / patch->area;

        if ( patch->directPotential > GLOBAL_statistics_maxDirectPotential ) {
            GLOBAL_statistics_maxDirectPotential = patch->directPotential;
        }
        GLOBAL_statistics_totalDirectPotential += new_direct_importance[i];
        GLOBAL_statistics_averageDirectPotential += new_direct_importance[i];

        if ( new_direct_importance[i] > GLOBAL_statistics_maxDirectImportance ) {
            GLOBAL_statistics_maxDirectImportance = new_direct_importance[i];
        }
    }
    GLOBAL_statistics_averageDirectPotential /= GLOBAL_statistics_totalArea;

    free((char *) new_direct_importance);
    free((char *) id2patch);
    free((char *) ids);
}

static SGL_PIXEL PatchPointer(Patch *P) {
    return (SGL_PIXEL) P;
}

static void SoftGetPatchPointers(SGL_CONTEXT *sgl) {
    SGL_PIXEL *pix;
    int i;

    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        // Initialise to invisible
        PATCH_SET_INVISIBLE(window->patch);
    }

    for ( pix = sgl->frameBuffer, i = 0; i < sgl->width * sgl->height; pix++, i++ ) {
        Patch *P = (Patch *) (*pix);
        if ( P ) {
            // Visible
            PATCH_SET_VISIBLE(P);
        }
    }
}

void
SoftUpdateDirectVisibility() {
    clock_t t = clock();
    SGL_CONTEXT *oldsgl = GLOBAL_sgl_currentContext;
    SGL_CONTEXT *sgl = SetupSoftFrameBuffer();
    SoftRenderPatches(PatchPointer);
    SoftGetPatchPointers(sgl);
    sglClose(sgl);
    sglMakeCurrent(oldsgl);

    fprintf(stderr, "Determining visible patches in software took %g sec\n",
            (float) (clock() - t) / (float) CLOCKS_PER_SEC);
}

/* Updates view visibility status of all patches. */
void UpdateDirectVisibility() {
    canvasPushMode();
    SoftUpdateDirectVisibility();
    canvasPullMode();
}

