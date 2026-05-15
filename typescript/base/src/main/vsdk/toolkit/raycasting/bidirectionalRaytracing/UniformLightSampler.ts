import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { XxdfComponentFlag } from "../../material/XxdfComponentFlag";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { Patch } from "../../environment/geometry/elements/Patch";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { NextEventSampler } from "../raytracing/NextEventSampler";
import { Sampler } from "../raytracing/Sampler";
import { LightList } from "./LightList";
import { LightListIterator } from "./LightListIterator";

export class UniformLightSampler extends NextEventSampler {
  private lightList: LightList | null;
  private iterator: LightListIterator | null;
  private currentPatch: Patch | null;
  private unitsActive: boolean;

  public constructor(inLightList: LightList | null) {
    super();
    this.lightList = inLightList;
    this.iterator = null;
    this.currentPatch = null;
    this.unitsActive = false;
  }

  public override ActivateFirstUnit(): boolean {
    if (this.lightList === null) {
      return false;
    }

    if (this.iterator === null) {
      this.iterator = new LightListIterator(this.lightList);
    }

    this.currentPatch = this.iterator.First(this.lightList);

    if (this.currentPatch !== null) {
      this.unitsActive = true;
      return true;
    }
    return false;
  }

  public override ActivateNextUnit(): boolean {
    this.currentPatch = this.iterator!.Next();
    return this.currentPatch !== null;
  }

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
    void sceneVoxelGrid;
    void sceneBackground;
    void prevNode;
    void doRR;

    let pdfLight: number;
    let pdfPoint: number;
    let light: Patch | null;
    const point = new Vector3D();

    newNode.m_depth = 0;
    newNode.m_rayType = PathRayType.STOPS;

    newNode.m_useBsdf = null;
    newNode.m_inBsdf = null;
    newNode.m_outBsdf = null;
    newNode.m_G = 1.0;

    if (this.unitsActive) {
      if (this.currentPatch !== null) {
        light = this.currentPatch;
        pdfLight = 1.0;
      }
      else {
        VsdkLogger.warning("sample Unit Light Node", "No valid light selected");
        return false;
      }
    }
    else {
      if (this.lightList === null) {
        VsdkLogger.warning("FillLightNode", "No light list available");
        return false;
      }
      const localX1 = [x1];
      const outPdfLight = [0.0];
      light = this.lightList.sample(localX1, outPdfLight);
      x1 = localX1[0];
      pdfLight = outPdfLight[0];

      if (light === null) {
        VsdkLogger.warning("FillLightNode", "No light found");
        return false;
      }
    }

    if (light.hasZeroVertices()) {
      const pdf = [0.0];
      let dir = new Vector3D(0.0, 0.0, 0.0);

      if (light.material !== null && light.material.getEdf() !== null && thisNode !== null) {
        dir = light.material.getEdf()!.phongEdfSample(thisNode.m_hit, flags, x1, x2, null, pdf);
      }

      point.subtraction(thisNode!.m_hit.getPoint(), dir);

      newNode.m_hit.init(light, point, null, light.material);

      newNode.m_inDirT.scaledCopy(-1.0, dir);
      newNode.m_inDirF.copy(dir);
      newNode.m_normal.copy(dir);

      pdfPoint = pdf[0];
    }
    else {
      light.uniformPoint(x1, x2, point);
      pdfPoint = 1.0 / light.area;

      newNode.m_hit.init(light, point, light.normal, light.material);
      const normal = new Vector3D();
      newNode.m_hit.shadingNormal(normal);
      newNode.m_hit.setNormal(normal);
      newNode.m_normal.copy(newNode.m_hit.getNormal());
    }

    newNode.m_pdfFromPrev = pdfLight * pdfPoint;

    newNode.m_accUsedComponents = 0;

    newNode.accumulatedRussianRouletteFactors = 1.0;

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
    void thisNode;
    void flags;
    let localPdf: number;

    if (this.unitsActive) {
      localPdf = 1.0;
    }
    else {
      if (this.lightList === null) {
        return 0.0;
      }
      const position = newNode.m_hit.getPoint();
      localPdf = this.lightList.evalPdf(newNode.m_hit.getPatch() as Patch, position);
    }

    if ((newNode.m_hit.getPatch() as Patch).hasZeroVertices()) {
      let pdfDir: number;
      if ((newNode.m_hit.getPatch() as Patch).material === null || (newNode.m_hit.getPatch() as Patch).material!.getEdf() === null) {
        pdfDir = 0.0;
      }
      else {
        const outPdfDir = [0.0];
        (newNode.m_hit.getPatch() as Patch).material!.getEdf()!.phongEdfEval(
          null as any,
          newNode.m_inDirF,
          XxdfComponentFlag.DIFFUSE_COMPONENT | XxdfComponentFlag.GLOSSY_COMPONENT | XxdfComponentFlag.SPECULAR_COMPONENT,
          outPdfDir
        );
        pdfDir = outPdfDir[0];
      }

      localPdf *= pdfDir;
    }
    else {
      if (localPdf >= Numeric.EPSILON && (newNode.m_hit.getPatch() as Patch).area > Numeric.EPSILON) {
        localPdf = localPdf / (newNode.m_hit.getPatch() as Patch).area;
      }
      else {
        localPdf = 0.0;
      }
    }

    if (pdf !== null && pdf.length > 0) {
      pdf[0] = localPdf;
    }
    if (pdfRR !== null && pdfRR.length > 0) {
      pdfRR[0] = 1.0;
    }

    return localPdf;
  }
}
