/*
 * lightlist.H : definition of the lightlist class
 *  this class can be used for sampling lights
 */

#ifndef _LIGHTLIST_H_
#define _LIGHTLIST_H_

#include "skin/PatchSet.h"
#include "common/linealAlgebra/vectorMacros.h"

#include "common/dataStructures/CSList.h"

class CLightInfo {
public:
    float emittedFlux;
    float importance; // cumlative probability : for importance sampling
    Patch *light;
};


class CLightList_Iter;

class CLightList : private CTSList<CLightInfo> {
private:
    // Total flux ( sum(L * A * PI))
    float totalFlux, totalImp;
    bool includeVirtual;

    int lightCount;

    CTSList_Iter<CLightInfo> *current;

public:

    // Iteration over lights, not multithreaded!!

    // Discrete sampling of lightsources

    // A getPatchList must be supplied for building a light list.
    // Non emitting patches (edf == nullptr) are NOT put in the list.
    CLightList(PatchSet *list, bool includeVirtualPatches = false);

    ~CLightList();

    // mutator of includeVirtual
    void IncludeVirtualPatches(bool newValue);

    // Normal sampling : uniform over emitted power
    Patch *Sample(double *x_1, double *pdf);


    // Normal PDF evaluation : uniform over emitted power
    double EvalPDF(Patch *light, Vector3D *point);

    // Importance sampling routines

    Patch *SampleImportant(Vector3D *point, Vector3D *normal, double *x_1,
                           double *pdf);

    double EvalPDFImportant(Patch *light, Vector3D *lightPoint,
                            Vector3D *litPoint, Vector3D *normal);

protected:
    Vector3D lastPoint, lastNormal;

    void ComputeLightImportances(Vector3D *point, Vector3D *normal);

    double ComputeOneLightImportance(Patch *light, const Vector3D *point,
                                     const Vector3D *normal,
                                     float avgEmittedRadiance);


    // specialisations by patch type (normal or virtual) of ComputeOneLightImportance
    double ComputeOneLightImportance_virtual(Patch *light, const Vector3D *point,
                                             const Vector3D *normal,
                                             float avgEmittedRadiance);

    double ComputeOneLightImportance_real(Patch *light, const Vector3D *point,
                                          const Vector3D *normal,
                                          float avgEmittedRadiance);

    // specialisations by patch type (normal or virtual) of EvalPDF
    double EvalPDF_virtual(Patch *light, Vector3D *point);

    double EvalPDF_real(Patch *light, Vector3D *point);

    friend class CLightList_Iter;
};


class CLightList_Iter {
private:
    CTSList_Iter<CLightInfo> m_iter;
public:
    CLightList_Iter(CLightList &list) : m_iter(list) {}

    Patch *First(CLightList &list) {
        m_iter.Init(list);

        CLightInfo *li = m_iter.Next();
        if ( li != nullptr ) {
            return li->light;
        } else {
            return nullptr;
        }
    }

    Patch *Next() {
        CLightInfo *li = m_iter.Next();
        if ( li ) {
            return li->light;
        } else {
            return nullptr;
        }
    }
};

// Global var for the scene light list
extern CLightList *GLOBAL_lightList;

#endif
