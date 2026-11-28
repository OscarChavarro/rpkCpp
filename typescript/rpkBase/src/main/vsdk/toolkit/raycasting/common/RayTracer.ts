import { OutputStream } from "vitral/dist/java/io/OutputStream";
import { ArrayList } from "vitral/dist/java/util/ArrayList";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { ImageOutputTarget } from "../../render/ImageOutputTarget";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { Scene } from "../../scene/Scene";
import { Patch } from "../../environment/geometry/elements/Patch";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";

export abstract class RayTracer {
  public abstract defaults(): void;

  public abstract getName(): string;

  public abstract initialize(lightPatches: ArrayList<Patch>): void;

  public abstract execute(
    ip: ImageOutputTarget | null,
    scene: Scene,
    radianceMethod: RadianceMethod,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void;

  public abstract saveImage(imageOutputTarget: ImageOutputTarget): boolean;

  public abstract terminate(): void;

  public static rayTrace(
    fileName: string,
    stream: OutputStream | null,
    isPipe: number,
    rayTracer: RayTracer | null,
    scene: Scene,
    radianceMethod: RadianceMethod,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void {
    let img: ImageOutputTarget | null = null;

    if (stream !== null) {
      if (scene.camera === null) {
        return;
      }
      img = ImageOutputTarget.create(fileName, stream, isPipe);
      if (img === null) {
        return;
      }
    }

    if (rayTracer !== null) {
      rayTracer.execute(img, scene, radianceMethod, toneMapOptions, renderOptions);
    }
  }
}
