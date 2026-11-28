import { String as JavaString } from "vitral/dist/java/lang/String";
import { Logger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { RendererConfiguration } from "../material/RendererConfiguration";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Statistics } from "../common/statistics/Statistics";
import { PolygonVisibilityBuffer } from "vitral/dist/vsdk/toolkit/render/raster/polygonScanConverter/PolygonVisibilityBuffer";
import { Scene } from "../scene/Scene";
import { Patch } from "../environment/geometry/elements/Patch";
import { Canvas } from "./Canvas";
import { FaceIndexSample, type FaceVisibilityBuffer } from "./FaceVisibilityBuffer";
import { SoftIds } from "./SoftIds";

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

    const camera = scene.camera!;
    const target: FaceVisibilityBuffer = new PolygonVisibilityBuffer<FaceIndexSample>(
      camera.xSize, camera.ySize, new FaceIndexSample());
    SoftIds.renderVisibleFaces(scene, renderOptions, target);
    const x = [target.getWidth()];
    const y = [target.getHeight()];

    Canvas.canvasPullMode();

    let lostPixels = 0;

    const maximumPatchId = Patch.getNextId() - 1;
    const id2patch = new Array<Patch | null>(maximumPatchId + 1).fill(null);
    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      const patch = scene.patchList[i]!;
      id2patch[patch.id] = patch;
    }

    // Allocate space for an array to hold the new direct potential of the patches
    const newDirectImportance = new Float64Array(maximumPatchId + 1);

    // h and v are the horizontal resp. vertical distance between two
    // neighboring pixels on the screen
    const PI = globalThis.Math.PI;
    const h = 2.0 * globalThis.Math.tan((camera.horizontalFov * PI) / 180.0) / x[0]!;
    const v = 2.0 * globalThis.Math.tan((camera.verticalFov * PI) / 180.0) / y[0]!;
    const pixelArea = h * v;

    for (let j = y[0]! - 1, ySample = (-v * (y[0]! - 1)) / 2.0; j >= 0; j--, ySample = ySample + v) {
      let xSample = (-h * (x[0]! - 1)) / 2.0;
      for (let i = 0; i < x[0]!; i++, xSample = xSample + h) {
        const face = target.getSample(i, j);
        const theId = face.isBackground() ? 0 : scene.getFace(face.getIndex()).id & 0x00ffffff;

        if (theId > 0 && theId <= maximumPatchId) {
          let pixDir = new Vector3Dd();
          pixDir = camera.Z.add(camera.X.multiply(xSample)).add(camera.Y.multiply(ySample));

          // Delta_importance = (cosine of the angle between the direction to
          // the pixel and the viewing direction, over the distance from the
          // eye point to the pixel) squared, times area of the pixel
          let deltaImportance = camera.Z.dotProduct(pixDir) / pixDir.dotProduct(pixDir);
          deltaImportance = deltaImportance * (deltaImportance * pixelArea);
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
        directPotentials.set(faceIndex, newDirectImportance[i]! / patch.getArea());

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

  private static softUpdateDirectVisibility(scene: Scene, renderOptions: RendererConfiguration): void {
    const t = process.hrtime.bigint();

    const target: FaceVisibilityBuffer = new PolygonVisibilityBuffer<FaceIndexSample>(
      scene.camera!.xSize, scene.camera!.ySize, new FaceIndexSample());
    SoftIds.renderVisibleFaces(scene, renderOptions, target);

    const faceVisibility = scene.getFaceVisibility()!;
    for (let i = 0; i < faceVisibility.getNumberOfFaces(); i++) {
      faceVisibility.setVisible(i, false);
    }
    for (let y = 0; y < target.getHeight(); y++) {
      for (let x = 0; x < target.getWidth(); x++) {
        const face = target.getSample(x, y);
        if (!face.isBackground()) {
          faceVisibility.setVisible(face.getIndex(), true);
        }
      }
    }

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
