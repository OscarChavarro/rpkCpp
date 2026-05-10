import { Logger } from "../common/logging/Logger";
import { RenderOptions } from "../common/RenderOptions";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { Statistics } from "../common/statistics/Statistics";
import { SglContext } from "../render/sgl/SglContext";
import { Scene } from "../scene/Scene";
import { Patch } from "../skin/Patch";
import { Canvas } from "./Canvas";
import { SoftIds } from "./SoftIds";

export class Potential {
  private constructor() {
  }

  public static updateDirectPotential(scene: Scene, renderOptions: RenderOptions): void {
    Canvas.canvasPushMode();

    const x = [0];
    const y = [0];
    const ids = SoftIds.softRenderIds(x, y, scene, renderOptions);

    Canvas.canvasPullMode();

    if (ids === null) {
      return;
    }
    const camera = scene.camera;
    if (camera === null) {
      return;
    }

    let lostPixels = 0;

    const maximumPatchId = Patch.getNextId() - 1;
    const id2patch = new Array<Patch | null>(maximumPatchId + 1).fill(null);
    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      const patch = scene.patchList[i];
      id2patch[patch.id] = patch;
    }

    const newDirectImportance = new Array<number>(maximumPatchId + 1).fill(0.0);

    const h = 2.0 * globalThis.Math.tan(camera.horizontalFov * globalThis.Math.PI / 180.0) / x[0];
    const v = 2.0 * globalThis.Math.tan(camera.verticalFov * globalThis.Math.PI / 180.0) / y[0];
    const pixelArea = h * v;

    for (let j = y[0] - 1, ySample = -v * (y[0] - 1) / 2.0; j >= 0; j--, ySample += v) {
      const rowStart = j * x[0];
      let xSample = -h * (x[0] - 1) / 2.0;
      for (let i = 0; i < x[0]; i++, xSample += globalThis.Math.trunc(h)) {
        const theId = ids[rowStart + i] & 0x00ffffff;

        if (theId > 0 && theId <= maximumPatchId) {
          const pixDir = new Vector3D();
          pixDir.combine3(camera.Z, xSample, camera.X, ySample, camera.Y);

          let deltaImportance = camera.Z.dotProduct(pixDir) / pixDir.dotProduct(pixDir);
          deltaImportance *= deltaImportance * pixelArea;
          newDirectImportance[theId] += deltaImportance;
        }
        else if (theId > maximumPatchId) {
          lostPixels++;
        }
      }
    }

    if (lostPixels > 0) {
      Logger.warning(null, "%d lost pixels", lostPixels);
    }

    const statistics = Statistics.instance();
    statistics.potential.averageDirectPotential = 0.0;
    statistics.potential.totalDirectPotential = 0.0;
    statistics.potential.maxDirectPotential = 0.0;
    statistics.potential.maxDirectImportance = 0.0;

    for (let i = 1; i <= maximumPatchId; i++) {
      const patch = id2patch[i];
      if (patch !== null) {
        patch.directPotential = newDirectImportance[i] / patch.area;

        if (patch.directPotential > statistics.potential.maxDirectPotential) {
          statistics.potential.maxDirectPotential = patch.directPotential;
        }
        statistics.potential.totalDirectPotential += newDirectImportance[i];
        statistics.potential.averageDirectPotential += newDirectImportance[i];

        if (newDirectImportance[i] > statistics.potential.maxDirectImportance) {
          statistics.potential.maxDirectImportance = newDirectImportance[i];
        }
      }
    }
    statistics.potential.averageDirectPotential /= statistics.radiance.totalArea;
  }

  private static softGetPatchPointers(sgl: SglContext, scenePatches: Patch[] | null): void {
    for (let i = 0; scenePatches !== null && i < scenePatches.length; i++) {
      scenePatches[i].setInvisible();
    }

    const pixelCount = sgl.width * sgl.height;
    for (let i = 0; i < pixelCount; i++) {
      const patch = sgl.patchBuffer[i];
      if (patch !== null) {
        patch.setVisible();
      }
    }
  }

  private static softUpdateDirectVisibility(scene: Scene, renderOptions: RenderOptions): void {
    if (scene.camera === null) {
      return;
    }

    const t = process.hrtime.bigint();
    const currentSglContext = SoftIds.setupSoftFrameBuffer(scene.camera);

    SoftIds.softRenderPatches(scene, renderOptions, currentSglContext);
    Potential.softGetPatchPointers(currentSglContext, scene.patchList);

    const elapsedSec = Number(process.hrtime.bigint() - t) / 1_000_000_000.0;
    process.stderr.write(`Determining visible patches in software took ${elapsedSec} sec\n`);
  }

  public static updateDirectVisibility(scene: Scene, renderOptions: RenderOptions): void {
    Canvas.canvasPushMode();
    Potential.softUpdateDirectVisibility(scene, renderOptions);
    Canvas.canvasPullMode();
  }
}
