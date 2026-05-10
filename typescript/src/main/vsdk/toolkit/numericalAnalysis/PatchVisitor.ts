import { ColorRgb } from "../common/color/ColorRgb";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { RayHitFlag } from "../skin/RayHitFlag";
import { Patch } from "../skin/Patch";
import { RayHit } from "../skin/RayHit";
import { Niederreiter31 } from "./quasiMonteCarlo/Niederreiter31";

export class PatchVisitor {
  private static getNumberOfSamples(patch: Patch): number {
    let numberOfSamples = 1;
    if (patch.material !== null
      && patch.material.getBsdf() !== null
      && patch.material.getBsdf()!.splitBsdfIsTextured()) {
      const v0 = patch.vertex[0]?.textureCoordinates;
      const v1 = patch.vertex[1]?.textureCoordinates;
      const v2 = patch.vertex[2]?.textureCoordinates;
      const v3 = patch.vertex[3]?.textureCoordinates;
      if (v0 === v1
        && v0 === v2
        && (patch.numberOfVertices === 3 || v0 === v3)
        && v0 !== null) {
        numberOfSamples = 1;
      }
      else {
        numberOfSamples = 100;
      }
    }
    return numberOfSamples;
  }

  public static averageNormalAlbedo(patch: Patch, components: number): ColorRgb {
    const albedo = new ColorRgb();
    const hit = new RayHit();

    hit.init(patch, patch.midPoint, patch.normal, patch.material);

    const numberOfSamples = PatchVisitor.getNumberOfSamples(patch);
    albedo.clear();
    for (let i = 0; i < numberOfSamples; i++) {
      let sample: ColorRgb;
      const xi = Niederreiter31.niederreiter31(i);
      hit.setUv(xi[0] * Niederreiter31.RECIP, xi[1] * Niederreiter31.RECIP);
      const newFlags = hit.getFlags() | RayHitFlag.UV;
      hit.setFlags(newFlags);
      const position: Vector3D = hit.getPoint();
      patch.pointBarycentricMapping(hit.getUv().u, hit.getUv().v, position);
      sample = new ColorRgb();
      sample.clear();
      if (patch.material !== null && patch.material.getBsdf() !== null) {
        sample = patch.material.getBsdf()!.splitBsdfScatteredPower(hit, components);
      }
      albedo.add(albedo, sample);
    }
    albedo.scaleInverse(numberOfSamples, albedo);

    return albedo;
  }

  public static averageEmittance(patch: Patch, components: number): ColorRgb {
    const emittance = new ColorRgb();
    const hit = new RayHit();
    hit.init(patch, patch.midPoint, patch.normal, patch.material);

    const numberOfSamples = PatchVisitor.getNumberOfSamples(patch);
    emittance.clear();
    for (let i = 0; i < numberOfSamples; i++) {
      let sample = new ColorRgb();
      const xi = Niederreiter31.niederreiter31(i);
      hit.setUv(xi[0] * Niederreiter31.RECIP, xi[1] * Niederreiter31.RECIP);
      const newFlags = hit.getFlags() | RayHitFlag.UV;
      hit.setFlags(newFlags);
      const position: Vector3D = hit.getPoint();
      patch.pointBarycentricMapping(hit.getUv().u, hit.getUv().v, position);

      if (patch.material === null || patch.material.getEdf() === null) {
        sample.clear();
      }
      else {
        sample = patch.material.getEdf().phongEmittance(hit, components);
      }
      emittance.add(emittance, sample);
    }
    emittance.scaleInverse(numberOfSamples, emittance);

    return emittance;
  }
}
