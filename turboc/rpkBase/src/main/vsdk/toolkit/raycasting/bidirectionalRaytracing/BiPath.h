/**
Stores eyePath and lightPath, lengths and end nodes
*/

#ifndef __BI_PATH__
#define __BI_PATH__

#include "vsdk/toolkit/raycasting/common/SimpleRaytracingPathNode.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"

class BiPath {
  public:
    SimpleRaytracingPathNode *m_eyePath;
    SimpleRaytracingPathNode *m_eyeEndNode;
    int m_eyeSize;

    SimpleRaytracingPathNode *m_lightPath;
    SimpleRaytracingPathNode *m_lightEndNode;
    int m_lightSize;

    double m_pdfLNE; // pdf for sampling the light point separately

    Vector3Dd m_dirEL;
    Vector3Dd m_dirLE;  // connection dir's

    double m_geomConnect;

    // Constructor
    BiPath();

    void init();

    // Some interesting methods, only to be called for
    // valid bidirectional paths
    ColorRgb evalRadiance() const;

    // Evaluate accumulated PDF of a bipath (no weighting)
    double evalPdfAcc() const;

    // Evaluate weight/pdf for a bipath, taking into account other pdf's
    // depending on the config (baseConfig).
    double
    evalPdfAndWeight(
        const BidirPathRaytrcCnfg *baseConfig,
        double *pPdf = NULL,
        double *pWeight = NULL) const;
};

#endif
