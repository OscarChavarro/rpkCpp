import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { BsdfComponent } from "../../material/BsdfComponent";
import { Niederreiter31 } from "../../numericalAnalysis/quasiMonteCarlo/Niederreiter31";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { NextEventSampler } from "./NextEventSampler";
import { SampleConnectionFlags } from "./SampleConnectionFlags";
import { Sampler } from "./Sampler";
import { SurfaceSampler } from "./SurfaceSampler";

export class SamplerConfig {
  public pointSampler: Sampler | null;
  public dirSampler: Sampler | null;
  public surfaceSampler: SurfaceSampler | null;
  public neSampler: NextEventSampler | null;

  public m_useQMC: boolean;
  public m_qmcDepth: number;
  public m_qmcSeed: number[] | null;

  public minDepth: number;
  public maxDepth: number;

  public constructor() {
    this.m_useQMC = false;
    this.m_qmcDepth = 0;
    this.minDepth = 0;
    this.maxDepth = 0;
    this.pointSampler = null;
    this.dirSampler = null;
    this.surfaceSampler = null;
    this.neSampler = null;
    this.m_qmcSeed = null;

    this.clearVars();
    this.init();
  }

  public clearVars(): void {
    this.pointSampler = null;
    this.dirSampler = null;
    this.surfaceSampler = null;
    this.neSampler = null;
    this.m_qmcSeed = null;
  }

  public releaseVars(): void {
    this.pointSampler = null;
    this.dirSampler = null;
    this.surfaceSampler = null;
    this.neSampler = null;
    this.m_qmcSeed = null;
  }

  public init(): void;
  public init(useQMC: boolean, qmcDepth: number): void;
  public init(useQMC?: boolean, qmcDepth?: number): void {
    this.m_useQMC = useQMC ?? false;
    this.m_qmcDepth = qmcDepth ?? 0;

    if (this.m_useQMC) {
      this.m_qmcSeed = new Array<number>(this.m_qmcDepth);

      for (let i = 0; i < this.m_qmcDepth; i++) {
        this.m_qmcSeed[i] = globalThis.Math.floor(globalThis.Math.random() * 2147483647.0);
        process.stdout.write(`Seed ${this.m_qmcSeed[i]}\n`);
      }
    }
    else {
      this.m_qmcSeed = null;
    }
  }

  public getRand(depth: number, x1: number[], x2: number[]): void {
    if (x1.length === 0 || x2.length === 0) {
      return;
    }

    if (!this.m_useQMC || depth >= this.m_qmcDepth || this.m_qmcSeed === null) {
      x1[0] = globalThis.Math.random();
      x2[0] = globalThis.Math.random();
    }
    else {
      if (depth === 0 || depth === 2) {
        x1[0] = globalThis.Math.random();
        x2[0] = globalThis.Math.random();
      }
      else if (depth === 1) {
        const nrs = Niederreiter31.niederreiter31(this.m_qmcSeed[1]++);
        x1[0] = nrs[0] * Niederreiter31.RECIP;
        x2[0] = nrs[1] * Niederreiter31.RECIP;
      }
      else {
        process.stdout.write(`Hmmmm MD ${this.m_qmcDepth} D${depth}\n`);
        x1[0] = globalThis.Math.random();
        x2[0] = globalThis.Math.random();
      }
    }
  }

  public traceNode(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    nextNode: SimpleRaytracingPathNode | null,
    x1: number,
    x2: number,
    flags: number
  ): SimpleRaytracingPathNode | null {
    if (nextNode === null) {
      nextNode = new SimpleRaytracingPathNode();
    }

    const lastNode = nextNode.previous();

    if (lastNode === null) {
      if (this.pointSampler === null) {
        VsdkLogger.warning("SamplerConfig::traceNode", "Point sampler not configured");
        return null;
      }
      if (!this.pointSampler.sample(camera, sceneVoxelGrid, sceneBackground, null, null, nextNode, x1, x2)) {
        VsdkLogger.warning("SamplerConfig::traceNode", "Point sampler failed");
        return null;
      }
    }
    else if (lastNode.m_depth === 0) {
      if (lastNode.m_depth + 1 < this.maxDepth) {
        if (this.dirSampler === null) {
          VsdkLogger.warning("SamplerConfig::traceNode", "Direction sampler not configured");
          lastNode.m_rayType = PathRayType.STOPS;
          return null;
        }
        if (!this.dirSampler.sample(camera, sceneVoxelGrid, sceneBackground, null, lastNode, nextNode, x1, x2)) {
          lastNode.m_rayType = PathRayType.STOPS;
          return null;
        }
      }
      else {
        lastNode.m_rayType = PathRayType.STOPS;
        return null;
      }
    }
    else {
      if (lastNode.m_depth + 1 < this.maxDepth) {
        if (this.surfaceSampler === null) {
          VsdkLogger.warning("SamplerConfig::traceNode", "Surface sampler not configured");
          lastNode.m_rayType = PathRayType.STOPS;
          return null;
        }
        if (!this.surfaceSampler.sample(
          camera,
          sceneVoxelGrid,
          sceneBackground,
          lastNode.previous(),
          lastNode,
          nextNode,
          x1,
          x2,
          lastNode.m_depth >= this.minDepth,
          flags
        )) {
          lastNode.m_rayType = PathRayType.STOPS;
          return null;
        }
      }
      else {
        lastNode.m_rayType = PathRayType.STOPS;
        return null;
      }
    }

    if (nextNode.m_depth > 0) {
      nextNode.assignBsdfAndNormal();
    }

    return nextNode;
  }

  public tracePath(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    nextNode: SimpleRaytracingPathNode | null,
    flags: number
  ): SimpleRaytracingPathNode | null {
    const x1 = [0.0];
    const x2 = [0.0];

    if (nextNode === null || nextNode.previous() === null) {
      this.getRand(0, x1, x2);
    }
    else {
      this.getRand(nextNode.previous()!.m_depth + 1, x1, x2);
    }

    const sampledNode = this.traceNode(camera, sceneVoxelGrid, sceneBackground, nextNode, x1[0], x2[0], flags);

    let continueTrace = sampledNode !== null;
    if (continueTrace && sceneBackground !== null && sampledNode!.ends()) {
      continueTrace = false;
    }

    if (continueTrace) {
      sampledNode!.ensureNext();
      this.tracePath(camera, sceneVoxelGrid, sceneBackground, sampledNode!.next(), flags);
    }

    return sampledNode;
  }

  public tracePathDefault(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    nextNode: SimpleRaytracingPathNode | null
  ): SimpleRaytracingPathNode | null {
    return this.tracePath(camera, sceneVoxelGrid, sceneBackground, nextNode, Sampler.BSDF_ALL_COMPONENTS);
  }

  public static pathNodeConnect(
    camera: Camera,
    nodeX: SimpleRaytracingPathNode,
    nodeY: SimpleRaytracingPathNode,
    eyeConfig: SamplerConfig,
    lightConfig: SamplerConfig,
    flags: number,
    bsdfFlagsE: number,
    bsdfFlagsL: number,
    pDirEl: Vector3D | null
  ): number;
  public static pathNodeConnect(
    camera: Camera,
    nodeX: SimpleRaytracingPathNode,
    nodeY: SimpleRaytracingPathNode,
    eyeConfig: SamplerConfig,
    lightConfig: SamplerConfig,
    flags: number,
    pDirEl: Vector3D | null
  ): number;
  public static pathNodeConnect(
    camera: Camera,
    nodeX: SimpleRaytracingPathNode,
    nodeY: SimpleRaytracingPathNode,
    eyeConfig: SamplerConfig,
    lightConfig: SamplerConfig,
    flags: number,
    bsdfFlagsEOrDir: number | Vector3D | null,
    bsdfFlagsL?: number,
    pDirEl?: Vector3D | null
  ): number {
    let bsdfFlagsE: number;
    let bsdfFlagsLValue: number;
    let outDirEl: Vector3D | null;

    if (typeof bsdfFlagsEOrDir === "number") {
      bsdfFlagsE = bsdfFlagsEOrDir;
      bsdfFlagsLValue = bsdfFlagsL ?? Sampler.BSDF_ALL_COMPONENTS;
      outDirEl = pDirEl ?? null;
    }
    else {
      bsdfFlagsE = Sampler.BSDF_ALL_COMPONENTS;
      bsdfFlagsLValue = Sampler.BSDF_ALL_COMPONENTS;
      outDirEl = bsdfFlagsEOrDir as Vector3D | null;
    }

    const nodeEP = nodeX.previous();
    const nodeLP = nodeY.previous();
    const pdf = [0.0];
    const pdfRR = [0.0];

    const dirLE = new Vector3D();
    const dirEL = new Vector3D();

    dirEL.subtraction(nodeY.m_hit.getPoint(), nodeX.m_hit.getPoint());
    const dist2 = dirEL.norm2();
    const dist = globalThis.Math.sqrt(dist2);
    dirEL.inverseScaledCopy(dist, dirEL, Numeric.EPSILON_FLOAT);
    dirLE.scaledCopy(-1.0, dirEL);

    if (outDirEl !== null) {
      outDirEl.copy(dirEL);
    }

    if ((flags & SampleConnectionFlags.CONNECT_EL) !== 0) {
      if (nodeX.m_depth < eyeConfig.maxDepth - 1) {
        if (nodeEP === null) {
          if (eyeConfig.dirSampler === null) {
            pdf[0] = 0.0;
          }
          else {
            pdf[0] = eyeConfig.dirSampler.evalPDF(camera, nodeX, nodeY);
          }
          pdfRR[0] = 1.0;
        }
        else if (eyeConfig.surfaceSampler === null) {
          pdf[0] = 0.0;
          pdfRR[0] = 0.0;
        }
        else {
          eyeConfig.surfaceSampler.evalPDF(camera, nodeX, nodeY, bsdfFlagsE, pdf, pdfRR);
        }
      }
      else {
        pdf[0] = 0.0;
        pdfRR[0] = 0.0;
      }

      nodeY.m_pdfFromNext = pdf[0];
      nodeY.m_rrPdfFromNext = pdfRR[0];

      if ((flags & SampleConnectionFlags.FILL_OTHER_PDF) !== 0 && nodeLP !== null) {
        if (nodeX.m_depth < eyeConfig.maxDepth - 2 && lightConfig.surfaceSampler !== null) {
          lightConfig.surfaceSampler.EvalPDFPrev(nodeX, nodeY, nodeLP, bsdfFlagsE, pdf, pdfRR);
        }
        else {
          pdf[0] = 0.0;
          pdfRR[0] = 0.0;
        }

        nodeLP.m_pdfFromNext = pdf[0];
        nodeLP.m_rrPdfFromNext = pdfRR[0];
      }
    }

    if ((flags & SampleConnectionFlags.CONNECT_LE) !== 0) {
      if (nodeY.m_depth < lightConfig.maxDepth - 1) {
        if (nodeLP === null) {
          if (lightConfig.dirSampler === null) {
            pdf[0] = 0.0;
          }
          else {
            pdf[0] = lightConfig.dirSampler.evalPDF(camera, nodeY, nodeX);
          }
          pdfRR[0] = 1.0;
        }
        else if (lightConfig.surfaceSampler === null) {
          pdf[0] = 0.0;
          pdfRR[0] = 0.0;
        }
        else {
          lightConfig.surfaceSampler.evalPDF(camera, nodeY, nodeX, bsdfFlagsLValue, pdf, pdfRR);
        }
      }
      else {
        pdf[0] = 0.0;
        pdfRR[0] = 0.0;
      }

      nodeX.m_pdfFromNext = pdf[0];
      nodeX.m_rrPdfFromNext = pdfRR[0];

      if ((flags & SampleConnectionFlags.FILL_OTHER_PDF) !== 0 && nodeEP !== null) {
        if (nodeY.m_depth < lightConfig.maxDepth - 2 && lightConfig.surfaceSampler !== null) {
          lightConfig.surfaceSampler.EvalPDFPrev(nodeY, nodeX, nodeEP, bsdfFlagsLValue, pdf, pdfRR);
        }
        else {
          pdf[0] = 0.0;
          pdfRR[0] = 0.0;
        }

        nodeEP.m_pdfFromNext = pdf[0];
        nodeEP.m_rrPdfFromNext = pdfRR[0];
      }
    }

    if (nodeEP === null) {
      nodeX.m_bsdfEval.setMonochrome(1.0);
      nodeX.m_bsdfComp.Clear();
      nodeX.m_bsdfComp.Fill(nodeX.m_bsdfEval, BsdfComponent.BRDF_DIFFUSE_COMPONENT);
    }
    else if (eyeConfig.surfaceSampler !== null) {
      nodeX.m_bsdfEval = eyeConfig.surfaceSampler.DoBsdfEval(
        nodeX.m_useBsdf,
        nodeX.m_hit,
        nodeX.m_inBsdf,
        nodeX.m_outBsdf,
        nodeX.m_inDirF,
        dirEL,
        bsdfFlagsE,
        nodeX.m_bsdfComp
      );
    }
    else {
      nodeX.m_bsdfEval.clear();
      nodeX.m_bsdfComp.Clear();
    }

    if (nodeLP === null) {
      if (nodeY.m_hit.getMaterial() === null || nodeY.m_hit.getMaterial()!.getEdf() === null) {
        nodeY.m_bsdfEval.clear();
      }
      else {
        nodeY.m_bsdfEval = nodeY.m_hit.getMaterial()!.getEdf()!.phongEdfEval(
          nodeY.m_hit,
          dirLE,
          bsdfFlagsLValue,
          null
        );
      }
      nodeY.m_bsdfComp.Clear();
      nodeY.m_bsdfComp.Fill(nodeY.m_bsdfEval, BsdfComponent.BRDF_DIFFUSE_COMPONENT);
    }
    else if (lightConfig.surfaceSampler !== null) {
      nodeY.m_bsdfEval = lightConfig.surfaceSampler.DoBsdfEval(
        nodeY.m_useBsdf,
        nodeY.m_hit,
        nodeY.m_inBsdf,
        nodeY.m_outBsdf,
        nodeY.m_inDirF,
        dirLE,
        bsdfFlagsLValue,
        nodeY.m_bsdfComp
      );
    }
    else {
      nodeY.m_bsdfEval.clear();
      nodeY.m_bsdfComp.Clear();
    }

    const cosA = -dirEL.dotProduct(nodeY.m_normal);
    const geom = globalThis.Math.abs(cosA * nodeX.m_normal.dotProduct(dirEL) / dist2);

    return geom;
  }
}
