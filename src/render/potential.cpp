/**
Routines dealing with view potential
*/

#include <ctime>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "material/statistics.h"
#include "scene/Camera.h"
#include "render/canvas.h"
#include "render/softids.h"
#include "render/potential.h"

/**
In analogy with Smits, "Importance-driven Radiosity", SIGGRAPH '92, we
call the integral of potential over surface area "importance"
*/

/**
Updates directly received potential for all patches
*/
void
updateDirectPotential(
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry)
{
    Patch **id2patch;
    unsigned long *ids;
    unsigned long *id;
    long j;
    long x;
    long y;
    unsigned long maximumPatchId;
    long lostPixels;
    Vector3D pixDir;
    float v;
    float h;
    float ySample;
    float *newDirectImportance;
    float deltaImportance;
    float pixelArea;

    canvasPushMode();

    // Get the patch IDs for each pixel
    ids = sglRenderIds(&x, &y, camera, scenePatches, clusteredWorldGeometry);

    canvasPullMode();

    if ( !ids ) {
        return;
    }
    lostPixels = 0;

    // Build a table to convert a patch ID to the corresponding Patch
    maximumPatchId = Patch::getNextId() - 1;
    id2patch = (Patch **)malloc((int) (maximumPatchId + 1) * sizeof(Patch *));
    for ( unsigned long i = 0; i <= maximumPatchId; i++ ) {
        id2patch[i] = nullptr;
    }
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        id2patch[patch->id] = patch;
    }

    // Allocate space for an array to hold the new direct potential of the patches
    newDirectImportance = (float *)malloc((int) (maximumPatchId + 1) * sizeof(float));
    for ( unsigned long i = 0; i <= maximumPatchId; i++ ) {
        newDirectImportance[i] = 0.0;
    }

    // h and v are the horizontal resp. vertical distance between two
    // neighboring pixels on the screen
    h = 2.0f * (float)std::tan(camera->horizontalFov * (float)M_PI / 180.0f) / (float)x;
    v = 2.0f * (float)std::tan(camera->verticalFov * (float)M_PI / 180.0f) / (float)y;
    pixelArea = h * v;

    for ( j = y - 1, ySample = -v * (float) (y - 1) / 2.0f;
          j >= 0;
          j--, ySample += v ) {
        id = ids + j * x;
        for ( long i = 0, xSample = (long)(-h * (float) (x - 1) / 2.0f); i < x; i++, id++, xSample += (long)h ) {
            unsigned long the_id = (*id) & 0xffffff;

            if ( the_id > 0 && the_id <= maximumPatchId ) {
                // Compute direction to center of pixel
                vectorComb3(camera->Z, (float)xSample, camera->X, ySample,
                            camera->Y, pixDir);

                // Delta_importance = (cosine of the angle between the direction to
                // the pixel and the viewing direction, over the distance from the
                // eye point to the pixel) squared, times area of the pixel
                deltaImportance = vectorDotProduct(camera->Z, pixDir) /
                                  vectorDotProduct(pixDir, pixDir);
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
    GLOBAL_statistics.averageDirectPotential /= GLOBAL_statistics.totalArea;

    free((char *) newDirectImportance);
    free((char *) id2patch);
    free((char *) ids);
}

static void
softGetPatchPointers(SGL_CONTEXT *sgl, java::ArrayList<Patch *> *scenePatches) {
    SGL_PIXEL *pix;
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
softUpdateDirectVisibility(
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry)
{
    clock_t t = clock();
    SGL_CONTEXT *oldSglContext = GLOBAL_sgl_currentContext;
    SGL_CONTEXT *currentSglContext = setupSoftFrameBuffer(camera);

    softRenderPatches(camera, scenePatches, clusteredWorldGeometry);
    softGetPatchPointers(currentSglContext, scenePatches);
    delete currentSglContext;
    sglMakeCurrent(oldSglContext);

    fprintf(stderr, "Determining visible patches in software took %g sec\n",
            (float) (clock() - t) / (float) CLOCKS_PER_SEC);
}

/**
Updates view visibility status of all patches
*/
void
updateDirectVisibility(
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry)
{
    canvasPushMode();
    softUpdateDirectVisibility(camera, scenePatches, clusteredWorldGeometry);
    canvasPullMode();
}
