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
    float totalFlux;
    float totalImp;
    bool includeVirtual;

    int lightCount;

public:

    // Iteration over lights, not multi-threaded!

    // Discrete sampling of light sources

    // A getPatchList must be supplied for building a light list.
    // Non emitting patches (edf == nullptr) are NOT put in the list.
    CLightList(PatchSet *list, bool includeVirtualPatches = false);

    ~CLightList();

    // Normal sampling : uniform over emitted power
    Patch *sample(double *x1, double *pdf);

    // Normal PDF evaluation : uniform over emitted power
    double evalPdf(Patch *light, Vector3D *point);

    // Importance sampling routines
    Patch *sampleImportant(Vector3D *point, Vector3D *normal, double *x1, double *pdf);

    double evalPdfImportant(Patch *light, Vector3D *, Vector3D *litPoint, Vector3D *normal);

protected:
    Vector3D lastPoint, lastNormal;

    void computeLightImportance(Vector3D *point, Vector3D *normal);

    static double
    computeOneLightImportance(
        Patch *light,
        const Vector3D *point,
        const Vector3D *normal,
        float emittedFlux);

    // Specialisations by patch type (normal or virtual) of ComputeOneLightImportance
    static double
    computeOneLightImportanceVirtual(
        Patch *light,
        const Vector3D *,
        const Vector3D *,
        float);

    static double computeOneLightImportanceReal(Patch *light, const Vector3D *point,
                                         const Vector3D *normal,
                                         float emittedFlux);

    // specialisations by patch type (normal or virtual) of EvalPDF
    double evalPdfVirtual(Patch *light, Vector3D *) const;

    double evalPdfReal(Patch *light, Vector3D *) const;

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
