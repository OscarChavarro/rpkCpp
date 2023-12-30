/*
importancemap.H: the real importance map storage
*/

#ifndef _IMPORTANCEMAP_H_
#define _IMPORTANCEMAP_H_

#include "PHOTONMAP/photonmap.h"

// Importance map, derived from photonmap

class CImportanceMap : public CPhotonMap {
protected:
    float m_maxImp, m_avgImp;
    float m_totalMaxDistance;
    int m_preReconPhotons;
    float *m_impScalePtr;
public:
    // Constructor: always use irradiance percomputation
    CImportanceMap(int *estimate_nrp, float *impScalePtr) : CPhotonMap(estimate_nrp, true) {
        m_impScalePtr = impScalePtr;
    }

    // Override some photonmap functions

    virtual bool AddPhoton(CPhoton &photon, Vector3D &normal,
                           short flags = 0);

    virtual void PhotonPrecomputeIrradiance(CIrrPhoton *photon);

    virtual void PrecomputeIrradiance();

    // New functions

    virtual float ReconstructImp(Vector3D &pos, Vector3D &normal);

    virtual float GetImpReqDensity(Vector3D &pos, Vector3D &normal);

    virtual float GetRequiredDensity(Vector3D &pos, Vector3D &normal);

protected:
    void ComputeAllRequiredDensities(Vector3D &pos, Vector3D &normal,
                                     float *imp, float *pot, float *diff);

};

#endif
