import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { RayHitFlag } from "../../environment/geometry/elements/RayHitFlag";
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

export class ImportantLightSampler extends NextEventSampler {
  private lightList: LightList | null;

  public constructor(inLightList: LightList | null) {
    super();
    this.lightList = inLightList;
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

    if (this.lightList === null || thisNode === null) {
      return false;
    }

    const localX1 = [x1];
    const outPdfLight = [0.0];
    if ((thisNode.m_hit.getFlags() & RayHitFlag.BACK) !== 0) {
      if (thisNode.m_outBsdf === null) {
        const invNormal = new Vector3D();

        invNormal.scaledCopy(-1.0, thisNode.m_normal);

        const position = thisNode.m_hit.getPoint();
        light = this.lightList.sampleImportant(position, invNormal, localX1, outPdfLight);
      }
      else {
        light = null;
      }
    }
    else if (thisNode.m_inBsdf === null) {
      const position = thisNode.m_hit.getPoint();
      light = this.lightList.sampleImportant(position, thisNode.m_normal, localX1, outPdfLight);
    }
    else {
      light = null;
    }

    x1 = localX1[0]!;
    pdfLight = outPdfLight[0]!;

    if (light === null) {
      return false;
    }

    if (light.hasZeroVertices()) {
      const pdf = [0.0];
      let dir = new Vector3D(0.0, 0.0, 0.0);

      if (light.material !== null && light.material.getEdf() !== null) {
        dir = light.material.getEdf()!.phongEdfSample(null, flags, x1, x2, null, pdf);
      }

      point.addition(thisNode.m_hit.getPoint(), dir);

      newNode.m_hit.init(light, point, null, light.material);

      newNode.m_inDirT.scaledCopy(-1.0, dir);
      newNode.m_inDirF.copy(dir);
      newNode.m_normal.copy(dir);

      pdfPoint = pdf[0]!;
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
    void flags;
    let localPdf: number;
    let pdfDir: number;

    if (this.lightList === null) {
      return 0.0;
    }
    const newPosition = newNode.m_hit.getPoint();
    const thisPosition = thisNode.m_hit.getPoint();
    localPdf = this.lightList.evalPdfImportant(
      newNode.m_hit.getPatch() as Patch,
      newPosition,
      thisPosition,
      thisNode.m_normal
    );

    if ((newNode.m_hit.getPatch() as Patch).hasZeroVertices()) {
      if ((newNode.m_hit.getPatch() as Patch).material === null || (newNode.m_hit.getPatch() as Patch).material!.getEdf() === null) {
        pdfDir = 0.0;
      }
      else {
        const outPdfDir = [0.0];
        (newNode.m_hit.getPatch() as Patch).material!.getEdf()!.phongEdfEval(
          null,
          newNode.m_inDirF,
          XxdfComponentFlag.DIFFUSE_COMPONENT | XxdfComponentFlag.GLOSSY_COMPONENT | XxdfComponentFlag.SPECULAR_COMPONENT,
          outPdfDir
        );
        pdfDir = outPdfDir[0]!;
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
