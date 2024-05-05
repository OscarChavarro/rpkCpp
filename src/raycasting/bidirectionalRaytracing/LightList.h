/**
Definition of the light list class
this class can be used for sampling lights
*/

#ifndef __LIGHT_LIST__
#define __LIGHT_LIST__

#include "java/util/ArrayList.h"
#include "common/dataStructures/CircularList.h"
#include "skin/Patch.h"

class LightInfo {
public:
    float emittedFlux;
    float importance; // Cumulative probability : for importance sampling
    Patch *light;
};


class LightListIterator;

class LightList : private CTSList<LightInfo> {
private:
    // Total flux ( sum(L * A * PI))
    float totalFlux;
    float totalImp;
    bool includeVirtual;
    int lightCount;

public:
    // Iteration over lights, not multi-thread!

    // Discrete sampling of light sources

    // A getPatchList must be supplied for building a light list.
    // Non emitting patches (edf == nullptr) are NOT put in the list.
    explicit LightList(java::ArrayList<Patch *> *list, bool includeVirtualPatches = false);

    ~LightList();

    // Normal sampling : uniform over emitted power
    Patch *sample(double *x1, double *pdf);

    // Normal PDF evaluation : uniform over emitted power
    double evalPdf(Patch *light, const Vector3D *point);

    // Importance sampling routines
    Patch *sampleImportant(Vector3D *point, Vector3D *normal, double *x1, double *pdf);

    double evalPdfImportant(Patch *light, Vector3D *, Vector3D *litPoint, Vector3D *normal);

protected:
    Vector3D lastPoint;
    Vector3D lastNormal;

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
        const Patch *light,
        const Vector3D *,
        const Vector3D *,
        float);

    static double
    computeOneLightImportanceReal(
        Patch *light,
        const Vector3D *point,
        const Vector3D *normal,
        float emittedFlux);

    // specialisations by patch type (normal or virtual) of EvalPDF
    double evalPdfVirtual(const Patch *light, const Vector3D *) const;

    double evalPdfReal(Patch *light, const Vector3D *) const;

    friend class LightListIterator;
};

class LightListIterator {
  private:
    CTSList_Iter<LightInfo> iterator;
  public:
    explicit LightListIterator(LightList &list) : iterator(list) {}

    Patch *
    First(LightList &list) {
        iterator.init(list);

        LightInfo *li = iterator.nextOnSequence();
        if ( li != nullptr ) {
            return li->light;
        } else {
            return nullptr;
        }
    }

    Patch *
    Next() {
        LightInfo *li = iterator.nextOnSequence();
        if ( li ) {
            return li->light;
        } else {
            return nullptr;
        }
    }
};

// Global var for the scene light list
extern LightList *GLOBAL_lightList;

#endif
