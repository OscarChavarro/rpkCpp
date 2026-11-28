import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { Numeric } from "../../skin/Numeric";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { BsdfComponent } from "../../material/BsdfComponent";
import { XxdfComponentFlag } from "../../material/XxdfComponentFlag";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { SceneQuery } from "../../scene/SceneQuery";
import type { SceneQueryWorkspace } from "../../scene/SceneQuery";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { Sampler } from "../raytracing/Sampler";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class LightDirSampler extends Sampler {
  public override sample(
    camera: Camera,
    sceneQuery: SceneQuery,
    sceneSceneQueryWorkspace: SceneQueryWorkspace,
    sceneBackground: Background | null,
    prevNode: SimpleRaytracingPathNode | null,
    thisNode: SimpleRaytracingPathNode | null,
    newNode: SimpleRaytracingPathNode,
    x1: number,
    x2: number,
    doRR = false,
    flags = Sampler.BSDF_ALL_COMPONENTS
  ): boolean {
    void camera;
    void prevNode;
    void doRR;
    const pdfDir = [0.0];

    if (thisNode === null || thisNode.m_hit.getMaterial() === null || thisNode.m_hit.getMaterial()!.getEdf() === null) {
      VsdkLogger.reportMessage(this, VsdkLogger.ERROR, "CLightDirSampler::sample", "No EDF");
      return false;
    }

    let dir = new Vector3Dd(0.0, 0.0, 0.0);
    const lightContextOk = [false];
    const lightContext = thisNode.m_hit.shadingContext(lightContextOk);
    if (!lightContextOk[0]) {
      return false;
    }
    dir = thisNode.m_hit.getMaterial()!.getEdf()!.phongEdfSample(
      lightContext,
      XxdfComponentFlag.DIFFUSE_COMPONENT,
      x1,
      x2,
      thisNode.m_bsdfEval,
      pdfDir
    );

    if (pdfDir[0]! < VSDK.EPSILON) {
      return false;
    }

    thisNode.m_rayType = PathRayType.STARTS;
    newNode.m_inBsdf = thisNode.m_outBsdf;

    if (!this.sampleTransfer(sceneQuery, sceneSceneQueryWorkspace, sceneBackground, thisNode, newNode, dir, pdfDir[0]!)) {
      thisNode.m_rayType = PathRayType.STOPS;
      return false;
    }

    thisNode.m_bsdfComp.Clear();
    thisNode.m_bsdfComp.Fill(thisNode.m_bsdfEval, BsdfComponent.BRDF_DIFFUSE_COMPONENT);

    thisNode.m_usedComponents = 0;
    newNode.m_accUsedComponents = thisNode.m_accUsedComponents | thisNode.m_usedComponents;

    newNode.accumulatedRussianRouletteFactors = thisNode.accumulatedRussianRouletteFactors;

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
    void camera;
    void flags;
    let pdfDir: number;

    if (thisNode.m_hit.getMaterial() === null || thisNode.m_hit.getMaterial()!.getEdf() === null) {
      VsdkLogger.reportMessage(this, VsdkLogger.ERROR, "CLightDirSampler::evalPdf", "No EDF");
      return 0.0;
    }

    let outDir = new Vector3Dd();

    outDir = newNode.m_hit.getPoint().subtract(thisNode.m_hit.getPoint());
    const dist2 = outDir.lengthSquared();
    const dist = globalThis.Math.sqrt(dist2);
    outDir = outDir.multiplyByInverse(dist, Numeric.EPSILON);

    const outPdf = [0.0];
    thisNode.m_hit.getMaterial()!.getEdf()!.phongEdfEval(
      thisNode.m_hit.shadingContext(),
      outDir,
      XxdfComponentFlag.DIFFUSE_COMPONENT,
      outPdf
    );
    pdfDir = outPdf[0]!;

    if (pdfDir < 0.0) {
      return 0.0;
    }

    const cosA = -outDir.dotProduct(newNode.m_normal);

    const pdf = pdfDir * cosA / dist2;
    if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
      probabilityDensityFunction[0] = pdf;
    }
    if (probabilityDensityFunctionRR !== null && probabilityDensityFunctionRR.length > 0) {
      probabilityDensityFunctionRR[0] = 1.0;
    }

    return pdf;
  }
}
