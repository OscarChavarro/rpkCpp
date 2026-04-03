package vsdk.toolkit.raycasting.raytracing;

/**
Next event samplers provide a few functions to
enumerate different 'next event units' (e.g. light sources
or cameras). This allows to sample all units separately,
f.i. if you want to sample all light sources.

The interface is very simple. I just wanted to be able
to sample all light sources.
*/
public abstract class CNextEventSampler extends Sampler {
    // Setting units causes sampling of the activated unit
    // instead of over all units.

    public boolean ActivateFirstUnit() {
        return false;
    }

    // Activate next unit.
    // If no next unit is available:
    //   Returns false and unsets units
    public boolean ActivateNextUnit() {
        return false;
    }
}
