import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { BsdfComponent } from "../../material/BsdfComponent";
import { XxdfComponentFlag } from "../../material/XxdfComponentFlag";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { Sampler } from "../raytracing/Sampler";

export class LightDirSampler extends Sampler {
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
    void camera;
    void prevNode;
    void doRR;
    const pdfDir = [0.0];

    if (thisNode === null || thisNode.m_hit.getMaterial() === null || thisNode.m_hit.getMaterial()!.getEdf() === null) {
      VsdkLogger.error("CLightDirSampler::sample", "No EDF");
      return false;
    }

    let dir = new Vector3D(0.0, 0.0, 0.0);
    dir = thisNode.m_hit.getMaterial()!.getEdf()!.phongEdfSample(
      thisNode.m_hit,
      XxdfComponentFlag.DIFFUSE_COMPONENT,
      x1,
      x2,
      thisNode.m_bsdfEval,
      pdfDir
    );

    if (pdfDir[0] < Numeric.EPSILON) {
      return false;
    }

    thisNode.m_rayType = PathRayType.STARTS;
    newNode.m_inBsdf = thisNode.m_outBsdf;

    if (!this.sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, dir, pdfDir[0])) {
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
      VsdkLogger.error("CLightDirSampler::evalPdf", "No EDF");
      return 0.0;
    }

    const outDir = new Vector3D();

    outDir.subtraction(newNode.m_hit.getPoint(), thisNode.m_hit.getPoint());
    const dist2 = outDir.norm2();
    const dist = globalThis.Math.sqrt(dist2);
    outDir.inverseScaledCopy(dist, outDir, Numeric.EPSILON_FLOAT);

    const outPdf = [0.0];
    thisNode.m_hit.getMaterial()!.getEdf()!.phongEdfEval(
      thisNode.m_hit,
      outDir,
      XxdfComponentFlag.DIFFUSE_COMPONENT,
      outPdf
    );
    pdfDir = outPdf[0];

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
