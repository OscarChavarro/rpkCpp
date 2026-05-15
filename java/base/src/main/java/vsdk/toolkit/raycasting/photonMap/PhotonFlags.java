/**
Photon flags used by the photon map
*/

package vsdk.toolkit.raycasting.photonMap;

public final class PhotonFlags {
    public static final short DIRECT_LIGHT_PHOTON = 0x10;
    public static final short CAUSTIC_LIGHT_PHOTON = 0x20; // Lower 4 bits reserved for kd tree
    // This type of photon should not be included in the importance sampling
    public static final short NO_IMPSAMP_PHOTON = (short)(DIRECT_LIGHT_PHOTON | CAUSTIC_LIGHT_PHOTON);

    private PhotonFlags() {
    }
}
