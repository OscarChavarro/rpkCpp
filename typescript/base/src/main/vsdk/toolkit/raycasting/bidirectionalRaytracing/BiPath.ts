import { ColorRgb } from "../../common/color/ColorRgb";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { BidirectionalPathRaytracerConfig } from "./BidirectionalPathRaytracerConfig";

export class BiPath {
  public m_eyePath: SimpleRaytracingPathNode | null = null;
  public m_eyeEndNode: SimpleRaytracingPathNode | null = null;
  public m_eyeSize = 0;

  public m_lightPath: SimpleRaytracingPathNode | null = null;
  public m_lightEndNode: SimpleRaytracingPathNode | null = null;
  public m_lightSize = 0;

  public m_pdfLNE = 0.0;

  public m_dirEL: Vector3D;
  public m_dirLE: Vector3D;

  public m_geomConnect = 0.0;

  public constructor() {
    this.m_dirEL = new Vector3D();
    this.m_dirLE = new Vector3D();
    this.init();
  }

  public init(): void {
    this.m_lightPath = null;
    this.m_lightEndNode = null;
    this.m_eyePath = null;
    this.m_eyeEndNode = null;
    this.m_eyeSize = 0;
    this.m_lightSize = 0;
    this.m_pdfLNE = 0.0;
    this.m_geomConnect = 0.0;
  }

  public evalRadiance(): ColorRgb {
    const col = new ColorRgb();
    let factor = 1.0;

    col.setMonochrome(1.0);

    let node = this.m_eyePath;

    for (let i = 0; i < this.m_eyeSize; i++) {
      col.selfScalarProduct(node!.m_bsdfEval);
      factor *= node!.m_G;
      node = node!.next();
    }

    node = this.m_lightPath;

    for (let i = 0; i < this.m_lightSize; i++) {
      col.selfScalarProduct(node!.m_bsdfEval);
      factor *= node!.m_G;
      node = node!.next();
    }

    factor *= this.m_geomConnect;

    col.scale(factor);

    return col;
  }

  public evalPdfAcc(): number {
    let pdfAcc = 1.0;

    let node = this.m_eyePath;

    for (let i = 0; i < this.m_eyeSize; i++) {
      pdfAcc *= node!.m_pdfFromPrev;
      node = node!.next();
    }

    node = this.m_lightPath;

    for (let i = 0; i < this.m_lightSize; i++) {
      pdfAcc *= node!.m_pdfFromPrev;
      node = node!.next();
    }

    return pdfAcc;
  }

  public evalPdfAndWeight(
    baseConfig: BidirectionalPathRaytracerConfig,
    pPdf: number[] | null,
    pWeight: number[] | null
  ): number {
    let currentConnect: number;
    const pdfAcc = this.evalPdfAcc();
    let pdfSum: number;
    let currentPdf: number;
    let newPdf: number;
    let c: number;
    let nextNode: SimpleRaytracingPathNode | null;
    let realPdf: number;
    let tmpPdf: number;
    let weight: number;

    currentConnect = this.m_eyeSize;

    currentPdf = pdfAcc;

    if (this.m_eyeSize === 1) {
      c = baseConfig.totalSamples;
    }
    else {
      c = baseConfig.samplesPerPixel;
    }

    if (this.m_lightSize === 1) {
      realPdf = pdfAcc * this.m_pdfLNE / this.m_lightEndNode!.m_pdfFromPrev;
    }
    else {
      realPdf = pdfAcc;
    }

    weight = SimpleRaytracingPathNode.multipleImportanceSampling(c * realPdf);
    pdfSum = weight;

    nextNode = this.m_lightEndNode;

    while (currentConnect < this.m_eyeSize + this.m_lightSize) {
      currentConnect++;

      newPdf = currentPdf * nextNode!.m_pdfFromNext / nextNode!.m_pdfFromPrev;

      if (currentConnect - 1 >= baseConfig.minimumPathDepth) {
        newPdf *= nextNode!.m_rrPdfFromNext;
      }

      if (currentConnect === 1) {
        c = baseConfig.totalSamples;
      }
      else {
        c = baseConfig.samplesPerPixel;
      }

      if (currentConnect === this.m_eyeSize + this.m_lightSize - 1) {
        tmpPdf = newPdf * this.m_pdfLNE / nextNode!.previous()!.m_pdfFromPrev;
      }
      else {
        tmpPdf = newPdf;
      }

      pdfSum += SimpleRaytracingPathNode.multipleImportanceSampling(c * tmpPdf);

      currentPdf = newPdf;
      nextNode = nextNode!.previous();
    }

    nextNode = this.m_eyeEndNode;
    currentConnect = this.m_eyeSize;
    currentPdf = pdfAcc;

    while (currentConnect > 1) {
      currentConnect--;

      newPdf = currentPdf * nextNode!.m_pdfFromNext / nextNode!.m_pdfFromPrev;

      if (this.m_eyeSize + this.m_lightSize - 2 - currentConnect >= baseConfig.minimumPathDepth) {
        newPdf *= nextNode!.m_rrPdfFromNext;
      }

      if (currentConnect === 1) {
        c = baseConfig.totalSamples;
      }
      else {
        c = baseConfig.samplesPerPixel;
      }

      if (currentConnect === this.m_eyeSize + this.m_lightSize - 1) {
        tmpPdf = newPdf * this.m_pdfLNE / nextNode!.m_pdfFromNext;
      }
      else {
        tmpPdf = newPdf;
      }

      pdfSum += SimpleRaytracingPathNode.multipleImportanceSampling(c * tmpPdf);

      currentPdf = newPdf;
      nextNode = nextNode!.previous();
    }

    weight = weight / pdfSum;

    if (pWeight !== null && pWeight.length > 0) {
      pWeight[0] = weight;
    }

    if (pPdf !== null && pPdf.length > 0) {
      pPdf[0] = realPdf;
    }

    return weight / realPdf;
  }
}
