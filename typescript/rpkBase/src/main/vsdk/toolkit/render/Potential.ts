import { String as JavaString } from "../../../java/lang/String";
import { Logger } from "../common/logging/Logger";
import { RendererConfiguration } from "../material/RendererConfiguration";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { Statistics } from "../common/statistics/Statistics";
import { SglContext } from "../render/sgl/SglContext";
import type { FaceVisibilityTable } from "../scene/FaceVisibilityTable";
import { Scene } from "../scene/Scene";
import { Patch } from "../environment/geometry/elements/Patch";
import { Canvas } from "./Canvas";
import { SoftIds } from "./SoftIds";

// Single precision rounding of the source's float stores and operations
const f = Math.fround;

/**
Routines dealing with view potential. In analogy with [SMIT1992] Smits,
"Importance-driven Radiosity", SIGGRAPH '92, we call the integral of
potential over surface area "importance"
*/
export class Potential {
  private constructor() {
  }

  /**
  Updates directly received potential for all patches
  */
  public static updateDirectPotential(scene: Scene, renderOptions: RendererConfiguration): void {
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
      const patch = scene.patchList[i]!;
      id2patch[patch.id] = patch;
    }

    // Allocate space for an array to hold the new direct potential of the patches
    const newDirectImportance = new Float32Array(maximumPatchId + 1);

    // h and v are the horizontal resp. vertical distance between two
    // neighboring pixels on the screen
    const PI = f(globalThis.Math.PI);
    const h = f(2.0 * globalThis.Math.tan(f(f(camera.horizontalFov * PI) / 180.0)) / f(x[0]!));
    const v = f(2.0 * globalThis.Math.tan(f(f(camera.verticalFov * PI) / 180.0)) / f(y[0]!));
    const pixelArea = f(h * v);

    for (let j = y[0]! - 1, ySample = f(f(-v * f(y[0]! - 1)) / 2.0); j >= 0; j--, ySample = f(ySample + v)) {
      const rowStart = j * x[0]!;
      let xSample = f(f(-h * f(x[0]! - 1)) / 2.0);
      for (let i = 0; i < x[0]!; i++, xSample = f(xSample + h)) {
        const theId = ids[rowStart + i]! & 0x00ffffff;

        if (theId > 0 && theId <= maximumPatchId) {
          const pixDir = new Vector3D();
          pixDir.combine3(camera.Z, xSample, camera.X, ySample, camera.Y);

          // Delta_importance = (cosine of the angle between the direction to
          // the pixel and the viewing direction, over the distance from the
          // eye point to the pixel) squared, times area of the pixel
          let deltaImportance = f(camera.Z.dotProduct(pixDir) / pixDir.dotProduct(pixDir));
          deltaImportance = f(deltaImportance * f(deltaImportance * pixelArea));
          newDirectImportance[theId] = newDirectImportance[theId]! + deltaImportance;
        }
        else if (theId > maximumPatchId) {
          lostPixels++;
        }
      }
    }

    if (lostPixels > 0) {
      Logger.reportMessage(null, Logger.WARNING, "", JavaString.vformat("%d lost pixels", [lostPixels]));
    }

    const statistics = Statistics.instance();
    statistics.potential.averageDirectPotential = 0.0;
    statistics.potential.totalDirectPotential = 0.0;
    statistics.potential.maxDirectPotential = 0.0;
    statistics.potential.maxDirectImportance = 0.0;

    for (let i = 1; i <= maximumPatchId; i++) {
      const patch = id2patch[i]!;
      if (patch !== null) {
        const directPotentials = scene.getDirectPotentials()!;
        const faceIndex = patch.getFaceIndex();
        directPotentials.set(faceIndex, f(newDirectImportance[i]! / patch.getArea()));

        if (directPotentials.get(faceIndex) > statistics.potential.maxDirectPotential) {
          statistics.potential.maxDirectPotential = directPotentials.get(faceIndex);
        }
        statistics.potential.totalDirectPotential += newDirectImportance[i]!;
        statistics.potential.averageDirectPotential += newDirectImportance[i]!;

        if (newDirectImportance[i]! > statistics.potential.maxDirectImportance) {
          statistics.potential.maxDirectImportance = newDirectImportance[i]!;
        }
      }
    }
    statistics.potential.averageDirectPotential /= statistics.radiance.totalArea;
  }

  private static softGetPatchPointers(sgl: SglContext, scenePatches: Patch[] | null, faceVisibility: FaceVisibilityTable): void {
    for (let i = 0; scenePatches !== null && i < scenePatches.length; i++) {
      faceVisibility.setVisible(scenePatches[i]!.getFaceIndex(), false);
    }

    const pixelCount = sgl.width * sgl.height;
    for (let i = 0; i < pixelCount; i++) {
      const patch = sgl.patchBuffer[i]!; // softRenderPatches stores patch pointers there
      if (patch !== null) {
        faceVisibility.setVisible(patch.getFaceIndex(), true);
      }
    }
  }

  private static softUpdateDirectVisibility(scene: Scene, renderOptions: RendererConfiguration): void {
    if (scene.camera === null) {
      return;
    }

    const t = process.hrtime.bigint();
    const currentSglContext = SoftIds.setupSoftFrameBuffer(scene.camera);

    SoftIds.softRenderPatches(scene, renderOptions, currentSglContext);
    Potential.softGetPatchPointers(currentSglContext, scene.patchList, scene.getFaceVisibility()!);

    const elapsedSec = Number(process.hrtime.bigint() - t) / 1_000_000_000.0;
    process.stderr.write(`Determining visible patches in software took ${elapsedSec} sec\n`);
  }

  /**
  Updates view visibility status of all patches
  */
  public static updateDirectVisibility(scene: Scene, renderOptions: RendererConfiguration): void {
    Canvas.canvasPushMode();
    Potential.softUpdateDirectVisibility(scene, renderOptions);
    Canvas.canvasPullMode();
  }
}
