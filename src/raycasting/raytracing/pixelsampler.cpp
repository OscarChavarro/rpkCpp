#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include <cmath>

#include "java/lang/Math.h"
#include "scene/Camera.h"
#include "raycasting/common/raytools.h"
#include "raycasting/raytracing/pixelsampler.h"

bool
CPixelSampler::sample(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool doRR,
    char flags)
{
    Vector3D dir;

    // Pre-conditions: 1. thisNode == eye 2. prevNode == nullptr 3. SetPixel called

    // Sample direction
    double xSample = (m_px + camera->pixelWidth * x1);
    double ySample = (m_py + camera->pixelHeight * x2);

    dir.combine3(camera->Z, (float) xSample, camera->X, (float) ySample, camera->Y);
    double distPixel2 = dir.norm2();
    double distPixel = java::Math::sqrt(distPixel2);
    dir.inverseScaledCopy((float) distPixel, dir, Numeric::EPSILON_FLOAT);

    double cosPixel = java::Math::abs(camera->Z.dotProduct(dir));

    double pdfDir = ((1.0 / (camera->pixelWidth * camera->pixelHeight)) * // 1 / Area pixel
                     (distPixel2 / cosPixel));  // Spherical angle measure

    // Determine ray type
    thisNode->m_rayType = PathRayType::STARTS;
    newNode->m_inBsdf = thisNode->m_outBsdf; // Camera can be placed in a medium

    // Transfer
    if ( !sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, &dir, pdfDir) ) {
        thisNode->m_rayType = PathRayType::STOPS;
        return false;
    }

    // "Bsdf" in thisNode

    // Potential is one for all directions through a pixel
    thisNode->m_bsdfEval.setMonochrome(1.0);

    // Make sure evaluation of eye components always includes the diff ref.
    thisNode->m_bsdfComp.Clear();
    thisNode->m_bsdfComp.Fill(thisNode->m_bsdfEval, BRDF_DIFFUSE_COMPONENT);

    // Component propagation
    thisNode->m_usedComponents = NO_COMPONENTS; // the eye...
    newNode->m_accUsedComponents = static_cast<char>(thisNode->m_accUsedComponents | thisNode->m_usedComponents);

    newNode->accumulatedRussianRouletteFactors = thisNode->accumulatedRussianRouletteFactors; // No russian roulette

    return true;
}

void
CPixelSampler::SetPixel(const Camera *defaultCamera, int nx, int ny, const Camera *camera) {
    if ( camera == nullptr ) {
        // Primary camera
        camera = defaultCamera;
    }

    m_px = -camera->pixelWidth * (double)camera->xSize / 2.0 + (double)nx * camera->pixelWidth;
    m_py = -camera->pixelHeight * (double)camera->ySize / 2.0 + (double)ny * camera->pixelHeight;
}

double
CPixelSampler::evalPDF(
    Camera *camera,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    char /*flags*/,
    double * /*pdf*/,
    double * /*pdfRR*/)
{
    double dist2;
    double dist;
    double cosA;
    double cosB;
    double pdf;
    Vector3D outDir;

    // More efficient with extra params?
    outDir.subtraction(newNode->m_hit.getPoint(), thisNode->m_hit.getPoint());
    dist2 = outDir.norm2();
    dist = java::Math::sqrt(dist2);
    outDir.inverseScaledCopy((float) dist, outDir, Numeric::EPSILON_FLOAT);

    // pdf = 1 / A_pixel transformed to area measure

    cosA = thisNode->m_normal.dotProduct(outDir);

    // pdf = 1/APixel * (r^2 / cos(dir, eyeNormal) * (cos(dir, patchNormal) / d^2)
    //                 |__> to spherical.angle           |__> to area on patch

    // Three cosines : r^2 / cos = 1 / cos^3 since r is length
    // of viewing ray to the pixel.
    pdf = 1.0 / (camera->pixelHeight * camera->pixelWidth * cosA * cosA * cosA);

    cosB = -newNode->m_normal.dotProduct(outDir);
    pdf = pdf * cosB / dist2;

    return pdf;
}

#endif
