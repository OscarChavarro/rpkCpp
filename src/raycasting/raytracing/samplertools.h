/**
A configuration structure, that determines the sampling
procedure of a monte carlo ray tracing like algorithm
*/

#ifndef __SAMPLER_TOOLS__
#define __SAMPLER_TOOLS__

#include "raycasting/raytracing/sampler.h"
#include "raycasting/raytracing/pixelsampler.h"

class CSamplerConfig {
  public:
    Sampler *pointSampler;  // Samples first point
    Sampler *dirSampler; // Samples first direction
    CSurfaceSampler *surfaceSampler; // Samples on surfaces
    CNextEventSampler *neSampler; // Samples last point separately for next event

    bool m_useQMC;
    int m_qmcDepth;
    unsigned *m_qmcSeed;

    int minDepth;
    int maxDepth;

    // methods

    // Constructor

    void
    clearVars() {
        pointSampler = nullptr;
        dirSampler = nullptr;
        surfaceSampler = nullptr;
        neSampler = nullptr;
        m_qmcSeed = nullptr;
    }

    void
    releaseVars() {
        if ( pointSampler ) {
            delete pointSampler;
            pointSampler = nullptr;
        }
        if ( dirSampler ) {
            delete dirSampler;
            dirSampler = nullptr;
        }
        if ( surfaceSampler ) {
            delete surfaceSampler;
            surfaceSampler = nullptr;
        }
        if ( neSampler ) {
            delete neSampler;
            neSampler = nullptr;
        }
        if ( m_qmcSeed ) {
            delete[] m_qmcSeed;
            m_qmcSeed = nullptr;
        }
    }

    void init(bool useQMC = false, int maxD = 0);

    CSamplerConfig():
        pointSampler(), dirSampler(), surfaceSampler(), neSampler(), m_useQMC(), m_qmcDepth(), m_qmcSeed(),
        minDepth(), maxDepth()
    {
        clearVars();
        init();
    }

    ~CSamplerConfig() {
        if ( m_qmcSeed ) {
            delete[] m_qmcSeed;
        }
    }

    // TraceNode: trace a new node, given two random numbers
    // The correct sampler is chosen depending on the current
    // path depth.

    // nextNode is the next node to fill in.
    //   if nextNode = nullptr a new node is constructed and sampling
    //      starts with the point sampler.
    //   if nextNode != nullptr and nextNode->Previous() = nullptr then
    //      this is the first node and the point sampler is used first.
    //   if nextNode->Previous != nullptr then the depth of the previous
    //      node determines if the dirSampler (depth = 0) or the
    //      surfaceSampler is used (depth > 0)

    // RETURNS:
    //   if sampling ok: nextNode or a newly allocated node if nextNode == nullptr
    //   if sampling fails: nullptr

    SimpleRaytracingPathNode *
    traceNode(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *nextNode,
        double x1,
        double x2,
        BSDF_FLAGS flags) const;

    // photonMapTracePath : Traces a path using the samplers in the class
    // New nodes are allocated if necessary. TraceNode is used
    // for sampling individual nodes.
    // The first filled in node is returned (==nextNode if nextNode != nullptr)

    SimpleRaytracingPathNode *
    tracePath(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        SimpleRaytracingPathNode *nextNode,
        BSDF_FLAGS flags = BSDF_ALL_COMPONENTS);

    // Generate two random numbers. Depth needed for QMC sampling
    void getRand(int depth, double *x_1, double *x_2) const;

};

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
typedef int CONNECT_FLAGS;

#define CONNECT_EL 0x01  // Compute pdf(E->L) and bsdf(EP -> E -> L)
#define CONNECT_LE 0x02  // Compute pdf(L->E) and bsdf(LP -> L -> E)
// Note that if EP or LP are nullptr the meaning of bsdf may change !

#define FILL_OTHER_PDF 0x10  // if (CONNECT_EL) then also compute the pdf
// (when coming from nodeE) for generating the node the leads to nodeL.
// Same if (CONNECT_LE).

double
pathNodeConnect(
    Camera *camera,
    SimpleRaytracingPathNode *nodeX,
    SimpleRaytracingPathNode *nodeY,
    CSamplerConfig *eyeConfig,
    CSamplerConfig *lightConfig,
    CONNECT_FLAGS flags,
    BSDF_FLAGS bsdfFlagsE = BSDF_ALL_COMPONENTS,
    BSDF_FLAGS bsdfFlagsL = BSDF_ALL_COMPONENTS,
    Vector3D *pDirEl = nullptr);

#endif
