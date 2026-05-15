/**
Samples a random point on the view screen and traces the viewing ray.
*/

import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { BsdfComponent } from "../../material/BsdfComponent";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { Sampler } from "../raytracing/Sampler";

export class ScreenSampler extends Sampler {
  public override sample(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    prevNode: SimpleRaytracingPathNode | null,
    thisNode: SimpleRaytracingPathNode | null,
    newNode: SimpleRaytracingPathNode,
    x1: number,
    x2: number,
    doRR = false,
    flags = Sampler.BSDF_ALL_COMPONENTS
  ): boolean {
    void prevNode;
    void doRR;
    void flags;

    if (thisNode === null) {
      return false;
    }

    const dir = new Vector3D();

    const xSample = camera.pixelWidth * camera.xSize * (-0.5 + x1);
    const ySample = camera.pixelHeight * camera.ySize * (-0.5 + x2);

    dir.combine3(camera.Z, xSample, camera.X, ySample, camera.Y);
    const distScreen2 = dir.norm2();
    const distScreen = globalThis.Math.sqrt(distScreen2);
    dir.inverseScaledCopy(distScreen, dir, Numeric.EPSILON_FLOAT);

    const cosScreen = globalThis.Math.abs(camera.Z.dotProduct(dir));

    const pdfDir =
      (1.0 / (camera.pixelWidth * camera.xSize * camera.pixelHeight * camera.ySize))
      * (distScreen2 / cosScreen);

    thisNode.m_rayType = PathRayType.STARTS;
    newNode.m_inBsdf = thisNode.m_outBsdf;

    if (!this.sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, dir, pdfDir)) {
      thisNode.m_rayType = PathRayType.STOPS;
      return false;
    }

    thisNode.m_bsdfEval.setMonochrome(1.0);
    thisNode.m_bsdfComp.Clear();
    thisNode.m_bsdfComp.Fill(thisNode.m_bsdfEval, BsdfComponent.BRDF_DIFFUSE_COMPONENT);

    thisNode.m_usedComponents = 0;
    newNode.m_accUsedComponents = thisNode.m_accUsedComponents | thisNode.m_usedComponents;
    return true;
  }

  public override evalPDF(
    camera: Camera,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    flags = Sampler.BSDF_ALL_COMPONENTS,
    probabilityDensityFunction: number[] | null = null,
    probabilityDensityFunctionRR: number[] | null = null
  ): number {
    void flags;

    const outDir = new Vector3D();

    outDir.subtraction(newNode.m_hit.getPoint(), thisNode.m_hit.getPoint());
    const dist2 = outDir.norm2();
    const dist = globalThis.Math.sqrt(dist2);
    outDir.inverseScaledCopy(dist, outDir, Numeric.EPSILON_FLOAT);

    const cosA = thisNode.m_normal.dotProduct(outDir);
    let pdf = 1.0 /
      (camera.pixelHeight * camera.ySize * camera.pixelWidth * camera.xSize * cosA * cosA * cosA);

    const cosB = -newNode.m_normal.dotProduct(outDir);
    pdf = pdf * cosB / dist2;

    if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
      probabilityDensityFunction[0] = pdf;
    }
    if (probabilityDensityFunctionRR !== null && probabilityDensityFunctionRR.length > 0) {
      probabilityDensityFunctionRR[0] = 1.0;
    }

    return pdf;
  }
}

