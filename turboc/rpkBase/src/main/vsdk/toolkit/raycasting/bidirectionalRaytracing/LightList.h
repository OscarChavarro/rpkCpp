/**
Definition of the light list class
this class can be used for sampling lights
*/

#ifndef __LIGHT_LIST__
#define __LIGHT_LIST__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/dataStructures/CircularList.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightInfo.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class LightList: private CircularList<LightInfo>{ private:
    // Total flux ( sum(L * A * PI))
    double totalFlux;
    double totalImp;
    bool includeVirtual;
    int lightCount;

  public:
    // Iteration over lights, not multi-thread!

    // Discrete sampling of light sources

    // A getPatchList must be supplied for building a light list.
    // Non emitting patches (edf == NULL) are NOT put in the list.
    explicit LightList(const ArrayList<Patch *> *list, bool includeVirtualPatches = false);

    ~LightList();
    CircularList<LightInfo> &entries();

    // Normal sampling: uniform over emitted power
    Patch *sample(double *x1, double *pdf);

    // Normal PDF evaluation: uniform over emitted power
    double evalPdf(Patch *light, const Vector3Dd *point) const;

    // Importance sampling routines
    Patch *sampleImportant(const Vector3Dd *point, const Vector3Dd *normal, double *x1, double *pdf);

    double evalPdfImportant(const Patch *light, const Vector3Dd *, const Vector3Dd *litPoint, const Vector3Dd *normal);

  private:
    Vector3Dd lastPoint;
    Vector3Dd lastNormal;

    void computeLightImportance(const Vector3Dd *point, const Vector3Dd *normal);

    static double
    computeOneLightImportance( const Patch *light, const Vector3Dd *point, const Vector3Dd *normal, double emittedFlux);

    // Specialisations by patch type (normal or virtual) of ComputeOneLightImportance
    static double
    cmptOneLightImpVrtl( const Patch *light, const Vector3Dd *, const Vector3Dd *, double);

    static double
    computeOneLightImportanceReal( const Patch *light, const Vector3Dd *point, const Vector3Dd *normal, double emittedFlux);

    // specialisations by patch type (normal or virtual) of EvalPDF
    double evalPdfVirtual(const Patch *light, const Vector3Dd *) const;

    double evalPdfReal(Patch *light, const Vector3Dd *) const;
};

#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightListIterator.h"

#endif
