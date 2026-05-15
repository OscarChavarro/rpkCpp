#ifndef Phngm
#define Phngm

#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"
#include "material/ShadingContext.h"
#include "environment/geometry/elements/RayHit.h"

/**
Emittance Distribution Functions: the self-emitted radiance
distribution of light sources
*/
class PhongEmitDistFunc {
  private:
    ColorRgb Kd;
    ColorRgb kd;
    ColorRgb Ks;
    float Ns;

    bool isSpecular() const;

public:
    explicit PhongEmitDistFunc(const ColorRgb *KdParameter, const ColorRgb *KsParameter, double NsParameter);
    virtual ~PhongEmitDistFunc();

    const ColorRgb &getKd() const;
    const ColorRgb &getKs() const;
    float getNs() const;

    static bool edfIsTextured();

    static bool
    edfShadingFrame(
        const ShadingContext &context,
        const Vector3D *X,
        const Vector3D *Y,
        const Vector3D *Z);

    static bool
    edfShadingFrame(
        const RayHit *hit,
        const Vector3D *X,
        const Vector3D *Y,
        const Vector3D *Z);

    ColorRgb phongEmittance(const ShadingContext &context, char flags) const;
    ColorRgb phongEmittance(const RayHit * /*hit*/, char flags) const;

    ColorRgb
    phongEdfEval(
        const ShadingContext &context,
        const Vector3D *out,
        char flags,
        double *probabilityDensityFunction) const;

    ColorRgb
    phongEdfEval(
        RayHit *hit,
        const Vector3D *out,
        char flags,
        double *probabilityDensityFunction) const;

    Vector3D
    phongEdfSample(
        const ShadingContext &context,
        char flags,
        double xi1,
        double xi2,
        ColorRgb *selfEmittedRadiance,
        double *probabilityDensityFunction) const ;

    Vector3D
    phongEdfSample(
        RayHit *hit,
        char flags,
        double xi1,
        double xi2,
        ColorRgb *selfEmittedRadiance,
        double *probabilityDensityFunction) const ;
};

inline const ColorRgb &
PhongEmitDistFunc::getKd() const {
    return Kd;
}

inline const ColorRgb &
PhongEmitDistFunc::getKs() const {
    return Ks;
}

inline float
PhongEmitDistFunc::getNs() const {
    return Ns;
}

#endif
