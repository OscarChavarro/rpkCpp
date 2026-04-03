#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED
#include "common/RenderOptions.h"
#include "photonMap/ScreenSampler.h"
#include "raycasting/common/Raytools.h"

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
    char /* flags */)
{
    Vector3D dir;

    // Pre-condition2: 1. thisNode == eye 2. prevNode == nullptr 3. SetPixel called

    // Sample direction
    double xSample = camera->pixelWidth * static_cast<double>(camera->xSize) * (-0.5 + x1);
    double ySample = camera->pixelHeight * static_cast<double>(camera->ySize) * (-0.5 + x2);

    dir.combine3(camera->Z, static_cast<float>(xSample), camera->X, static_cast<float>(ySample), camera->Y);
    double distScreen2 = dir.norm2();
    double distScreen = java::Math::sqrt(distScreen2);
    dir.inverseScaledCopy(static_cast<float>(distScreen), dir, Numeric::EPSILON_FLOAT);

    double cosScreen = java::Math::abs(camera->Z.dotProduct(dir));

    double pdfDir = ((1.0 / (camera->pixelWidth * static_cast<float>(camera->xSize) *
                            camera->pixelHeight * static_cast<float>(camera->ySize))) * // 1 / Area pixel
                     (distScreen2 / cosScreen));  // Spherical angle measure

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
    thisNode->m_usedComponents = XxdfComponentFlagInfo::NO_COMPONENTS; // The eye...
    newNode->m_accUsedComponents = static_cast<char>(thisNode->m_accUsedComponents | thisNode->m_usedComponents);
    return true;
}

double
ScreenSampler::evalPDF(
    Camera *camera,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    char /*flags*/,
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
    outDir.subtraction(newNode->m_hit.getPoint(), thisNode->m_hit.getPoint());
    dist2 = outDir.norm2();
    dist = java::Math::sqrt(dist2);
    outDir.inverseScaledCopy(static_cast<float>(dist), outDir, Numeric::EPSILON_FLOAT);

    // probabilityDensityFunction = 1 / A_screen transformed to area measure
    cosA = thisNode->m_normal.dotProduct(outDir);

    // probabilityDensityFunction = 1/Apix * (r^2 / cos(dir, eyeNormal) * (cos(dir, patchNormal) / d^2)
    //                 |__> to spherical angle           |__> to area on patch

    // Three cosines : r^2 / cos = 1 / cos^3 since r is length
    // of viewing ray to the screen.
    probabilityDensityFunction = 1.0 /
        (camera->pixelHeight * static_cast<float>(camera->ySize) * camera->pixelWidth * static_cast<float>(camera->xSize) * cosA * cosA * cosA);

    cosB = -newNode->m_normal.dotProduct(outDir);
    probabilityDensityFunction = probabilityDensityFunction * cosB / dist2;

    return probabilityDensityFunction;
}

#endif

