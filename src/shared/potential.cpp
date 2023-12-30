/*
routines dealing with view potential
*/

#include <ctime>
#include <cstdlib>

#include "skin/patch_flags.h"
#include "common/error.h"
#include "skin/patch.h"
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
    PATCH **id2patch;
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

    CanvasPushMode();

    /* get the patch IDs for each pixel. */
    ids = RenderIds(&x, &y);

    CanvasPullMode();

    if ( !ids ) {
        return;
    }
    lostpixels = 0;

    /* build a table to convert a patch ID to the corresponding PATCH * */
    maxpatchid = PatchGetNextID() - 1;
    id2patch = (PATCH **)malloc((int) (maxpatchid + 1) * sizeof(PATCH *));
    for ( i = 0; i <= maxpatchid; i++ ) {
        id2patch[i] = (PATCH *) nullptr;
    }
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    id2patch[P->id] = P;
                }
    EndForAll;

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
        Warning(nullptr, "%d lost pixels", lostpixels);
    }

    GLOBAL_statistics_averageDirectPotential = GLOBAL_statistics_totalDirectPotential =
    GLOBAL_statistics_maxDirectPotential = GLOBAL_statistics_maxDirectImportance = 0.;
    for ( i = 1; i <= maxpatchid; i++ ) {
        PATCH *patch = id2patch[i];

        patch->direct_potential = new_direct_importance[i] / patch->area;

        if ( patch->direct_potential > GLOBAL_statistics_maxDirectPotential ) {
            GLOBAL_statistics_maxDirectPotential = patch->direct_potential;
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

static SGL_PIXEL PatchPointer(PATCH *P) {
    return (SGL_PIXEL) P;
}

static void SoftGetPatchPointers(SGL_CONTEXT *sgl) {
    SGL_PIXEL *pix;
    int i;

    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    PATCH_SET_INVISIBLE(P);  /* initialise to invisible */
                }
    EndForAll;

    for ( pix = sgl->fbuf, i = 0; i < sgl->width * sgl->height; pix++, i++ ) {
        PATCH *P = (PATCH *) (*pix);
        if ( P ) PATCH_SET_VISIBLE(P);  /* visible */
    }
}

void SoftUpdateDirectVisibility() {
    clock_t t = clock();
    SGL_CONTEXT *oldsgl = sglGetCurrent();
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
    CanvasPushMode();
    SoftUpdateDirectVisibility();
    CanvasPullMode();
}

