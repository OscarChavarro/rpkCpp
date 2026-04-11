import { OutputStream } from "../../../java/io/OutputStream";
import { ColorRgb } from "../common/ColorRgb";
import { RenderOptions } from "../common/RenderOptions";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { Element } from "../skin/Element";
import { Patch } from "../skin/Patch";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";
import { Camera } from "./Camera";
import { RadianceMethodAlgorithm } from "./RadianceMethodAlgorithm";
import { Scene } from "./Scene";

export abstract class RadianceMethod {
  public className!: RadianceMethodAlgorithm;

  public constructor() {
  }

  public abstract getRadianceMethodName(): string;

  public abstract parseOptions(argc: number[], argv: string[]): void;

  public abstract initialize(scene: Scene, toneMapOptions: ToneMappingContext): void;

  public abstract doStep(scene: Scene, renderOptions: RenderOptions): boolean;

  public abstract terminate(scenePatches: Patch[]): void;

  public abstract getRadiance(
    camera: Camera,
    patch: Patch,
    u: number,
    v: number,
    dir: Vector3D,
    renderOptions: RenderOptions
  ): ColorRgb;

  public abstract createPatchData(patch: Patch): Element | null;

  public abstract destroyPatchData(patch: Patch): void;

  public abstract getStats(): string;

  public abstract writeVRML(
    camera: Camera,
    outputStream: OutputStream,
    renderOptions: RenderOptions
  ): void;
}
