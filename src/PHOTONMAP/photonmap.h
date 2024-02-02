/* photonmap.H: the real photonmap storage */

#ifndef _PHOTONMAP_H_
#define _PHOTONMAP_H_

#include "common/linealAlgebra/vectorMacros.h"
#include "common/color.h"
#include "PHOTONMAP/photonkdtree.h"
#include "PHOTONMAP/photon.h"
#include "material/bsdf.h"
#include "scene/spherical.h"
#include "shared/samplegrid.h"
#include "PHOTONMAP/pmapoptions.h"


bool ZeroAlbedo(BSDF *bsdf, RayHit *hit, BSDFFLAGS flags);

// Convert a value val given a maximum into some nice color
COLOR GetFalseColor(float val);

COLOR GetFalseColorMonochrome(float val);

bool ZeroAlbedo(BSDF *bsdf, Vector3D *pos, BSDFFLAGS flags);


class CPhotonMap {
protected:
    bool m_balanced;
    bool m_doBalancing;

    bool m_precomputeIrradiance;
    bool m_irradianceComputed;


    // kdtree storage

    int *m_estimate_nrp; // Points to GUI changeable value (comfort)
    int m_sample_nrp;
    int m_nrPhotons;
    int m_totalPhotons;
    long m_totalPaths; // Number of traced paths, not number of photons!!
    // Stored flux value still has to be divided by the total number of paths.

    CPhotonkdtree *m_kdtree;

    // A grid is permanently allocated for importance sampling
    CSampleGrid2D *m_grid;
    Vector3D m_sampleLastPos;

    // Space to hold the photons and distances for queries

    int m_nrpFound; // Number of photons in the array
    int m_nrpCosinePos; /* Number of photons in the array that have
			direction * normal > 0, where normal is
			the a supplied reconstruction normal. */

    CPhoton **m_photons;
    float *m_distances;
    float *m_cosines; // photon dir * reconstruction normal
    bool m_cosinesOk; // indicates if cosines are computed

protected:
    // nearest photon queries must use these fucntions!
    int DoQuery(Vector3D *pos, int nrPhotons, float maxradius,
                short excludeFlags = 0) {
        m_cosinesOk = false;
        return m_kdtree->Query((float *) pos, nrPhotons,
                               m_photons, m_distances, maxradius,
                               excludeFlags);
    }

    int DoQuery(Vector3D *pos) {
        m_cosinesOk = false;
        return m_kdtree->Query((float *) pos, *m_estimate_nrp /*pmapstate.reconPhotons*/,
                               m_photons, m_distances, GetMaxR2());
    }

    CIrrPhoton *DoIrradianceQuery(Vector3D *pos, Vector3D *normal,
                                  float maxR2 = HUGE) {
        return m_kdtree->NormalPhotonQuery(pos, normal, 0.8, maxR2);
    }

    // Compute cosines of photons with a supplied normal
    void ComputeCosines(Vector3D &normal);

    // Add a photon taking possible irrPhoton into account
    void DoAddPhoton(CPhoton &photon, Vector3D &normal, short flags);

public:
    // Constructor

    CPhotonMap(int *estimate_nrp, bool doPrecomputeIrradiance = false);

    // Destructor

    virtual ~CPhotonMap();

    // Initialize

    void Init();

    // Set total paths

    void SetTotalPaths(long totalPaths) { m_totalPaths = totalPaths; }

    void AddPath() { m_totalPaths++; }

    // Adding photons, returns if photon was added

    virtual bool AddPhoton(CPhoton &photon, Vector3D &normal, short flags = 0);

    bool DC_AddPhoton(CPhoton &photon, RayHit &hit,
                      float requiredD, short flags = 0);

    void Redistribute(CPhoton &photon, float acceptProb, short flags = 0);

    // Get a maximum radius^2 for locating nearest photons
    virtual double GetMaxR2();

    // Precompute irradiance
    virtual void PrecomputeIrradiance();

    // For 1 specific photon
    virtual void PhotonPrecomputeIrradiance(CIrrPhoton *photon);

    // Reconstruct
    virtual COLOR Reconstruct(RayHit *hit, Vector3D &outDir,
                              BSDF *bsdf, BSDF *inBsdf, BSDF *outBsdf);

    /*
    bool IrradianceReconstruct(RayHit *hit, Vector3D &outDir,
                       BSDF *bsdf, BSDF *inBsdf,
                       BSDF *outBsdf,
                       COLOR *result);
    */

    bool IrradianceReconstruct(RayHit *hit, Vector3D &outDir,
                               COLOR &diffuseAlbedo,
                               COLOR *result);

    virtual float GetCurrentDensity(RayHit &hit, int nrPhotons = 0);

    // Return a color coded density of the photonmap
    virtual COLOR GetDensityColor(RayHit &hit);


    // Sample values: Random values r,s are transformed into new
    // random values so that importance sampling using the photon
    // map is incorportated.

    // IN: r,s in [0,1[
    //     coord: coordinate system that determines angles
    //     flags: component that will be sampled (GR or DR !!)
    //     n: phong exponent for GR

    // OUT: r,s are changed for importance sampling, pdf is returned

    double Sample(Vector3D &pos, double *r, double *s, COORDSYS *coord,
                  BSDFFLAGS flag, float n = 1);

    float GetGridValue(double phi, double theta);

    // Utility functions

    void PrintStats(FILE *fp) {
        fprintf(fp, "%i stored photons\n", m_nrPhotons);
    }

    void getStats(char *p, int n) {
        snprintf(p, n,
                "%i stored photons, %i total, %li paths\n",
                m_nrPhotons, m_totalPhotons, m_totalPaths);
    }

    void BalanceAnalysis() {
        m_kdtree->BalanceAnalysis();
    }

    void Balance() {
        m_kdtree->Balance();
    }

    void CheckNBalance() {
        if ((!m_balanced) && (m_doBalancing || m_precomputeIrradiance)) {
            Balance();
        }
    }

    void DoBalancing(bool state) {
        m_doBalancing = state;
    }
};

#endif
