package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.Error;
import vsdk.toolkit.scene.RadianceMethod;

/**
LD Spar : Uses direct diffuse as stored radiance. Allows sampling of
of eye paths. GetDirectRadiance is used as a readout function
*/
public final class LDSpar extends Spar {
    @Override
    public void init(SparConfig sparConfig, RadianceMethod radianceMethod) {
        super.init(sparConfig, radianceMethod);

        if ( !(sparConfig.baseConfig.doLD != 0 || sparConfig.baseConfig.doWeighted != 0) ) {
            return;
        }

        if ( radianceMethod == null ) {
            Error.error("CLDSpar::mainInitApplication", "Galerkin Radiance method not active !");
        }

        // Overlap group
        if ( sparConfig.baseConfig.doLD != 0 ) {
            parseAndInit(SparPathGroup.DISJOINT_GROUP, sparConfig.baseConfig.ldRegExp);
        }

        if ( sparConfig.baseConfig.doWeighted != 0 ) {
            parseAndInit(SparPathGroup.LD_GROUP, sparConfig.baseConfig.wldRegExp);
            m_sparList[SparPathGroup.LD_GROUP].add(sparConfig.leSpar);
        }
    }
}
