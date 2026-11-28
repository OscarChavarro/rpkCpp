package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.common.color.ColorRgb;

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
Java adaptation: reset() lets one array of plans be reused from one batch of
interactions to the next, where the C++ port allocates a new array per batch.
*/
public final class ClusterTransportPlan {
    private static final ColorRgb BLACK = new ColorRgb(0.0, 0.0, 0.0);

    private boolean sourceRadianceKnown;
    private ColorRgb sourceRadiance;
    private boolean receiverSamplesKnown;
    private double pixelArea;
    private int pool;
    private int firstSample;
    private int numberOfSamples;

    public ClusterTransportPlan() {
        reset();
    }

    /** Back to the state of a new plan: nothing computed ahead */
    public void reset() {
        sourceRadianceKnown = false;
        sourceRadiance = BLACK;
        receiverSamplesKnown = false;
        pixelArea = 0.0;
        pool = -1;
        firstSample = 0;
        numberOfSamples = 0;
    }

    public boolean hasSourceRadiance() {
        return sourceRadianceKnown;
    }

    public ColorRgb getSourceRadiance() {
        return sourceRadiance;
    }

    public void setSourceRadiance(ColorRgb radiance) {
        sourceRadianceKnown = true;
        sourceRadiance = radiance;
    }

    public boolean hasReceiverSamples() {
        return receiverSamplesKnown;
    }

    public double getPixelArea() {
        return pixelArea;
    }

    public int getPool() {
        return pool;
    }

    public int getFirstSample() {
        return firstSample;
    }

    public int getNumberOfSamples() {
        return numberOfSamples;
    }

    public void setReceiverSamples(double inPixelArea, int inPool, int inFirstSample, int inNumberOfSamples) {
        receiverSamplesKnown = true;
        pixelArea = inPixelArea;
        pool = inPool;
        firstSample = inFirstSample;
        numberOfSamples = inNumberOfSamples;
    }
}
