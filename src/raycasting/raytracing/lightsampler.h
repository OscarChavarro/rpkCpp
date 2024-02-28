/**
Samples a point on a light source. Two implementations are given : uniform sampling and
importance sampling
*/

#ifndef __LIGHT_SAMPLER__
#define __LIGHT_SAMPLER__

#include "raycasting/raytracing/lightlist.h"
#include "raycasting/raytracing/sampler.h"

class CUniformLightSampler : public CNextEventSampler {
private:
    CLightList_Iter *iterator;
    Patch *currentPatch;
    bool unitsActive;
public:
    CUniformLightSampler();

    // Units, see sampler.H
    virtual bool ActivateFirstUnit();

    virtual bool ActivateNextUnit();

    virtual void DeactivateUnits();

    // Sample : newNode gets filled, others may change
    virtual bool
    Sample(
        CPathNode *prevNode,
        CPathNode *thisNode,
        CPathNode *newNode,
        double x1,
        double x2,
        bool doRR,
        BSDFFLAGS flags);

    virtual double
    EvalPDF(CPathNode *thisNode, CPathNode *newNode,
                           BSDFFLAGS flags,
                           double *pdf, double *pdfRR);
};

class CImportantLightSampler : public CNextEventSampler {
public:
    // Sample : newNode gets filled, others may change
    virtual bool
    Sample(
        CPathNode *prevNode,
        CPathNode *thisNode,
        CPathNode *newNode,
        double x1,
        double x2,
        bool doRR,
        BSDFFLAGS flags);

    virtual double
    EvalPDF(
        CPathNode *thisNode,
        CPathNode *newNode,
        BSDFFLAGS flags,
        double *pdf,
        double *pdfRR);

protected:
    Vector3D lastPoint, lastNormal;
};

#endif
