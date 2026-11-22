import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { RayHitFlag } from "../environment/geometry/elements/RayHitFlag";
import { Patch } from "../environment/geometry/elements/Patch";
import { RayHit } from "../environment/geometry/elements/RayHit";
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

  public static averageNormalAlbedo(patch: Patch, components: number): ColorRgbMutable {
    const albedo = new ColorRgbMutable();
    const hit = new RayHit();

    hit.init(patch, patch.midPoint(), patch.getNormal(), patch.material);

    const numberOfSamples = PatchVisitor.getNumberOfSamples(patch);
    albedo.clear();
    for (let i = 0; i < numberOfSamples; i++) {
      let sample: ColorRgbMutable;
      const xi = Niederreiter31.niederreiter31(i);
      hit.setUv(xi[0]! * Niederreiter31.RECIP, xi[1]! * Niederreiter31.RECIP);
      hit.setPoint(patch.pointBarycentricMapping(hit.getUv().x, hit.getUv().y) ?? hit.getPoint());
      sample = new ColorRgbMutable();
      sample.clear();
      if (patch.material !== null && patch.material.getBsdf() !== null) {
        sample = patch.material.getBsdf()!.splitBsdfScatteredPower(hit.shadingContext(), components);
      }
      albedo.add(albedo, sample);
    }
    albedo.scaleInverse(numberOfSamples, albedo);

    return albedo;
  }

  public static averageEmittance(patch: Patch, components: number): ColorRgbMutable {
    const emittance = new ColorRgbMutable();
    const hit = new RayHit();
    hit.init(patch, patch.midPoint(), patch.getNormal(), patch.material);

    const numberOfSamples = PatchVisitor.getNumberOfSamples(patch);
    emittance.clear();
    for (let i = 0; i < numberOfSamples; i++) {
      let sample = new ColorRgbMutable();
      const xi = Niederreiter31.niederreiter31(i);
      hit.setUv(xi[0]! * Niederreiter31.RECIP, xi[1]! * Niederreiter31.RECIP);
      hit.setPoint(patch.pointBarycentricMapping(hit.getUv().x, hit.getUv().y) ?? hit.getPoint());

      if (patch.material === null || patch.material.getEdf() === null) {
        sample.clear();
      }
      else {
        sample = patch.material.getEdf().phongEmittance(hit.shadingContext(), components);
      }
      emittance.add(emittance, sample);
    }
    emittance.scaleInverse(numberOfSamples, emittance);

    return emittance;
  }
}
