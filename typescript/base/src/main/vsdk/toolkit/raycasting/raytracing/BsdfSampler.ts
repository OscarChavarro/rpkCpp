import { ColorRgb } from "../../common/color/ColorRgb";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { Sampler } from "./Sampler";
import { SurfaceSampler } from "./SurfaceSampler";

export class BsdfSampler extends SurfaceSampler {
  public override sample(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
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

    let dir = new Vector3D(0.0, 0.0, 0.0);

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

    if (pdfDir[0] <= Numeric.EPSILON) {
      return false;
    }

    newNode.accumulatedRussianRouletteFactors = thisNode.accumulatedRussianRouletteFactors;
    if (doRR) {
      let albedo = new ColorRgb();
      albedo.clear();
      if (thisNode.m_useBsdf !== null) {
        albedo = thisNode.m_useBsdf.splitBsdfScatteredPower(thisNode.m_hit.shadingContext(), flags);
      }
      newNode.accumulatedRussianRouletteFactors *= albedo.average();
    }

    SurfaceSampler.DetermineRayType(thisNode, newNode, dir);

    if (!this.sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, dir, pdfDir[0])) {
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

      prevNode.m_rrPdfFromNext = pdfRR[0];
      prevNode.m_pdfFromNext = pdfDirI[0] * thisNode.m_G / cosI;
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

    const outDir = new Vector3D();

    outDir.subtraction(newNode.m_hit.getPoint(), thisNode.m_hit.getPoint());
    const dist2 = outDir.norm2();
    const dist = globalThis.Math.sqrt(dist2);
    outDir.inverseScaledCopy(dist, outDir, Numeric.EPSILON_FLOAT);

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

    outPdf[0] = pdfDir[0] * cosA / dist2;

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
    const outDir = new Vector3D();

    const outPdf = pdf ?? pdfH;
    const outPdfRR = pdfRR ?? pdfRRH;

    outDir.subtraction(prevNode.m_hit.getPoint(), thisNode.m_hit.getPoint());
    outDir.normalize(Numeric.EPSILON_FLOAT);

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

    outPdf[0] = pdfDir[0] * thisNode.m_G / cosB;

    return outPdf[0] * outPdfRR[0];
  }
}
