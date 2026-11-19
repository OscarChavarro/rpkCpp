package vsdk.toolkit.material;

/**
Selects sampling mode according to given probabilities and random number x1.
If the selected mode is not absorption, x1 is rescaled to the interval [0, 1)
again
*/

public enum SplitBSDFSamplingMode {
    SAMPLE_TEXTURE,
    SAMPLE_REFLECTION,
    SAMPLE_TRANSMISSION,
    SAMPLE_ABSORPTION
}
