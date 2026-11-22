import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { SceneQuery } from "../../scene/SceneQuery";
import type { SceneQueryWorkspace } from "../../scene/SceneQuery";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { Sampler } from "./Sampler";
import { SurfaceSampler } from "./SurfaceSampler";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class BsdfSampler extends SurfaceSampler {
  public override sample(
    camera: Camera,
    sceneQuery: SceneQuery,
    sceneSceneQueryWorkspace: SceneQueryWorkspace,
    sceneBackground: Background | null,
    prevNode: SimpleRaytracingPathNode | null,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    x1: number,
    x2: number,
    doRR = false,
    flags = Sampler.BSDF_ALL_COMPONENTS
  ): boolean {
    void camera;
    const pdfDir = [0.0];

    let dir = new Vector3Df(0.0, 0.0, 0.0);

    if (thisNode.m_useBsdf !== null) {
      const thisContext = thisNode.m_hit.shadingContext();
      dir = thisNode.m_useBsdf.sample(
        thisContext,
        thisNode.m_inBsdf,
        thisNode.m_outBsdf,
        thisNode.m_inDirF,
        doRR ? 1 : 0,
        flags,
        x1,
        x2,
        pdfDir
      );
    }

    if (pdfDir[0]! <= VSDK.EPSILON) {
      return false;
    }

    newNode.accumulatedRussianRouletteFactors = thisNode.accumulatedRussianRouletteFactors;
    if (doRR) {
      let albedo = new ColorRgbMutable();
      albedo.clear();
      if (thisNode.m_useBsdf !== null) {
        albedo = thisNode.m_useBsdf.splitBsdfScatteredPower(thisNode.m_hit.shadingContext(), flags);
      }
      newNode.accumulatedRussianRouletteFactors *= albedo.average();
    }

    SurfaceSampler.DetermineRayType(thisNode, newNode, dir);

    if (!this.sampleTransfer(sceneQuery, sceneSceneQueryWorkspace, sceneBackground, thisNode, newNode, dir, pdfDir[0]!)) {
      thisNode.m_rayType = PathRayType.STOPS;
      return false;
    }

    thisNode.m_bsdfEval = this.DoBsdfEval(
      thisNode.m_useBsdf,
      thisNode.m_hit,
      thisNode.m_inBsdf,
      thisNode.m_outBsdf,
      thisNode.m_inDirF,
      newNode.m_inDirT,
      flags,
      thisNode.m_bsdfComp
    );

    thisNode.m_usedComponents = flags;
    newNode.m_accUsedComponents = thisNode.m_accUsedComponents | thisNode.m_usedComponents;

    if (this.m_computeFromNextPdf && prevNode !== null) {
      const cosI = thisNode.m_normal.dotProduct(thisNode.m_inDirF);
      const pdfDirI = [0.0];
      const pdfRR = [0.0];

      if (thisNode.m_useBsdf !== null) {
        thisNode.m_useBsdf.evaluateProbabilityDensityFunction(
          thisNode.m_hit.shadingContext(),
          thisNode.m_outBsdf,
          thisNode.m_inBsdf,
          newNode.m_inDirT,
          thisNode.m_inDirF,
          flags,
          pdfDirI,
          pdfRR
        );
      }

      prevNode.m_rrPdfFromNext = pdfRR[0]!;
      prevNode.m_pdfFromNext = pdfDirI[0]! * thisNode.m_G / cosI;
    }

    return true;
  }

  public override evalPDF(
    camera: Camera,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    flags = Sampler.BSDF_ALL_COMPONENTS,
    pdf: number[] | null = null,
    pdfRR: number[] | null = null
  ): number {
    void camera;
    const pdfH = [0.0];
    const pdfRRH = [0.0];

    const outPdf = pdf ?? pdfH;
    const outPdfRR = pdfRR ?? pdfRRH;

    let outDir = new Vector3Df();

    outDir = newNode.m_hit.getPoint().subtract(thisNode.m_hit.getPoint());
    const dist2 = outDir.lengthSquared();
    const dist = globalThis.Math.sqrt(dist2);
    outDir = outDir.multiplyByInverse(dist, Numeric.EPSILON_FLOAT);

    const pdfDir = [0.0];
    outPdfRR[0] = 0.0;
    if (thisNode.m_useBsdf !== null) {
      thisNode.m_useBsdf.evaluateProbabilityDensityFunction(
        thisNode.m_hit.shadingContext(),
        thisNode.m_inBsdf,
        thisNode.m_outBsdf,
        thisNode.m_inDirF,
        outDir,
        flags,
        pdfDir,
        outPdfRR
      );
    }

    const cosA = -outDir.dotProduct(newNode.m_normal);

    outPdf[0] = pdfDir[0]! * cosA / dist2;

    return outPdf[0] * outPdfRR[0];
  }

  public override EvalPDFPrev(
    prevNode: SimpleRaytracingPathNode,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    flags = Sampler.BSDF_ALL_COMPONENTS,
    pdf: number[] | null = null,
    pdfRR: number[] | null = null
  ): number {
    void newNode;
    const pdfH = [0.0];
    const pdfRRH = [0.0];
    let outDir = new Vector3Df();

    const outPdf = pdf ?? pdfH;
    const outPdfRR = pdfRR ?? pdfRRH;

    outDir = prevNode.m_hit.getPoint().subtract(thisNode.m_hit.getPoint());
    outDir = outDir.normalized(Numeric.EPSILON_FLOAT);

    const pdfDir = [0.0];
    outPdfRR[0] = 0.0;
    if (thisNode.m_useBsdf !== null) {
      thisNode.m_useBsdf.evaluateProbabilityDensityFunction(
        thisNode.m_hit.shadingContext(),
        thisNode.m_outBsdf,
        thisNode.m_inBsdf,
        outDir,
        thisNode.m_inDirF,
        flags,
        pdfDir,
        outPdfRR
      );
    }

    const cosB = thisNode.m_inDirF.dotProduct(thisNode.m_normal);

    outPdf[0] = pdfDir[0]! * thisNode.m_G / cosB;

    return outPdf[0] * outPdfRR[0];
  }
}
