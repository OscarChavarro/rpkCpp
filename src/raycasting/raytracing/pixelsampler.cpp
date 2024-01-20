#include <cmath>
#include "shared/Camera.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/pixelsampler.h"

bool CPixelSampler::Sample(CPathNode */*prevNode*/, CPathNode *thisNode,
                           CPathNode *newNode, double x_1, double x_2,
                           bool /* doRR */, BSDFFLAGS /* flags */) {
    Vector3D dir;

    // Precond: thisNode == eye, prevNode == nullptr, SetPixel called

    // Sample direction
    double xsample = (m_px + GLOBAL_camera_mainCamera.pixelWidth * x_1);
    double ysample = (m_py + GLOBAL_camera_mainCamera.pixelHeight * x_2);

    VECTORCOMB3(GLOBAL_camera_mainCamera.Z, xsample, GLOBAL_camera_mainCamera.X, ysample, GLOBAL_camera_mainCamera.Y, dir);
    double distPixel2 = VECTORNORM2(dir);
    double distPixel = sqrt(distPixel2);
    VECTORSCALEINVERSE(distPixel, dir, dir);

    double cosPixel = fabs(VECTORDOTPRODUCT(GLOBAL_camera_mainCamera.Z, dir));

    double pdfDir = ((1. / (GLOBAL_camera_mainCamera.pixelWidth * GLOBAL_camera_mainCamera.pixelHeight)) * // 1 / Area pixel
                     (distPixel2 / cosPixel));  // spher. angle measure

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
    newNode->m_accUsedComponents = (thisNode->m_accUsedComponents |
                                    thisNode->m_usedComponents);

    newNode->m_rracc = thisNode->m_rracc; // No russian roulette

    return true;
}

void CPixelSampler::SetPixel(int nx, int ny, Camera *cam) {
    m_nx = nx;
    m_ny = ny;

    if ( cam == nullptr ) {
        cam = &GLOBAL_camera_mainCamera;
    } // Primary camera

    m_px = -cam->pixelWidth * cam->xSize / 2.0 + nx * cam->pixelWidth;
    m_py = -cam->pixelHeight * cam->ySize / 2.0 + ny * cam->pixelHeight;
}

double CPixelSampler::EvalPDF(CPathNode *thisNode, CPathNode *newNode,
                              BSDFFLAGS /*flags*/, double * /*pdf*/,
                              double * /*pdfRR*/) {
    double dist2, dist, cosa, cosb, pdf;
    Vector3D outDir;

    /* -- more efficient with extra params ?? -- */

    VECTORSUBTRACT(newNode->m_hit.point, thisNode->m_hit.point, outDir);
    dist2 = VECTORNORM2(outDir);
    dist = sqrt(dist2);
    VECTORSCALEINVERSE(dist, outDir, outDir);

    // pdf = 1 / A_pixel transformed to area measure

    cosa = VECTORDOTPRODUCT(thisNode->m_normal, outDir);

    // pdf = 1/Apix * (r^2 / cos(dir, eyeNormal) * (cos(dir, patchNormal) / d^2)
    //                 |__> to sper.angle           |__> to area on patch

    // Three cosines : r^2 / cos = 1 / cos^3 since r is length
    // of viewing ray to the pixel.
    pdf = 1.0 / (GLOBAL_camera_mainCamera.pixelHeight * GLOBAL_camera_mainCamera.pixelWidth * cosa * cosa * cosa);

    cosb = -VECTORDOTPRODUCT(newNode->m_normal, outDir);
    pdf = pdf * cosb / dist2;

    return pdf;
}

