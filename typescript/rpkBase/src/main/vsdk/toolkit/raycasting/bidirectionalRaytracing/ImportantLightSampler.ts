import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { RayHitFlag } from "../../environment/geometry/elements/RayHitFlag";
import { XxdfComponentFlag } from "../../material/XxdfComponentFlag";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { SceneQuery } from "../../scene/SceneQuery";
import type { SceneQueryWorkspace } from "../../scene/SceneQuery";
import { Patch } from "../../environment/geometry/elements/Patch";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { NextEventSampler } from "../raytracing/NextEventSampler";
import { Sampler } from "../raytracing/Sampler";
import { LightList } from "./LightList";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class ImportantLightSampler extends NextEventSampler {
  private lightList: LightList | null;

  public constructor(inLightList: LightList | null) {
    super();
    this.lightList = inLightList;
  }

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
    void sceneQuery;
    void sceneBackground;
    void prevNode;
    void doRR;

    let pdfLight: number;
    let pdfPoint: number;
    let light: Patch | null;
    let point = new Vector3Df();

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
        let invNormal = new Vector3Df();

        invNormal = thisNode.m_normal.multiply(-1.0);

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
      let dir = new Vector3Df(0.0, 0.0, 0.0);

      if (light.material !== null && light.material.getEdf() !== null) {
        dir = light.material.getEdf()!.phongEdfSample(null, flags, x1, x2, null, pdf);
      }

      point = thisNode.m_hit.getPoint().add(dir);

      newNode.m_hit.init(light, point, null, light.material);

      newNode.m_inDirT = dir.multiply(-1.0);
      newNode.m_inDirF = dir;
      newNode.m_normal = dir;

      pdfPoint = pdf[0]!;
    }
    else {
      point = light.uniformPoint(x1, x2)!;

      pdfPoint = 1.0 / light.getArea();

      newNode.m_hit.init(light, point, light.getNormal(), light.material);
      const normal = [new Vector3Df()];
      newNode.m_hit.shadingNormal(normal);
      newNode.m_hit.setNormal(normal[0]!);
      newNode.m_normal = newNode.m_hit.getNormal();
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
      if (localPdf >= VSDK.EPSILON && (newNode.m_hit.getPatch() as Patch).getArea() > VSDK.EPSILON) {
        localPdf = localPdf / (newNode.m_hit.getPatch() as Patch).getArea();
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
