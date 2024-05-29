/**
Routines dealing with view potential
*/

#include <ctime>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/Statistics.h"
#include "scene/Camera.h"
#include "render/canvas.h"
#include "render/softids.h"
#include "render/potential.h"

/**
In analogy with [SMIT1992] Smits, "Importance-driven Radiosity", SIGGRAPH '92, we
call the integral of potential over surface area "importance"
*/

/**
Updates directly received potential for all patches
*/
void
updateDirectPotential(const Scene *scene, const RenderOptions *renderOptions) {
    canvasPushMode();

    // Get the patch IDs for each pixel
    long x;
    long y;
    unsigned long *ids = softRenderIds(&x, &y, scene, renderOptions);

    canvasPullMode();

    if ( ids == nullptr ) {
        return;
    }

    long lostPixels = 0;

    // Build a table to convert a patch ID to the corresponding Patch
    unsigned long maximumPatchId = Patch::getNextId() - 1;
    Patch **id2patch = new Patch *[maximumPatchId + 1];
    for ( unsigned long i = 0; i <= maximumPatchId; i++ ) {
        id2patch[i] = nullptr;
    }
    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        Patch *patch = scene->patchList->get(i);
        id2patch[patch->id] = patch;
    }

    // Allocate space for an array to hold the new direct potential of the patches
    float *newDirectImportance = new float[maximumPatchId + 1];
    for ( unsigned long i = 0; i <= maximumPatchId; i++ ) {
        newDirectImportance[i] = 0.0;
    }

    // h and v are the horizontal resp. vertical distance between two
    // neighboring pixels on the screen
    float h = 2.0f * java::Math::tan(scene->camera->horizontalFov * (float)M_PI / 180.0f) / (float)x;
    float v = 2.0f * java::Math::tan(scene->camera->verticalFov * (float)M_PI / 180.0f) / (float)y;
    float pixelArea = h * v;

    float ySample;
    long j;
    for ( j = y - 1, ySample = -v * (float) (y - 1) / 2.0f;
          j >= 0;
          j--, ySample += v ) {
        const unsigned long *id;

        id = ids + j * x;
        for ( long i = 0, xSample = (long)(-h * (float) (x - 1) / 2.0f); i < x; i++, id++, xSample += (long)h ) {
            unsigned long the_id = (*id) & 0xffffff;

            if ( the_id > 0 && the_id <= maximumPatchId ) {
                Vector3D pixDir;

                // Compute direction to center of pixel
                pixDir.combine3(scene->camera->Z, (float) xSample, scene->camera->X, ySample, scene->camera->Y);

                // Delta_importance = (cosine of the angle between the direction to
                // the pixel and the viewing direction, over the distance from the
                // eye point to the pixel) squared, times area of the pixel
                float deltaImportance = scene->camera->Z.dotProduct(pixDir) / pixDir.dotProduct(pixDir);
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

    GLOBAL_statistics.averageDirectPotential = GLOBAL_statistics.totalDirectPotential =
    GLOBAL_statistics.maxDirectPotential = GLOBAL_statistics.maxDirectImportance = 0.0;
    for ( unsigned long i = 1; i <= maximumPatchId; i++ ) {
        Patch *patch = id2patch[i];

        if ( patch != nullptr ) {
            patch->directPotential = newDirectImportance[i] / patch->area;

            if ( patch->directPotential > GLOBAL_statistics.maxDirectPotential ) {
                GLOBAL_statistics.maxDirectPotential = patch->directPotential;
            }
            GLOBAL_statistics.totalDirectPotential += newDirectImportance[i];
            GLOBAL_statistics.averageDirectPotential += newDirectImportance[i];

            if ( newDirectImportance[i] > GLOBAL_statistics.maxDirectImportance ) {
                GLOBAL_statistics.maxDirectImportance = newDirectImportance[i];
            }
        }
    }
    GLOBAL_statistics.averageDirectPotential /= GLOBAL_statistics.totalArea;

    delete[] newDirectImportance;
    delete[] id2patch;
    delete[] ids;
}

static void
softGetPatchPointers(const SGL_CONTEXT *sgl, const java::ArrayList<Patch *> *scenePatches) {
    const SGL_PIXEL *pix;
    int i;

    for ( i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        scenePatches->get(i)->setInvisible();
    }

    for ( pix = sgl->frameBuffer, i = 0; i < sgl->width * sgl->height; pix++, i++ ) {
        Patch *P = (Patch *) (*pix);
        if ( P ) {
            P->setVisible();
        }
    }
}

static void
softUpdateDirectVisibility(const Scene *scene, const RenderOptions *renderOptions) {
    clock_t t = clock();
    SGL_CONTEXT *oldSglContext = GLOBAL_sgl_currentContext;
    SGL_CONTEXT *currentSglContext = setupSoftFrameBuffer(scene->camera);

    softRenderPatches(scene, renderOptions);
    softGetPatchPointers(currentSglContext, scene->patchList);
    delete currentSglContext;
    sglMakeCurrent(oldSglContext);

    fprintf(stderr, "Determining visible patches in software took %g sec\n",
        (float) (clock() - t) / (float) CLOCKS_PER_SEC);
}

/**
Updates view visibility status of all patches
*/
void
updateDirectVisibility(const Scene *scene, const RenderOptions *renderOptions) {
    canvasPushMode();
    softUpdateDirectVisibility(scene, renderOptions);
    canvasPullMode();
}
