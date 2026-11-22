/**
Definition of the light list class
this class can be used for sampling lights
*/

#ifndef LIGHT_LIST__
#define LIGHT_LIST__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/dataStructures/CircularList.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightInfo.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class LightList final : private CircularList<LightInfo> {
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
    explicit LightList(const java::ArrayList<Patch *> *list, bool includeVirtualPatches = false);

    ~LightList() final;
    CircularList<LightInfo> &entries();

    // Normal sampling : uniform over emitted power
    Patch *sample(double *x1, double *pdf);

    // Normal PDF evaluation : uniform over emitted power
    double evalPdf(const Patch *light, const Vector3Df *point) const;

    // Importance sampling routines
    Patch *sampleImportant(const Vector3Df *point, const Vector3Df *normal, double *x1, double *pdf);

    double evalPdfImportant(const Patch *light, const Vector3Df *, const Vector3Df *litPoint, const Vector3Df *normal);

  private:
    Vector3Df lastPoint;
    Vector3Df lastNormal;

    void computeLightImportance(const Vector3Df *point, const Vector3Df *normal);

    static double
    computeOneLightImportance(
        const Patch *light,
        const Vector3Df *point,
        const Vector3Df *normal,
        float emittedFlux);

    // Specialisations by patch type (normal or virtual) of ComputeOneLightImportance
    static double
    computeOneLightImportanceVirtual(
        const Patch *light,
        const Vector3Df *,
        const Vector3Df *,
        float);

    static double
    computeOneLightImportanceReal(
        const Patch *light,
        const Vector3Df *point,
        const Vector3Df *normal,
        float emittedFlux);

    // specialisations by patch type (normal or virtual) of EvalPDF
    double evalPdfVirtual(const Patch *light, const Vector3Df *) const;

    double evalPdfReal(const Patch *light, const Vector3Df *) const;
};

#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightListIterator.h"

#endif
