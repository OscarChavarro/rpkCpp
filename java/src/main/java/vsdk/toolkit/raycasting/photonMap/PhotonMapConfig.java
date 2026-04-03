/**
Photon map configuration structure, used during construction
*/

package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.raycasting.bidirectionalRaytracing.BiPath;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.LightList;
import vsdk.toolkit.raycasting.raytracing.SamplerConfig;
import vsdk.toolkit.render.ScreenBuffer;

public class PhotonMapConfig {
    public SamplerConfig lightConfig;
    public SamplerConfig eyeConfig;
    public BiPath biPath;

    public ImportanceMap importanceMap;
    public ImportanceMap importanceCMap;
    public PhotonMap map;
    public PhotonMap causticMap;

    public PhotonMap currentMap; // Map in current use: global or caustic
    public ImportanceMap currentImpMap; // Importance Map in current use: global or caustic

    public ScreenBuffer screen;
    public LightList lightList;

    public PhotonMapConfig() {
        lightConfig = new SamplerConfig();
        eyeConfig = new SamplerConfig();
        biPath = new BiPath();

        importanceMap = null;
        importanceCMap = null;
        map = null;
        causticMap = null;

        currentMap = null;
        currentImpMap = null;

        screen = null;
        lightList = null;
    }
}
