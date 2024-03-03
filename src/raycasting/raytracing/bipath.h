/* 
 * bipath.C: BiPath class. Just stores eyePath and lightPath, lengths and 
 * endnodes 
 */

#ifndef _BIPATH_H_
#define _BIPATH_H_

#include "raycasting/raytracing/bidiroptions.h"
#include "raycasting/common/pathnode.h"

class CBiPath {
public:
    CPathNode *m_eyePath;
    CPathNode *m_eyeEndNode;
    int m_eyeSize;

    CPathNode *m_lightPath;
    CPathNode *m_lightEndNode;
    int m_lightSize;

    double m_pdfLNE; // pdf for sampling the light point separately

    Vector3D m_dirEL;
    Vector3D m_dirLE;  // connection dir's

    double m_geomConnect;

    // Constructor
    CBiPath();

    void Init();

    // ReleasePaths: release all nodes from eye and light path
    void ReleasePaths();

    // Some interesting methods, only to be called for
    // valid bidirectional paths
    COLOR EvalRadiance();

    // Evaluate accumulated PDF of a bipath (no weighting)
    double EvalPDFAcc();

    // Evaluate weight/pdf for a bipath, taking into account other pdf's
    // depending on the config (bcfg).
    float EvalPDFAndWeight(BP_BASECONFIG *bcfg, float *pPdf = nullptr,
                            float *pWeight = nullptr);

};

#endif
