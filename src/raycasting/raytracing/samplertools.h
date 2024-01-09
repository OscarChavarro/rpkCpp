/* samplerconfig.H
 *
 * A configuration structure, that determines the sampling
 * procedure of a monte carlo ray tracing like algorithm
 */

#ifndef _SAMPLERTOOLS_H_
#define _SAMPLERTOOLS_H_

#include "raycasting/raytracing/sampler.h"
#include "raycasting/raytracing/pixelsampler.h"

class CSamplerConfig {
public:
    CSampler *pointSampler;  // Samples first point
    CSampler *dirSampler; // Samples first direction
    CSurfaceSampler *surfaceSampler; // Samples on surfaces
    CNextEventSampler *neSampler; // Samples last point separately for next event

    bool m_useQMC;
    int m_qmcDepth;
    unsigned *m_qmcSeed;

    int minDepth;
    int maxDepth;

    // methods

    // Constructor

    void ClearVars() {
        pointSampler = nullptr;
        dirSampler = nullptr;
        surfaceSampler = nullptr;
        neSampler = nullptr;
        m_qmcSeed = nullptr;
    }

    void ReleaseVars() {
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

    void Init(bool useQMC = false, int maxd = 0);

    CSamplerConfig() {
        ClearVars();
        Init();
    }

    CSamplerConfig(bool useQMC, int maxd) {
        ClearVars();
        Init(useQMC, maxd);
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

    CPathNode *TraceNode(CPathNode *nextNode,
                         double x_1, double x_2,
                         BSDFFLAGS flags);

    // photonMapTracePath : Traces a path using the samplers in the class
    // New nodes are allocated if necessary. TraceNode is used
    // for sampling individual nodes.
    // The first filled in node is returned (==nextNode if nextNode != nullptr)

    CPathNode *TracePath(CPathNode *nextNode, BSDFFLAGS flags = BSDF_ALL_COMPONENTS);


    // Generate two random numbers. Depth needed for QMC sampling
    void GetRand(int depth, double *x_1, double *x_2);

};

/*
 * PathNodeConnect : this is a flexible function for connecting
 * pathnodes.
 *
 * IN : 2 nodes are needed. nodeE and nodeL are going to be connected
 *      Visibility is NOT checked !
 *
 *      nodeE must be the node in an EYE subpath
 *      nodeL must be the node in a LIGHT subpath
 *
 *      config determines the samplers used to generate the paths.
 *      These samplers are needed for pdf evaluations
 *
 *      Flags determine what needs to be computed. See the flag definitions
 *
 * OUT : geom factor is returned (not filled in cause it would overwrite 
 *       other geom's !)
 */

typedef int CONNECTFLAGS;

#define CONNECT_EL 0x01  // Compute pdf(E->L) and bsdf(EP -> E -> L)
#define CONNECT_LE 0x02  // Compute pdf(L->E) and bsdf(LP -> L -> E)
// Note that if EP or LP are nullptr the meaning of bsdf may change !

#define FILL_OTHER_PDF 0x10  // if (CONNECT_EL) then also compute the pdf
// (when coming from nodeE) for generating the node the leads to nodeL.
// Same if (CONNECT_LE).

double PathNodeConnect(CPathNode *nodeX,
                       CPathNode *nodeY,
                       CSamplerConfig *eyeConfig,
                       CSamplerConfig *lightConfig,
                       CONNECTFLAGS flags,
                       BSDFFLAGS bsdfFlagsE = BSDF_ALL_COMPONENTS,
                       BSDFFLAGS bsdfFlagsL = BSDF_ALL_COMPONENTS,
                       Vector3D *p_dirEL = nullptr);

#endif // _SAMPLERTOOLS_H_
