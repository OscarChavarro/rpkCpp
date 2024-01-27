/**
Routines dealing with view potential
*/

#include <ctime>

#include "common/error.h"
#include "material/statistics.h"
#include "shared/options.h"
#include "shared/render.h"
#include "shared/canvas.h"
#include "shared/softids.h"
#include "shared/potential.h"

/**
In analogy with Smits, "Importance-driven Radiosity", SIGGRAPH '92, we
call the integral of potential over surface area "importance"
*/

/**
Updates directly received potential for all patches
*/
void
updateDirectPotential() {
    Patch **id2patch;
    unsigned long *ids;
    unsigned long *id;
    long j;
    long x;
    long y;
    unsigned long maximumPatchId;
    long lostPixels;
    Vector3D pixdir;
    float v;
    float h;
    float ySample;
    float *newDirectImportance;
    float deltaImportance;
    float pixelArea;

    canvasPushMode();

    // Get the patch IDs for each pixel
    ids = renderIds(&x, &y);

    canvasPullMode();

    if ( !ids ) {
        return;
    }
    lostPixels = 0;

    // Build a table to convert a patch ID to the corresponding Patch
    maximumPatchId = patchGetNextId() - 1;
    id2patch = (Patch **)malloc((int) (maximumPatchId + 1) * sizeof(Patch *));
    for ( unsigned long i = 0; i <= maximumPatchId; i++ ) {
        id2patch[i] = (Patch *) nullptr;
    }
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        id2patch[window->patch->id] = window->patch;
    }

    // Allocate space for an array to hold the new direct potential of the patches
    newDirectImportance = (float *)malloc((int) (maximumPatchId + 1) * sizeof(float));
    for ( unsigned long i = 0; i <= maximumPatchId; i++ ) {
        newDirectImportance[i] = 0.;
    }

    // h and v are the horizontal resp. vertical distance between two
    // neighboring pixels on the screen
    h = 2.0f * (float)std::tan(GLOBAL_camera_mainCamera.horizontalFov * (float)M_PI / 180.0f) / (float)x;
    v = 2.0f * (float)std::tan(GLOBAL_camera_mainCamera.verticalFov * (float)M_PI / 180.0f) / (float)y;
    pixelArea = h * v;

    for ( j = y - 1, ySample = -v * (float) (y - 1) / 2.0; j >= 0; j--, ySample += v ) {
        id = ids + j * x;
        for ( long i = 0, xSample = -h * (float) (x - 1) / 2.0; i < x; i++, id++, xSample += h ) {
            unsigned long the_id = (*id) & 0xffffff;

            if ( the_id > 0 && the_id <= maximumPatchId ) {
                // Compute direction to center of pixel
                VECTORCOMB3(GLOBAL_camera_mainCamera.Z, xSample, GLOBAL_camera_mainCamera.X, ySample, GLOBAL_camera_mainCamera.Y, pixdir);

                /* delta_importance = (cosine of the angle between the direction to
                 * the pixel and the viewing direction, over the distance from the
                 * eye point to the pixel) squared, times area of the pixel. */
                deltaImportance = VECTORDOTPRODUCT(GLOBAL_camera_mainCamera.Z, pixdir) / VECTORDOTPRODUCT(pixdir, pixdir);
                deltaImportance *= deltaImportance * pixelArea;

                newDirectImportance[the_id] += deltaImportance;
            } else if ( the_id > maximumPatchId ) {
                lostPixels++;
            }
        }
    }

    if ( lostPixels > 0 ) {
        logWarning(nullptr, "%d lost pixels", lostPixels);
    }

    GLOBAL_statistics_averageDirectPotential = GLOBAL_statistics_totalDirectPotential =
    GLOBAL_statistics_maxDirectPotential = GLOBAL_statistics_maxDirectImportance = 0.;
    for ( unsigned long i = 1; i <= maximumPatchId; i++ ) {
        Patch *patch = id2patch[i];

        patch->directPotential = newDirectImportance[i] / patch->area;

        if ( patch->directPotential > GLOBAL_statistics_maxDirectPotential ) {
            GLOBAL_statistics_maxDirectPotential = patch->directPotential;
        }
        GLOBAL_statistics_totalDirectPotential += newDirectImportance[i];
        GLOBAL_statistics_averageDirectPotential += newDirectImportance[i];

        if ( newDirectImportance[i] > GLOBAL_statistics_maxDirectImportance ) {
            GLOBAL_statistics_maxDirectImportance = newDirectImportance[i];
        }
    }
    GLOBAL_statistics_averageDirectPotential /= GLOBAL_statistics_totalArea;

    free((char *) newDirectImportance);
    free((char *) id2patch);
    free((char *) ids);
}

static SGL_PIXEL
patchPointer(Patch *P) {
    return (SGL_PIXEL) P;
}

static void
softGetPatchPointers(SGL_CONTEXT *sgl) {
    SGL_PIXEL *pix;
    int i;

    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        window->patch->setInvisible();
    }

    for ( pix = sgl->frameBuffer, i = 0; i < sgl->width * sgl->height; pix++, i++ ) {
        Patch *P = (Patch *) (*pix);
        if ( P ) {
            P->setVisible();
        }
    }
}

void
softUpdateDirectVisibility() {
    clock_t t = clock();
    SGL_CONTEXT *oldsgl = GLOBAL_sgl_currentContext;
    SGL_CONTEXT *sgl = setupSoftFrameBuffer();
    softRenderPatches(patchPointer);
    softGetPatchPointers(sgl);
    sglClose(sgl);
    sglMakeCurrent(oldsgl);

    fprintf(stderr, "Determining visible patches in software took %g sec\n",
            (float) (clock() - t) / (float) CLOCKS_PER_SEC);
}

/**
Updates view visibility status of all patches
*/
void
updateDirectVisibility() {
    canvasPushMode();
    softUpdateDirectVisibility();
    canvasPullMode();
}

