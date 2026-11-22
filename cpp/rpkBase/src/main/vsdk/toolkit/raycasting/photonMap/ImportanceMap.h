/**
The real importance map storage
*/

#ifndef IMPORTANCE_MAP__
#define IMPORTANCE_MAP__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/raycasting/photonMap/PhotonMap.h"

class ImportanceMap: public PhotonMap {
  private:
    float m_maxImp;
    float m_avgImp;
    float m_totalMaxDistance;
    int m_preReconPhotons;
    float *m_impScalePtr;

  public:
    // Constructor: always use irradiance per computation
    ImportanceMap(PhotonMapState &photonMapState, int *estimate_nrp, float *impScalePtr) :
        PhotonMap(photonMapState, estimate_nrp, true),
        m_maxImp(),
        m_avgImp(),
        m_totalMaxDistance(),
        m_preReconPhotons()
    {
        m_impScalePtr = impScalePtr;
    }

    // Override some photon map functions
    bool addPhoton(Photon &photon, Vector3Df normal, short flags) override;

    void photonPrecomputeIrradiance(Camera *camera, IrrPhoton *photon) override;
    void precomputeIrradiance() override;

    // New functions
    float reconstructImportance(Vector3Df, const Vector3Df &normal) const;
    float getImpReqDensity(const Camera *camera, const Vector3Df &pos, const Vector3Df &normal) const;
    float getRequiredDensity(const Camera *camera, Vector3Df pos, Vector3Df normal);

protected:
    void
    ComputeAllRequiredDensities(
        const Camera *camera,
        Vector3Df &pos,
        const Vector3Df &normal,
        float *imp,
        float *pot,
        float *diff);
};

#endif

#endif
