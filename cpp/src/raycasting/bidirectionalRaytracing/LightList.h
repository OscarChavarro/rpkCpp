/**
Definition of the light list class
this class can be used for sampling lights
*/

#ifndef LIGHT_LIST__
#define LIGHT_LIST__

#include "java/util/ArrayList.h"
#include "common/dataStructures/CircularList.h"
#include "raycasting/bidirectionalRaytracing/LightInfo.h"
#include "environment/geometry/elements/Patch.h"

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
    double evalPdf(Patch *light, const Vector3D *point) const;

    // Importance sampling routines
    Patch *sampleImportant(const Vector3D *point, const Vector3D *normal, double *x1, double *pdf);

    double evalPdfImportant(const Patch *light, const Vector3D *, const Vector3D *litPoint, const Vector3D *normal);

  private:
    Vector3D lastPoint;
    Vector3D lastNormal;

    void computeLightImportance(const Vector3D *point, const Vector3D *normal);

    static double
    computeOneLightImportance(
        const Patch *light,
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
        const Patch *light,
        const Vector3D *point,
        const Vector3D *normal,
        float emittedFlux);

    // specialisations by patch type (normal or virtual) of EvalPDF
    double evalPdfVirtual(const Patch *light, const Vector3D *) const;

    double evalPdfReal(Patch *light, const Vector3D *) const;
};

#include "raycasting/bidirectionalRaytracing/LightListIterator.h"

#endif
