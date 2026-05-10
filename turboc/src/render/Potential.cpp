/**
Routines dealing with view potential
*/

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "render/Canvas.h"
#include "render/Potential.h"
#include "render/Softids.h"

/**
In analogy with [SMIT1992] Smits, "Importance-driven Radiosity", SIGGRAPH '92, we
call the integral of potential over surface area "importance"
*/

/**
Updates directly received potential for all patches
*/
void
Potential::updateDirectPotential(const Scene *scene, const RenderOptions *renderOptions) {
    Canvas::canvasPushMode();

    // Get the patch IDs for each pixel
    long x;
    long y;
    unsigned long *ids = SoftIds::softRenderIds(&x, &y, scene, renderOptions);

    Canvas::canvasPullMode();

    if ( ids == NULL ) {
        return;
    }

    long lostPixels = 0;

    // Build a table to convert a patch ID to the corresponding Patch
    const int nextPatchId = Patch::getNextId();
    if ( nextPatchId <= 0 ) {
        delete[] ids;
        return;
    }
    unsigned long maximumPatchId = ((unsigned long)(nextPatchId - 1));
    Patch **id2patch = new Patch *[maximumPatchId + 1];
    for ( unsigned long i = 0; i <= maximumPatchId; i++ ) {
        id2patch[i] = NULL;
    }
    for ( int i = 0; scene->patchList != NULL && i < scene->patchList->size(); i++ ) {
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
    float h = 2.0f * Math::tan(scene->camera->horizontalFov * ((float)(M_PI)) / 180.0f) / ((float)(x));
    float v = 2.0f * Math::tan(scene->camera->verticalFov * ((float)(M_PI)) / 180.0f) / ((float)(y));
    float pixelArea = h * v;

    float ySample;
    long j;
    for ( j = y - 1, ySample = -v * ((float)(y - 1)) / 2.0f;
          j >= 0;
          j--, ySample += v ) {
        const long rowStart = j * x;
        for ( long i = 0; i < x; i++ ) {
            const float xSample = -h * ((float)(x - 1)) / 2.0f + h * ((float)(i));
            const unsigned long the_id = ids[rowStart + i] & 0xffffff;

            if ( the_id > 0 && the_id <= maximumPatchId ) {
                Vector3D pixDir;

                // Compute direction to center of pixel
                pixDir.combine3(scene->camera->Z, ((float)(xSample)), scene->camera->X, ySample, scene->camera->Y);

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
        Logger::warning(NULL, "%d lost pixels", lostPixels);
    }

    Statistics::instance().potential.averageDirectPotential = Statistics::instance().potential.totalDirectPotential =
    Statistics::instance().potential.maxDirectPotential = Statistics::instance().potential.maxDirectImportance = 0.0;
    for ( unsigned long i = 1; i <= maximumPatchId; i++ ) {
        Patch *patch = id2patch[i];

        if ( patch != NULL ) {
            patch->directPotential = newDirectImportance[i] / patch->area;

            if ( patch->directPotential > Statistics::instance().potential.maxDirectPotential ) {
                Statistics::instance().potential.maxDirectPotential = patch->directPotential;
            }
            Statistics::instance().potential.totalDirectPotential += newDirectImportance[i];
            Statistics::instance().potential.averageDirectPotential += newDirectImportance[i];

            if ( newDirectImportance[i] > Statistics::instance().potential.maxDirectImportance ) {
                Statistics::instance().potential.maxDirectImportance = newDirectImportance[i];
            }
        }
    }
    Statistics::instance().potential.averageDirectPotential /= Statistics::instance().radiance.totalArea;

    delete[] newDirectImportance;
    delete[] id2patch;
    delete[] ids;
}

void
Potential::softGetPatchPointers(const SglContext *sgl, const ArrayList<Patch *> *scenePatches) {
    int i;

    for ( i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
        scenePatches->get(i)->setInvisible();
    }

    const int pixelCount = sgl->width * sgl->height;
    for ( i = 0; i < pixelCount; i++ ) {
        Patch *P = ((Patch *)(sgl->frameBuffer[i]));
        if ( P ) {
            P->setVisible();
        }
    }
}

void
Potential::softUpdateDirectVisibility(const Scene *scene, const RenderOptions *renderOptions) {
    const long t = System::nanoTime();
    SglContext *currentSglContext = SoftIds::setupSoftFrameBuffer(scene->camera);

    SoftIds::softRenderPatches(scene, renderOptions, currentSglContext);
    Potential::softGetPatchPointers(currentSglContext, scene->patchList);
    delete currentSglContext;

    System::err.printf("Determining visible patches in software took %g sec\n",
        ((float)(((double)(System::nanoTime() - t)) / 1000000000.0)));
}

/**
Updates view visibility status of all patches
*/
void
Potential::updateDirectVisibility(const Scene *scene, const RenderOptions *renderOptions) {
    Canvas::canvasPushMode();
    Potential::softUpdateDirectVisibility(scene, renderOptions);
    Canvas::canvasPullMode();
}
