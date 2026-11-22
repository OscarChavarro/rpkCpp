import { SamplerConfig } from "../raytracing/SamplerConfig";
import { ScreenBuffer } from "../../render/ScreenBuffer";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { BidirectionalPathRaytracerConfig } from "./BidirectionalPathRaytracerConfig";
import { DensityBuffer } from "./DensityBuffer";
import { Kernel2D } from "./Kernel2D";
import { SparConfig } from "./SparConfig";
import { SparList } from "./SparList";

export class BidirectionalPathTracingConfiguration {
  public baseConfig: BidirectionalPathRaytracerConfig | null;

  public eyeConfig: SamplerConfig;
  public lightConfig: SamplerConfig;

  public screen: ScreenBuffer | null;
  public toneMapOptions: ToneMappingContext | null;
  public fluxToRadFactor: number;
  public nx: number;
  public ny: number;
  public pdfLNE: number;

  public dBuffer: DensityBuffer | null;
  public dBuffer2: DensityBuffer | null;
  public xSample: number;
  public ySample: number;
  public eyePath: SimpleRaytracingPathNode | null;
  public lightPath: SimpleRaytracingPathNode | null;

  public sparConfig: SparConfig;
  public sparList: SparList | null;
  public deStoreHits: boolean;
  public ref: ScreenBuffer | null;
  public dest: ScreenBuffer | null;
  public ref2: ScreenBuffer | null;
  public dest2: ScreenBuffer | null;
  public kernel: Kernel2D;
  public scaleSamples: number;

  public constructor() {
    this.baseConfig = null;
    this.eyeConfig = new SamplerConfig();
    this.lightConfig = new SamplerConfig();
    this.screen = null;
    this.toneMapOptions = null;
    this.fluxToRadFactor = 0.0;
    this.nx = 0;
    this.ny = 0;
    this.pdfLNE = 0.0;
    this.dBuffer = null;
    this.dBuffer2 = null;
    this.xSample = 0.0;
    this.ySample = 0.0;
    this.eyePath = null;
    this.lightPath = null;
    this.sparConfig = new SparConfig();
    this.sparList = null;
    this.deStoreHits = false;
    this.ref = null;
    this.dest = null;
    this.ref2 = null;
    this.dest2 = null;
    this.kernel = new Kernel2D();
    this.scaleSamples = 0;
  }
}
