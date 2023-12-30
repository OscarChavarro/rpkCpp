/*
 * lightsampler.H : Samples a point on a light source.
 *   Two implementations are given : uniform sampling and
 *   importance sampling
 */

#ifndef _LIGHTSAMPLER_H_
#define _LIGHTSAMPLER_H_

#include "shared/lightlist.h"
#include "raycasting/raytracing/sampler.h"

class CUniformLightSampler : public CNextEventSampler {
private:
    CLightList_Iter *iterator;
    PATCH *currentPatch;
    bool unitsActive;
public:
    CUniformLightSampler();

    // Units, see sampler.H
    virtual bool ActivateFirstUnit();

    virtual bool ActivateNextUnit();

    virtual void DeactivateUnits();

    // Sample : newNode gets filled, others may change
    virtual bool Sample(CPathNode *prevNode, CPathNode *thisNode,
                        CPathNode *newNode, double x_1, double x_2,
                        bool doRR = false,
                        BSDFFLAGS flags = BSDF_ALL_COMPONENTS);

    virtual double EvalPDF(CPathNode *thisNode, CPathNode *newNode,
                           BSDFFLAGS flags = BSDF_ALL_COMPONENTS,
                           double *pdf = nullptr, double *pdfRR = nullptr);
};

class CImportantLightSampler : public CNextEventSampler {
public:
    // Sample : newNode gets filled, others may change
    virtual bool Sample(CPathNode *prevNode, CPathNode *thisNode,
                        CPathNode *newNode, double x_1, double x_2,
                        bool doRR = false,
                        BSDFFLAGS flags = BSDF_ALL_COMPONENTS);

    virtual double EvalPDF(CPathNode *thisNode, CPathNode *newNode,
                           BSDFFLAGS flags = BSDF_ALL_COMPONENTS,
                           double *pdf = nullptr, double *pdfRR = nullptr);

protected:
    Vector3D lastPoint, lastNormal;
};

#endif
