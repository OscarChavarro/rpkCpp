/**
The real importance map storage
*/

#ifndef __IMPORTANCE_MAP__
#define __IMPORTANCE_MAP__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/raycasting/photonMap/PhotonMap.h"

class ImportanceMap: public PhotonMap {
  private:
    double m_maxImp;
    double m_avgImp;
    double m_totalMaxDistance;
    int m_preReconPhotons;
    double *m_impScalePtr;

  public:
    // Constructor: always use irradiance per computation
    ImportanceMap(PhotonMapState &photonMapState, int *estimate_nrp, double *impScalePtr) :
        PhotonMap(photonMapState, estimate_nrp, true),
        m_maxImp(),
        m_avgImp(),
        m_totalMaxDistance(),
        m_preReconPhotons()
    {
        m_impScalePtr = impScalePtr;
    }

    // Override some photon map functions
    bool addPhoton(Photon &photon, Vector3Dd normal, short flags) override;

    void photonPrecomputeIrradiance(Camera *camera, IrrPhoton *photon) override;
    void precomputeIrradiance() override;

    // New functions
    double reconstructImportance(Vector3Dd, const Vector3Dd &normal) const;
    double getImpReqDensity(const Camera *camera, const Vector3Dd &pos, const Vector3Dd &normal) const;
    double getRequiredDensity(const Camera *camera, Vector3Dd pos, Vector3Dd normal);

protected:
    void
    ComputeAllRequiredDensities(
        const Camera *camera,
        Vector3Dd &pos,
        const Vector3Dd &normal,
        double *imp,
        double *pot,
        double *diff);
};

#endif

#endif
