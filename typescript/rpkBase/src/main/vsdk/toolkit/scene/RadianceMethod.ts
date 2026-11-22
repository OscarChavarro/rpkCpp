import { OutputStream } from "vitral/dist/java/io/OutputStream";
import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { RendererConfiguration } from "../material/RendererConfiguration";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { Element } from "../environment/geometry/elements/Element";
import { Patch } from "../environment/geometry/elements/Patch";
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

  public abstract doStep(scene: Scene, renderOptions: RendererConfiguration): boolean;

  public abstract terminate(scenePatches: Patch[]): void;

  public abstract getRadiance(
    camera: Camera,
    patch: Patch,
    u: number,
    v: number,
    dir: Vector3Df,
    renderOptions: RendererConfiguration
  ): ColorRgbMutable;

  public abstract createPatchData(patch: Patch): Element | null;

  public abstract destroyPatchData(patch: Patch): void;

  public abstract getStats(): string;

  public abstract writeVRML(
    camera: Camera,
    outputStream: OutputStream,
    renderOptions: RendererConfiguration
  ): void;
}
