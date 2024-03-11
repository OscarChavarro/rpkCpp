#include <cmath>
#include "scene/Camera.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/pixelsampler.h"

bool CPixelSampler::sample(SimpleRaytracingPathNode *prevNode/*prevNode*/, SimpleRaytracingPathNode *thisNode,
                           SimpleRaytracingPathNode *newNode, double x1, double x2,
                           bool /* doRR */doRR, BSDF_FLAGS /* flags */flags) {
    Vector3D dir;

    // Pre-condition: thisNode == eye, prevNode == nullptr, SetPixel called

    // Sample direction
    double xSample = (m_px + GLOBAL_camera_mainCamera.pixelWidth * x1);
    double ySample = (m_py + GLOBAL_camera_mainCamera.pixelHeight * x2);

    vectorComb3(GLOBAL_camera_mainCamera.Z, (float)xSample, GLOBAL_camera_mainCamera.X, (float)ySample, GLOBAL_camera_mainCamera.Y,
                dir);
    double distPixel2 = vectorNorm2(dir);
    double distPixel = sqrt(distPixel2);
    vectorScaleInverse((float)distPixel, dir, dir);

    double cosPixel = fabs(vectorDotProduct(GLOBAL_camera_mainCamera.Z, dir));

    double pdfDir = ((1. / (GLOBAL_camera_mainCamera.pixelWidth * GLOBAL_camera_mainCamera.pixelHeight)) * // 1 / Area pixel
                     (distPixel2 / cosPixel));  // Spherical angle measure

    // Determine ray type
    thisNode->m_rayType = Starts;
    newNode->m_inBsdf = thisNode->m_outBsdf; // GLOBAL_camera_mainCamera can be placed in a medium

    // Transfer
    if ( !SampleTransfer(thisNode, newNode, &dir, pdfDir)) {
        thisNode->m_rayType = Stops;
        return false;
    }

    // "Bsdf" in thisNode

    // Potential is one for all directions through a pixel
    colorSetMonochrome(thisNode->m_bsdfEval, 1.0);

    // Make sure evaluation of eye components always includes the diff ref.
    thisNode->m_bsdfComp.Clear();
    thisNode->m_bsdfComp.Fill(thisNode->m_bsdfEval, BRDF_DIFFUSE_COMPONENT);

    // Component propagation
    thisNode->m_usedComponents = NO_COMPONENTS; // the eye...
    newNode->m_accUsedComponents = static_cast<BSDF_FLAGS>(thisNode->m_accUsedComponents | thisNode->m_usedComponents);

    newNode->m_rracc = thisNode->m_rracc; // No russian roulette

    return true;
}

void CPixelSampler::SetPixel(int nx, int ny, Camera *cam) {
    if ( cam == nullptr ) {
        cam = &GLOBAL_camera_mainCamera;
    } // Primary camera

    m_px = -cam->pixelWidth * (double)cam->xSize / 2.0 + (double)nx * cam->pixelWidth;
    m_py = -cam->pixelHeight * (double)cam->ySize / 2.0 + (double)ny * cam->pixelHeight;
}

double CPixelSampler::EvalPDF(SimpleRaytracingPathNode *thisNode, SimpleRaytracingPathNode *newNode,
                              BSDF_FLAGS /*flags*/, double * /*pdf*/,
                              double * /*pdfRR*/) {
    double dist2;
    double dist;
    double cosA;
    double cosB;
    double pdf;
    Vector3D outDir;

    /* -- more efficient with extra params ?? -- */

    vectorSubtract(newNode->m_hit.point, thisNode->m_hit.point, outDir);
    dist2 = vectorNorm2(outDir);
    dist = sqrt(dist2);
    vectorScaleInverse((float)dist, outDir, outDir);

    // pdf = 1 / A_pixel transformed to area measure

    cosA = vectorDotProduct(thisNode->m_normal, outDir);

    // pdf = 1/APixel * (r^2 / cos(dir, eyeNormal) * (cos(dir, patchNormal) / d^2)
    //                 |__> to spherical.angle           |__> to area on patch

    // Three cosines : r^2 / cos = 1 / cos^3 since r is length
    // of viewing ray to the pixel.
    pdf = 1.0 / (GLOBAL_camera_mainCamera.pixelHeight * GLOBAL_camera_mainCamera.pixelWidth * cosA * cosA * cosA);

    cosB = -vectorDotProduct(newNode->m_normal, outDir);
    pdf = pdf * cosB / dist2;

    return pdf;
}

