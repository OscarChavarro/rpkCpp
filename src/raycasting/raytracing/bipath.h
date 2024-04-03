/**
Stores eyePath and lightPath, lengths and end nodes
*/

#ifndef __BI_PATH__
#define __BI_PATH__

#include "raycasting/raytracing/bidiroptions.h"
#include "raycasting/common/pathnode.h"

class CBiPath {
public:
    SimpleRaytracingPathNode *m_eyePath;
    SimpleRaytracingPathNode *m_eyeEndNode;
    int m_eyeSize;

    SimpleRaytracingPathNode *m_lightPath;
    SimpleRaytracingPathNode *m_lightEndNode;
    int m_lightSize;

    double m_pdfLNE; // pdf for sampling the light point separately

    Vector3D m_dirEL;
    Vector3D m_dirLE;  // connection dir's

    double m_geomConnect;

    // Constructor
    CBiPath();

    void Init();

    // Some interesting methods, only to be called for
    // valid bidirectional paths
    ColorRgb EvalRadiance();

    // Evaluate accumulated PDF of a bipath (no weighting)
    double EvalPDFAcc();

    // Evaluate weight/pdf for a bipath, taking into account other pdf's
    // depending on the config (bcfg).
    float EvalPDFAndWeight(BP_BASECONFIG *bcfg, float *pPdf = nullptr,
                            float *pWeight = nullptr);

};

#endif
