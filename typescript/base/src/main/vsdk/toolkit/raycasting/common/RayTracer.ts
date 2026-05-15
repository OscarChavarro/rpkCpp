import { OutputStream } from "../../../../java/io/OutputStream";
import { ArrayList } from "../../../../java/util/ArrayList";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { ImageOutputHandle } from "../../io/image/ImageOutputHandle";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { Scene } from "../../scene/Scene";
import { Patch } from "../../environment/geometry/elements/Patch";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";

export abstract class RayTracer {
  public abstract defaults(): void;

  public abstract getName(): string;

  public abstract initialize(lightPatches: ArrayList<Patch>): void;

  public abstract execute(
    ip: ImageOutputHandle | null,
    scene: Scene,
    radianceMethod: RadianceMethod,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void;

  public abstract saveImage(imageOutputHandle: ImageOutputHandle): boolean;

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
    let img: ImageOutputHandle | null = null;

    if (stream !== null) {
      if (scene.camera === null) {
        return;
      }
      img = ImageOutputHandle.createRadianceImageOutputHandle(
        fileName,
        stream,
        isPipe,
        scene.camera.xSize,
        scene.camera.ySize
      );
      if (img === null) {
        return;
      }
    }

    if (rayTracer !== null) {
      rayTracer.execute(img, scene, radianceMethod, toneMapOptions, renderOptions);
    }

    if (img !== null) {
      ImageOutputHandle.deleteImageOutputHandle(img);
    }
  }
}
