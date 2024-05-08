#include <cmath>

#include "PHOTONMAP/screensampler.h"
#include "raycasting/common/raytools.h"
#include "scene/Camera.h"

/**
newNode gets filled, others may change
*/
bool
ScreenSampler::sample(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode */*prevNode*/,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool /* doRR */,
    BSDF_FLAGS /* flags */)
{
    Vector3D dir;

    // Pre-condition: thisNode == eye, prevNode == nullptr, SetPixel called

    // Sample direction
    double xSample = (double)(camera->pixelWidth * (double)camera->xSize * (-0.5 + x1));
    double ySample = (double)(camera->pixelHeight * (double)camera->ySize * (-0.5 + x2));

    vectorComb3(camera->Z, (float)xSample, camera->X, (float)ySample, camera->Y,
                dir);
    double distScreen2 = vectorNorm2(dir);
    double distScreen = std::sqrt(distScreen2);
    vectorScaleInverse((float)distScreen, dir, dir);

    double cosScreen = std::fabs(vectorDotProduct(camera->Z, dir));

    double pdfDir = ((1.0 / (camera->pixelWidth * (float)camera->xSize *
                            camera->pixelHeight * (float)camera->ySize)) * // 1 / Area pixel
                     (distScreen2 / cosScreen));  // Spherical angle measure

    // Determine ray type
    thisNode->m_rayType = STARTS;
    newNode->m_inBsdf = thisNode->m_outBsdf; // Camera can be placed in a medium

    // Transfer
    if ( !sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, &dir, pdfDir) ) {
        thisNode->m_rayType = STOPS;
        return false;
    }

    // "Bsdf" in thisNode

    // Potential is one for all directions through a pixel
    thisNode->m_bsdfEval.setMonochrome(1.0);

    // Make sure evaluation of eye components always includes the diff ref.
    thisNode->m_bsdfComp.Clear();
    thisNode->m_bsdfComp.Fill(thisNode->m_bsdfEval, BRDF_DIFFUSE_COMPONENT);

    // Component propagation
    thisNode->m_usedComponents = NO_COMPONENTS; // The eye...
    newNode->m_accUsedComponents = static_cast<BSDF_FLAGS>(thisNode->m_accUsedComponents | thisNode->m_usedComponents);
    return true;
}

double
ScreenSampler::evalPDF(
    Camera *camera,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    BSDF_FLAGS /*flags*/,
    double * /*probabilityDensityFunction*/,
    double * /*probabilityDensityFunctionRR*/)
{
    double dist2;
    double dist;
    double cosA;
    double cosB;
    double probabilityDensityFunction;
    Vector3D outDir;

    // More efficient with extra params?
    vectorSubtract(newNode->m_hit.getPoint(), thisNode->m_hit.getPoint(), outDir);
    dist2 = vectorNorm2(outDir);
    dist = std::sqrt(dist2);
    vectorScaleInverse((float)dist, outDir, outDir);

    // probabilityDensityFunction = 1 / A_screen transformed to area measure
    cosA = vectorDotProduct(thisNode->m_normal, outDir);

    // probabilityDensityFunction = 1/Apix * (r^2 / cos(dir, eyeNormal) * (cos(dir, patchNormal) / d^2)
    //                 |__> to spherical angle           |__> to area on patch

    // Three cosines : r^2 / cos = 1 / cos^3 since r is length
    // of viewing ray to the screen.
    probabilityDensityFunction = 1.0 /
        (camera->pixelHeight * (float)camera->ySize * camera->pixelWidth * (float)camera->xSize * cosA * cosA * cosA);

    cosB = -vectorDotProduct(newNode->m_normal, outDir);
    probabilityDensityFunction = probabilityDensityFunction * cosB / dist2;

    return probabilityDensityFunction;
}


