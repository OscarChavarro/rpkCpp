/**
A configuration structure, that determines the sampling
procedure of a monte carlo ray tracing like algorithm
*/

#ifndef __SAMPLER_CONFIG__
#define __SAMPLER_CONFIG__

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/raytracing/Sampler.h"
#include "raycasting/raytracing/PixelSampler.h"
#include "raycasting/raytracing/SampleConnectionFlags.h"

typedef int CONNECT_FLAGS;

class SamplerConfig {
  public:
    Sampler *pointSampler;  // Samples first point
    Sampler *dirSampler; // Samples first direction
    SurfaceSampler *surfaceSampler; // Samples on surfaces
    NextEventSampler *neSampler; // Samples last point separately for next event

    bool m_useQMC;
    int m_qmcDepth;
    unsigned *m_qmcSeed;

    int minDepth;
    int maxDepth;

    void clearVars();
    void releaseVars();
    void init(bool useQMC = false, int maxD = 0);
    SamplerConfig();
    ~SamplerConfig();

    // TraceNode: trace a new node, given two random numbers
    // The correct sampler is chosen depending on the current
    // path depth.

    // nextNode is the next node to fill in.
    //   if nextNode = NULL a new node is constructed and sampling
    //      starts with the point sampler.
    //   if nextNode != NULL and nextNode->Previous() = NULL then
    //      this is the first node and the point sampler is used first.
    //   if nextNode->Previous != NULL then the depth of the previous
    //      node determines if the dirSampler (depth = 0) or the
    //      surfaceSampler is used (depth > 0)

    // RETURNS:
    //   if sampling ok: nextNode or a newly allocated node if nextNode == NULL
    //   if sampling fails: NULL

    SimpleRaytracingPathNode *
    traceNode(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *nextNode,
        double x1,
        double x2,
        char flags) const;

    // photonMapTracePath : Traces a path using the samplers in the class
    // New nodes are allocated if necessary. TraceNode is used
    // for sampling individual nodes.
    // The first filled in node is returned (==nextNode if nextNode != NULL)

    SimpleRaytracingPathNode *
    tracePath(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *nextNode,
        char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS);

    // Generate two random numbers. Depth needed for QMC sampling
    void getRand(int depth, double *x1, double *x2) const;

    static double
    pathNodeConnect(
        Camera *camera,
        SimpleRaytracingPathNode *nodeX,
        SimpleRaytracingPathNode *nodeY,
        SamplerConfig *eyeConfig,
        SamplerConfig *lightConfig,
        CONNECT_FLAGS flags,
        char bsdfFlagsE = BsdfComponentInfo::BSDF_ALL_COMPONENTS,
        char bsdfFlagsL = BsdfComponentInfo::BSDF_ALL_COMPONENTS,
        Vector3D *pDirEl = NULL);

};

inline void
SamplerConfig::clearVars() {
    pointSampler = NULL;
    dirSampler = NULL;
    surfaceSampler = NULL;
    neSampler = NULL;
    m_qmcSeed = NULL;
}

inline void
SamplerConfig::releaseVars() {
    if ( pointSampler ) {
        delete pointSampler;
        pointSampler = NULL;
    }
    if ( dirSampler ) {
        delete dirSampler;
        dirSampler = NULL;
    }
    if ( surfaceSampler ) {
        delete surfaceSampler;
        surfaceSampler = NULL;
    }
    if ( neSampler ) {
        delete neSampler;
        neSampler = NULL;
    }
    if ( m_qmcSeed ) {
        delete[] m_qmcSeed;
        m_qmcSeed = NULL;
    }
}

inline SamplerConfig::SamplerConfig():
    pointSampler(), dirSampler(), surfaceSampler(), neSampler(), m_useQMC(), m_qmcDepth(), m_qmcSeed(),
    minDepth(), maxDepth()
{
    clearVars();
    init();
}

inline
SamplerConfig::~SamplerConfig() {
    if ( m_qmcSeed ) {
        delete[] m_qmcSeed;
    }
}

/**
pathNodeConnect : this is a flexible function for connecting
path nodes.

IN : 2 nodes are needed. nodeE and nodeL are going to be connected
     Visibility is NOT checked !

     nodeE must be the node in an EYE sub-path
     nodeL must be the node in a LIGHT sub-path

     config determines the samplers used to generate the paths.
     These samplers are needed for pdf evaluations

     Flags determine what needs to be computed. See the flag definitions

OUT : geometry factor is returned (not filled in cause it would overwrite
      other geometries !)
*/
#endif

#endif
