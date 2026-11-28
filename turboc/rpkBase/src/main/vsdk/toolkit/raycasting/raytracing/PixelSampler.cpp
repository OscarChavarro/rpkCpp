#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/raycasting/common/Raytools.h"
#include "vsdk/toolkit/raycasting/raytracing/PixelSampler.h"

bool
PixelSampler::sample(
    Camera *camera,
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneSceneQueryWorkspace,
    Background *sceneBackground,
    SimpleRaytracingPathNode */*prevNode*/,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool /*doRR*/,
    char /*flags*/)
{
    Vector3Dd dir;

    // Pre-conditions: 1. thisNode == eye 2. prevNode == NULL 3. SetPixel called

    // Sample direction
    double xSample = (m_px + camera->pixelWidth * x1);
    double ySample = (m_py + camera->pixelHeight * x2);

    dir = camera->Z.add(camera->X.multiply(((double)(xSample)))).add(camera->Y.multiply(((double)(ySample))));
    double distPixel2 = dir.lengthSquared();
    double distPixel = Math::sqrt(distPixel2);
    dir = dir.multiplyByInverse(((double)(distPixel)), Numeric::EPSILON);

    double cosPixel = Math::abs(camera->Z.dotProduct(dir));

    double pdfDir = ((1.0 / (camera->pixelWidth * camera->pixelHeight)) * // 1 / Area pixel
                     (distPixel2 / cosPixel));  // Spherical angle measure

    // Determine ray type
    thisNode->m_rayType = STARTS;
    newNode->m_inBsdf = thisNode->m_outBsdf; // Camera can be placed in a medium

    // Transfer
    if ( !sampleTransfer(sceneQuery, sceneSceneQueryWorkspace, sceneBackground, thisNode, newNode, &dir, pdfDir) ) {
        thisNode->m_rayType = STOPS;
        return false;
    }

    // "Bsdf" in thisNode

    // Potential is one for all directions through a pixel
    thisNode->m_bsdfEval = ColorRgb(1.0, 1.0, 1.0);

    // Make sure evaluation of eye components always includes the diff ref.
    thisNode->m_bsdfComp.Clear();
    thisNode->m_bsdfComp.Fill(thisNode->m_bsdfEval, BRDF_DIFFUSE_COMPONENT);

    // Component propagation
    thisNode->m_usedComponents = XxdfComponentFlagInfo::NO_COMPONENTS; // the eye...
    newNode->m_accUsedComponents = ((char)(thisNode->m_accUsedComponents | thisNode->m_usedComponents));

    newNode->accmlRssnRlttFctrs = thisNode->accmlRssnRlttFctrs; // No russian roulette

    return true;
}

void
PixelSampler::SetPixel(const Camera *defaultCamera, int nx, int ny, const Camera *camera) {
    if ( camera == NULL ) {
        // Primary camera
        camera = defaultCamera;
    }

    m_px = -camera->pixelWidth * ((double)(camera->xSize)) / 2.0 + ((double)(nx)) * camera->pixelWidth;
    m_py = -camera->pixelHeight * ((double)(camera->ySize)) / 2.0 + ((double)(ny)) * camera->pixelHeight;
}

double
PixelSampler::evalPDF(
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
    Vector3Dd outDir;

    // More efficient with extra params?
    outDir = (newNode->m_hit.getPoint()).subtract(thisNode->m_hit.getPoint());
    dist2 = outDir.lengthSquared();
    dist = Math::sqrt(dist2);
    outDir = outDir.multiplyByInverse(((double)(dist)), Numeric::EPSILON);

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
