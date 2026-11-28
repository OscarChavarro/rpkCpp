package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.galerkin.GalerkinElement;

/**
Count of the scratch buffer samples a top-level element covers.
Java adaptation: the C++ port's ElementSamples is an immutable value, replaced
in its list for every run of samples added to it. Here the counting that
creates an instance (ScratchVisibilityWorkspace.addSamples) adds to it in
place, so that counting allocates one object per visible element and not one
per run; once that counting ends (the next one starts with a new list of new
instances) it does not change any more, and it can be shared, as the sample
pools of the parallel Jacobi workers do.
*/
public final class ElementSamples {
    private final GalerkinElement element;
    private int samples;

    public ElementSamples() {
        this(null, 0);
    }

    public ElementSamples(GalerkinElement inElement, int inSamples) {
        element = inElement;
        samples = inSamples;
    }

    public GalerkinElement getElement() {
        return element;
    }

    public int getSamples() {
        return samples;
    }

    /** For ScratchVisibilityWorkspace only, during the counting that created this instance */
    void addSamples(int inSamples) {
        samples += inSamples;
    }
}
