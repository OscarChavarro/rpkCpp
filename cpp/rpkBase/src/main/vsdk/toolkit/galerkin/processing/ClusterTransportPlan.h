#ifndef __CLUSTER_TRANSPORT_PLAN__
#define __CLUSTER_TRANSPORT_PLAN__

#include "vsdk/toolkit/common/color/ColorRgb.h"

/**
What the light transport over one interaction with clusters takes from the
scratch visibility renderer, computed ahead (by a parallel Jacobi worker)
so that the transport itself, which sums into elements shared by other
interactions, is done afterwards in the serial order: the radiance a source
cluster emits towards the receiver and, for a receiver cluster seen from
outside, the area of a sample of its virtual screen and the samples each of
its visible top-level elements covers. Those counts are a range of a sample
pool of the worker (an ArrayList<ElementSamples>), given by its first index
and length, so that planning allocates no memory per interaction. A part that
was not computed ahead is computed by the transport itself.
*/
class ClusterTransportPlan final {
  private:
    bool sourceRadianceKnown;
    ColorRgb sourceRadiance;
    bool receiverSamplesKnown;
    double pixelArea;
    int pool;
    long int firstSample;
    long int numberOfSamples;

  public:
    ClusterTransportPlan():
        sourceRadianceKnown(false), sourceRadiance(0.0, 0.0, 0.0),
        receiverSamplesKnown(false), pixelArea(0.0), pool(-1), firstSample(0), numberOfSamples(0) {}

    bool hasSourceRadiance() const { return sourceRadianceKnown; }
    ColorRgb getSourceRadiance() const { return sourceRadiance; }

    void
    setSourceRadiance(ColorRgb radiance) {
        sourceRadianceKnown = true;
        sourceRadiance = radiance;
    }

    bool hasReceiverSamples() const { return receiverSamplesKnown; }
    double getPixelArea() const { return pixelArea; }
    int getPool() const { return pool; }
    long int getFirstSample() const { return firstSample; }
    long int getNumberOfSamples() const { return numberOfSamples; }

    void
    setReceiverSamples(double inPixelArea, int inPool, long int inFirstSample, long int inNumberOfSamples) {
        receiverSamplesKnown = true;
        pixelArea = inPixelArea;
        pool = inPool;
        firstSample = inFirstSample;
        numberOfSamples = inNumberOfSamples;
    }
};

#endif
