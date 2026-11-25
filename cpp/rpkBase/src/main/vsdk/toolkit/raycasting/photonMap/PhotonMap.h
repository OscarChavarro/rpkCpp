#ifndef PHOTON_MAP__
#define PHOTON_MAP__

#include "java/lang/Float.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/io/PrintStream.h"
#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonClass.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonKDTree.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapState.h"
#include "vsdk/toolkit/raycasting/photonMap/SampleGrid2D.h"

class PhotonMap {
  protected:
    PhotonMapState &photonMapState;
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

    PhotonKDTree *m_kdtree;

    // A grid is permanently allocated for importance sampling
    SampleGrid2D *m_grid;
    Vector3Df m_sampleLastPos;

    // Space to hold the photons and distances for queries

    int m_nrpFound; // Number of photons in the array
    int m_nrpCosinePos; /* Number of photons in the array that have
			direction * normal > 0, where normal is
			the supplied reconstruction normal. */

    Photon **m_photons;
    float *m_distances;
    float *m_cosines; // photon dir * reconstruction normal
    bool m_cosinesOk; // indicates if cosines are computed

    // nearest photon queries must use these functions!
    int
    doQuery(
        Vector3Df *position,
        int numberOfPhotons,
        float maximumRadius,
        short excludeFlags = 0) {
        m_cosinesOk = false;
        return m_kdtree->query(
            *position,
            numberOfPhotons,
            m_photons,
            m_distances,
            maximumRadius,
            excludeFlags);
    }

    int doQuery(Vector3Df *pos) {
        m_cosinesOk = false;
        return m_kdtree->query(*pos, *m_estimate_nrp /*pmapstate.reconPhotons*/,
                               m_photons, m_distances, static_cast<float>(GetMaxR2()));
    }

    IrrPhoton *
    DoIrradianceQuery(Vector3Df *position, const Vector3Df *normal, float maxR2 = java::Float::MAX_VALUE) {
        return m_kdtree->normalPhotonQuery(position, normal, 0.8F, maxR2);
    }

    // Compute cosines of photons with a supplied normal
    void computeCosines(Vector3Df normal);

    // Add a photon taking possible irrPhoton into account
    void doAddPhoton(Photon &photon, Vector3Df normal, short flags);

  private:
    static float getFalseMonochrome(float val, const PhotonMapState &photonMapState);
    static double computeAcceptProb(
        float currentD,
        float requiredD,
        const PhotonMapState &photonMapState);

  public:
    static bool
    zeroAlbedo(const PhongBidirectionalScatteringDistributionFunction *bsdf, RpkRayHit *hit, char flags);

    // Convert a value val given a maximum into some nice color
    static ColorRgb getFalseColor(float val, const PhotonMapState &photonMapState);

    explicit PhotonMap(PhotonMapState &photonMapState, int *estimate_nrp, bool doPrecomputeIrradiance = false);
    virtual ~PhotonMap();

    void setTotalPaths(long totalPaths) { m_totalPaths = totalPaths; }

    virtual bool addPhoton(Photon &photon, Vector3Df normal, short flags);

    bool DC_AddPhoton(Photon &photon, RpkRayHit &hit,
                      float requiredD, short flags = 0);

    void redistribute(const Photon &photon) const;

    // Get a maximum radius^2 for locating the nearest photons
    virtual double GetMaxR2();

    // Precompute irradiance
    virtual void precomputeIrradiance();

    // For 1 specific photon
    virtual void photonPrecomputeIrradiance(Camera *camera, IrrPhoton *photon);

    // reconstruct
    virtual ColorRgb reconstruct(RpkRayHit *hit, Vector3Df &outDir,
                                 PhongBidirectionalScatteringDistributionFunction *bsdf, PhongBidirectionalScatteringDistributionFunction *inBsdf, PhongBidirectionalScatteringDistributionFunction *outBsdf);

    bool
    irradianceReconstruct(
        RpkRayHit *hit,
        const Vector3Df &outDir,
        const ColorRgb &diffuseAlbedo,
        ColorRgb *result);

    virtual float getCurrentDensity(RpkRayHit &hit, int nrPhotons);

    // Return a color coded density of the photonmap
    virtual ColorRgb getDensityColor(RpkRayHit &hit);

    // Sample values: Random values r,s are transformed into new
    // random values so that importance sampling using the photon
    // map is incorporated

    // IN: r,s in [0,1[
    //     coord: coordinate system that determines angles
    //     flags: component that will be sampled (GR or DR !!)
    //     n: phong exponent for GR

    // OUT: r,s are changed for importance sampling, probabilityDensityFunction is returned
    double
    sample(Vector3Df position, double *r, double *s, const CoordinateSystem *coord, char flag, float n = 1);

    // Utility functions

    void printStats(java::PrintStream *stream) const;

    void getStats(char *p, int n) const;

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
