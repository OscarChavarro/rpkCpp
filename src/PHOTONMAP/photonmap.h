#ifndef __PHOTON_MAP__
#define __PHOTON_MAP__

#include "common/ColorRgb.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "common/linealAlgebra/CoordinateSystem.h"
#include "PHOTONMAP/photonkdtree.h"
#include "PHOTONMAP/photon.h"
#include "PHOTONMAP/samplegrid.h"
#include "PHOTONMAP/pmapoptions.h"

bool zeroAlbedo(const PhongBidirectionalScatteringDistributionFunction *bsdf, RayHit *hit, char flags);

// Convert a value val given a maximum into some nice color
ColorRgb getFalseColor(float val);

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
			the supplied reconstruction normal. */

    CPhoton **m_photons;
    float *m_distances;
    float *m_cosines; // photon dir * reconstruction normal
    bool m_cosinesOk; // indicates if cosines are computed

    // nearest photon queries must use these functions!
    int
    doQuery(
        Vector3D *position,
        int numberOfPhotons,
        float maximumRadius,
        short excludeFlags = 0) {
        m_cosinesOk = false;
        return m_kdtree->query(
            (float *)position,
            numberOfPhotons,
            m_photons,
            m_distances,
            maximumRadius,
            excludeFlags);
    }

    int doQuery(Vector3D *pos) {
        m_cosinesOk = false;
        return m_kdtree->query((float *) pos, *m_estimate_nrp /*pmapstate.reconPhotons*/,
                               m_photons, m_distances, (float) GetMaxR2());
    }

    CIrrPhoton *
    DoIrradianceQuery(Vector3D *position, const Vector3D *normal, float maxR2 = HUGE_FLOAT_VALUE) {
        return m_kdtree->normalPhotonQuery(position, normal, 0.8f, maxR2);
    }

    // Compute cosines of photons with a supplied normal
    void computeCosines(Vector3D normal);

    // Add a photon taking possible irrPhoton into account
    void doAddPhoton(CPhoton &photon, Vector3D normal, short flags);

  public:
    explicit CPhotonMap(int *estimate_nrp, bool doPrecomputeIrradiance = false);
    virtual ~CPhotonMap();

    void setTotalPaths(long totalPaths) { m_totalPaths = totalPaths; }

    virtual bool addPhoton(CPhoton &photon, Vector3D normal, short flags);

    bool DC_AddPhoton(CPhoton &photon, RayHit &hit,
                      float requiredD, short flags = 0);

    void redistribute(const CPhoton &photon) const;

    // Get a maximum radius^2 for locating the nearest photons
    virtual double GetMaxR2();

    // Precompute irradiance
    virtual void precomputeIrradiance();

    // For 1 specific photon
    virtual void photonPrecomputeIrradiance(Camera *camera, CIrrPhoton *photon);

    // reconstruct
    virtual ColorRgb reconstruct(RayHit *hit, Vector3D &outDir,
                                 PhongBidirectionalScatteringDistributionFunction *bsdf, PhongBidirectionalScatteringDistributionFunction *inBsdf, PhongBidirectionalScatteringDistributionFunction *outBsdf);

    bool
    irradianceReconstruct(
        RayHit *hit,
        const Vector3D &outDir,
        const ColorRgb &diffuseAlbedo,
        ColorRgb *result);

    virtual float getCurrentDensity(RayHit &hit, int nrPhotons);

    // Return a color coded density of the photonmap
    virtual ColorRgb getDensityColor(RayHit &hit);

    // Sample values: Random values r,s are transformed into new
    // random values so that importance sampling using the photon
    // map is incorporated

    // IN: r,s in [0,1[
    //     coord: coordinate system that determines angles
    //     flags: component that will be sampled (GR or DR !!)
    //     n: phong exponent for GR

    // OUT: r,s are changed for importance sampling, probabilityDensityFunction is returned
    double
    sample(Vector3D position, double *r, double *s, const CoordinateSystem *coord, char flag, float n = 1);

    // Utility functions

    void printStats(FILE *fp) const {
        fprintf(fp, "%i stored photons\n", m_nrPhotons);
    }

    void getStats(char *p, int n) const {
        snprintf(p, n,
                "%i stored photons, %i total, %li paths\n",
                m_nrPhotons, m_totalPhotons, m_totalPaths);
    }

    void Balance() {
        m_kdtree->balance();
    }

    void checkNBalance() {
        if ((!m_balanced) && (m_doBalancing || m_precomputeIrradiance) ) {
            Balance();
        }
    }

    void doBalancing(bool state) {
        m_doBalancing = state;
    }
};

#endif
