/**
The real importance map storage
*/

#ifndef __IMPORTANCE_MAP__
#define __IMPORTANCE_MAP__

#include "PHOTONMAP/photonmap.h"

// Importance map, derived from photon map
class CImportanceMap : public CPhotonMap {
  protected:
    float m_maxImp, m_avgImp;
    float m_totalMaxDistance;
    int m_preReconPhotons;
    float *m_impScalePtr;

  public:
    // Constructor: always use irradiance per computation
    CImportanceMap(int *estimate_nrp, float *impScalePtr) :
        CPhotonMap(estimate_nrp, true),
        m_maxImp(),
        m_avgImp(),
        m_totalMaxDistance(),
        m_preReconPhotons()
    {
        m_impScalePtr = impScalePtr;
    }

    // Override some photon map functions
    virtual bool
    AddPhoton(CPhoton &photon, Vector3D &normal, short flags);

    virtual void PhotonPrecomputeIrradiance(Camera *camera, CIrrPhoton *photon);
    virtual void PrecomputeIrradiance();

    // New functions
    virtual float ReconstructImp(Vector3D &pos, Vector3D &normal);
    virtual float GetImpReqDensity(Camera *camera, Vector3D &pos, Vector3D &normal);
    virtual float GetRequiredDensity(Camera *camera, Vector3D &pos, Vector3D &normal);

protected:
    void
    ComputeAllRequiredDensities(
        Camera *camera,
        Vector3D &pos,
        Vector3D &normal,
        float *imp,
        float *pot,
        float *diff);
};

#endif
