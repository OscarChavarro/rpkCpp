/**
Photon map configuration structure, used during construction
*/

import { ScreenBuffer } from "../../render/ScreenBuffer";
import { BiPath } from "../bidirectionalRaytracing/BiPath";
import { LightList } from "../bidirectionalRaytracing/LightList";
import { SamplerConfig } from "../raytracing/SamplerConfig";
import { ImportanceMap } from "./ImportanceMap";
import { PhotonMap } from "./PhotonMap";

export class PhotonMapConfig {
  public lightConfig: SamplerConfig;
  public eyeConfig: SamplerConfig;
  public biPath: BiPath;

  public importanceMap: ImportanceMap | null;
  public importanceCMap: ImportanceMap | null;
  public map: PhotonMap | null;
  public causticMap: PhotonMap | null;

  public currentMap: PhotonMap | null;
  public currentImpMap: ImportanceMap | null;

  public screen: ScreenBuffer | null;
  public lightList: LightList | null;

  public constructor() {
    this.lightConfig = new SamplerConfig();
    this.eyeConfig = new SamplerConfig();
    this.biPath = new BiPath();

    this.importanceMap = null;
    this.importanceCMap = null;
    this.map = null;
    this.causticMap = null;

    this.currentMap = null;
    this.currentImpMap = null;

    this.screen = null;
    this.lightList = null;
  }
}

