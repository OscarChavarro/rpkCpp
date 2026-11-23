/**
Routines dealing with view potential
*/

#include "java/lang/Math.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/render/Canvas.h"
#include "vsdk/toolkit/render/Potential.h"
#include "vsdk/toolkit/render/Softids.h"

/**
In analogy with [SMIT1992] Smits, "Importance-driven Radiosity", SIGGRAPH '92, we
call the integral of potential over surface area "importance"
*/

/**
Updates directly received potential for all patches
*/
void
Potential::updateDirectPotential(const Scene *scene, const RendererConfiguration *renderOptions) {
    Canvas::canvasPushMode();

    FaceVisibilityBuffer target(scene->camera->xSize, scene->camera->ySize, SglFaceIndex());
    SoftIds::renderVisibleFaces(*scene, renderOptions, target);
    const long x = target.getWidth();
    const long y = target.getHeight();

    Canvas::canvasPullMode();

    long lostPixels = 0;

    // Build a table to convert a patch ID to the corresponding Patch
    const unsigned long maximumPatchId = Patch::getNextId() - 1;
    Patch **id2patch = new Patch *[maximumPatchId + 1];
    for ( unsigned long i = 0; i <= maximumPatchId; i++ ) {
        id2patch[i] = nullptr;
    }
    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        Patch * const patch = scene->patchList->get(i);
        id2patch[patch->getId()] = patch;
    }

    // Allocate space for an array to hold the new direct potential of the patches
    float *newDirectImportance = new float[maximumPatchId + 1];
    for ( unsigned long i = 0; i <= maximumPatchId; i++ ) {
        newDirectImportance[i] = 0.0;
    }

    // h and v are the horizontal resp. vertical distance between two
    // neighboring pixels on the screen
    const float h = 2.0F * java::Math::tan(scene->camera->horizontalFov * static_cast<float>(M_PI) / 180.0F) / static_cast<float>(x);
    const float v = 2.0F * java::Math::tan(scene->camera->verticalFov * static_cast<float>(M_PI) / 180.0F) / static_cast<float>(y);
    const float pixelArea = h * v;

    float ySample;
    long j;
    for ( j = y - 1, ySample = -v * static_cast<float>(y - 1) / 2.0F;
          j >= 0;
          j--, ySample += v ) {
        float xSample = -h * static_cast<float>(x - 1) / 2.0F;
        for ( long i = 0; i < x; i++, xSample += h ) {
            const SglFaceIndex face = target.getSample(static_cast<int>(i), static_cast<int>(j));
            const unsigned long the_id = face.isBackground() ? 0 :
                scene->getFace(face.getIndex())->getId() & 0xffffff;

            if ( the_id > 0 && the_id <= maximumPatchId ) {
                Vector3Df pixDir;

                // Compute direction to center of pixel
                pixDir = scene->camera->Z.add(scene->camera->X.multiply(xSample)).add(scene->camera->Y.multiply(ySample));

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
        Logger::reportMessage("Potential", Logger::WARNING, "", java::String::format("%d lost pixels", lostPixels));
    }

    Statistics::instance().potential.averageDirectPotential = Statistics::instance().potential.totalDirectPotential =
    Statistics::instance().potential.maxDirectPotential = Statistics::instance().potential.maxDirectImportance = 0.0;
    for ( unsigned long i = 1; i <= maximumPatchId; i++ ) {
        Patch * const patch = id2patch[i];

        if ( patch != nullptr ) {
            DirectPotentialTable * const directPotentials = scene->getDirectPotentials();
            const int faceIndex = patch->getFaceIndex();
            directPotentials->set(faceIndex, static_cast<float>(newDirectImportance[i] / patch->getArea()));

            if ( directPotentials->get(faceIndex) > Statistics::instance().potential.maxDirectPotential ) {
                Statistics::instance().potential.maxDirectPotential = directPotentials->get(faceIndex);
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
}

void
Potential::softUpdateDirectVisibility(const Scene *scene, const RendererConfiguration *renderOptions) {
    const long long t = java::System::nanoTime();

    FaceVisibilityBuffer target(scene->camera->xSize, scene->camera->ySize, SglFaceIndex());
    SoftIds::renderVisibleFaces(*scene, renderOptions, target);

    FaceVisibilityTable *const faceVisibility = scene->getFaceVisibility();
    for ( int i = 0; i < faceVisibility->getNumberOfFaces(); i++ ) {
        faceVisibility->setVisible(i, false);
    }
    for ( int y = 0; y < target.getHeight(); y++ ) {
        for ( int x = 0; x < target.getWidth(); x++ ) {
            const SglFaceIndex face = target.getSample(x, y);
            if ( !face.isBackground() ) {
                faceVisibility->setVisible(face.getIndex(), true);
            }
        }
    }

    java::System::err.printf("Determining visible patches in software took %g sec\n",
        static_cast<float>(static_cast<double>(java::System::nanoTime() - t) / 1000000000.0));
}

/**
Updates view visibility status of all patches
*/
void
Potential::updateDirectVisibility(const Scene *scene, const RendererConfiguration *renderOptions) {
    Canvas::canvasPushMode();
    Potential::softUpdateDirectVisibility(scene, renderOptions);
    Canvas::canvasPullMode();
}
