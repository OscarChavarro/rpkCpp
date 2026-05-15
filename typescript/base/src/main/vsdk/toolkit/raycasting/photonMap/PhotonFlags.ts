/**
Photon flags used by the photon map
*/
export class PhotonFlags {
  public static readonly DIRECT_LIGHT_PHOTON = 0x10;
  public static readonly CAUSTIC_LIGHT_PHOTON = 0x20;
  public static readonly NO_IMPSAMP_PHOTON =
    PhotonFlags.DIRECT_LIGHT_PHOTON | PhotonFlags.CAUSTIC_LIGHT_PHOTON;

  private constructor() {
  }
}

