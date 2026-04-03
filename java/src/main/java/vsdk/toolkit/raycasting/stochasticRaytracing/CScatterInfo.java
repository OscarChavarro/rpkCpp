package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;

/**
CScatterinfo includes information about different scattering properties for different bsdf components
This info is used during scattering, but also when weighting or reading storage decisions must be made
*/
public class CScatterInfo {
    // The components under consideration
    public byte flags;
    // Spawning factor if no 'flags' bounce was made before
    public int nrSamplesBefore;
    // Spawning factor after at least one 'flags' bounce
    public int nrSamplesAfter;

    // Some utility functions

    // Were 'flags' last used in the bounce in 'node'
    public boolean
    DoneThisBounce(SimpleRaytracingPathNode node) {
        return node.m_usedComponents == flags;
    }

    // Were 'flags' used at some previous point in the path
    public boolean
    DoneSomePreviousBounce(SimpleRaytracingPathNode node) {
        return ((node.m_accUsedComponents & flags) == flags);
    }
}
