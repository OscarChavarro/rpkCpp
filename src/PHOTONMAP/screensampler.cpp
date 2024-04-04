#include <cmath>

#include "PHOTONMAP/screensampler.h"
#include "raycasting/common/raytools.h"
#include "scene/Camera.h"

/**
newNode gets filled, others may change
*/
bool
ScreenSampler::sample(
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
    double xSample = (double)(GLOBAL_camera_mainCamera.pixelWidth * (double)GLOBAL_camera_mainCamera.xSize * (-0.5 + x1));
    double ySample = (double)(GLOBAL_camera_mainCamera.pixelHeight * (double)GLOBAL_camera_mainCamera.ySize * (-0.5 + x2));

    vectorComb3(GLOBAL_camera_mainCamera.Z, (float)xSample, GLOBAL_camera_mainCamera.X, (float)ySample, GLOBAL_camera_mainCamera.Y,
                dir);
    double distScreen2 = vectorNorm2(dir);
    double distScreen = std::sqrt(distScreen2);
    vectorScaleInverse((float)distScreen, dir, dir);

    double cosScreen = std::fabs(vectorDotProduct(GLOBAL_camera_mainCamera.Z, dir));

    double pdfDir = ((1.0 / (GLOBAL_camera_mainCamera.pixelWidth * (float)GLOBAL_camera_mainCamera.xSize *
                            GLOBAL_camera_mainCamera.pixelHeight * (float)GLOBAL_camera_mainCamera.ySize)) * // 1 / Area pixel
                     (distScreen2 / cosScreen));  // Spherical angle measure

    // Determine ray type
    thisNode->m_rayType = STARTS;
    newNode->m_inBsdf = thisNode->m_outBsdf; // GLOBAL_camera_mainCamera can be placed in a medium

    // Transfer
    if ( !SampleTransfer(thisNode, newNode, &dir, pdfDir) ) {
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
    vectorSubtract(newNode->m_hit.point, thisNode->m_hit.point, outDir);
    dist2 = vectorNorm2(outDir);
    dist = std::sqrt(dist2);
    vectorScaleInverse((float)dist, outDir, outDir);

    // probabilityDensityFunction = 1 / A_screen transformed to area measure

    cosA = vectorDotProduct(thisNode->m_normal, outDir);

    // probabilityDensityFunction = 1/Apix * (r^2 / cos(dir, eyeNormal) * (cos(dir, patchNormal) / d^2)
    //                 |__> to spherical angle           |__> to area on patch

    // Three cosines : r^2 / cos = 1 / cos^3 since r is length
    // of viewing ray to the screen.
    probabilityDensityFunction = 1.0 / (GLOBAL_camera_mainCamera.pixelHeight * (float)GLOBAL_camera_mainCamera.ySize * GLOBAL_camera_mainCamera.pixelWidth *
                                        (float)GLOBAL_camera_mainCamera.xSize * cosA * cosA * cosA);

    cosB = -vectorDotProduct(newNode->m_normal, outDir);
    probabilityDensityFunction = probabilityDensityFunction * cosB / dist2;

    return probabilityDensityFunction;
}


