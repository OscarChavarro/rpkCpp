/**
Specification of the Stored Partial Radiance class
*/

package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.scene.RadianceMethod;

public class Spar {
    public ContribHandler[] m_contrib;
    public SparList[] m_sparList;

    public Spar() {
        m_contrib = new ContribHandler[SparPathGroupInfo.MAX_PATH_GROUPS];
        m_sparList = new SparList[SparPathGroupInfo.MAX_PATH_GROUPS];

        for ( int i = 0; i < SparPathGroupInfo.MAX_PATH_GROUPS; i++ ) {
            m_contrib[i] = new ContribHandler();
            m_sparList[i] = new SparList();
        }
    }

    public void init(SparConfig config, RadianceMethod radianceMethod) {
        for ( int i = 0; i < SparPathGroupInfo.MAX_PATH_GROUPS; i++ ) {
            m_contrib[i].init(config.baseConfig.maximumPathDepth);
            m_sparList[i].removeAll();
        }
    }

    /**
    MainInit spar with a comma separated list of regular expressions
    */
    public void parseAndInit(int group, String regExp) {
        if ( regExp == null ) {
            return;
        }

        int beginPos = 0;
        int endPos = 0;

        while ( endPos < regExp.length() ) {
            if ( regExp.charAt(endPos) == ',' ) {
                // Next RegExp
                m_contrib[group].addRegExp(regExp.substring(beginPos, endPos));
                beginPos = endPos + 1; // Begin next regexp
            }

            endPos++;
        }

        // Still parse last regexp in list
        if ( beginPos != endPos ) {
            m_contrib[group].addRegExp(regExp.substring(beginPos, endPos));
        }
    }

    /**
    Handles a bidirectional path. Image contribution
    is returned. Normally this is a contribution for the pixel
    affected by the path
    */
    public ColorRgb handlePath(SparConfig config, BiPath path) {
        ColorRgbMutable result = new ColorRgbMutable();
        result.clear();
        return result.toImmutable();
    }
}
