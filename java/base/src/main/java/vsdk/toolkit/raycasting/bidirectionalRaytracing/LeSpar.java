package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.scene.RadianceMethod;

/**
Le Spar : Uses emission ase stored radiance. Allows sampling of
all bidirectional paths
*/
public final class LeSpar extends Spar {
    @Override
    public void init(SparConfig sparConfig, RadianceMethod radianceMethod) {
        super.init(sparConfig, radianceMethod);

        // Disjoint path group for BPT
        if ( sparConfig.baseConfig.doLe != 0 ) {
            parseAndInit(SparPathGroup.DISJOINT_GROUP, sparConfig.baseConfig.leRegExp);
        }

        if ( sparConfig.baseConfig.doWeighted != 0 ) {
            parseAndInit(SparPathGroup.LD_GROUP, sparConfig.baseConfig.wleRegExp);
            m_sparList[SparPathGroup.LD_GROUP].add(sparConfig.ldSpar);
        }
    }
}
