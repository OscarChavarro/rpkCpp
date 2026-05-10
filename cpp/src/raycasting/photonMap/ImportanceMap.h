/**
The real importance map storage
*/

#ifndef __IMPORTANCE_MAP__
#define __IMPORTANCE_MAP__

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/photonMap/PhotonMap.h"

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
    bool addPhoton(Photon &photon, Vector3D normal, short flags) override;

    void photonPrecomputeIrradiance(Camera *camera, IrrPhoton *photon) override;
    void precomputeIrradiance() override;

    // New functions
    float reconstructImportance(Vector3D, const Vector3D &normal) const;
    float getImpReqDensity(const Camera *camera, const Vector3D &pos, const Vector3D &normal) const;
    float getRequiredDensity(const Camera *camera, Vector3D pos, Vector3D normal);

protected:
    void
    ComputeAllRequiredDensities(
        const Camera *camera,
        Vector3D &pos,
        const Vector3D &normal,
        float *imp,
        float *pot,
        float *diff);
};

#endif

#endif
