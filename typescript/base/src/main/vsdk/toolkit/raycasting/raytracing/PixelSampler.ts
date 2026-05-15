import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { BsdfComponent } from "../../material/BsdfComponent";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { Sampler } from "./Sampler";

export class PixelSampler extends Sampler {
  private m_px: number;
  private m_py: number;

  public constructor() {
    super();
    this.m_px = 0.0;
    this.m_py = 0.0;
  }

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
    void prevNode;
    void doRR;
    void flags;

    const dir = new Vector3D();

    const xSample = this.m_px + camera.pixelWidth * x1;
    const ySample = this.m_py + camera.pixelHeight * x2;

    dir.combine3(camera.Z, xSample, camera.X, ySample, camera.Y);
    const distPixel2 = dir.norm2();
    const distPixel = globalThis.Math.sqrt(distPixel2);
    dir.inverseScaledCopy(distPixel, dir, Numeric.EPSILON_FLOAT);

    const cosPixel = globalThis.Math.abs(camera.Z.dotProduct(dir));

    const pdfDir = (1.0 / (camera.pixelWidth * camera.pixelHeight)) * (distPixel2 / cosPixel);

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

    newNode.accumulatedRussianRouletteFactors = thisNode.accumulatedRussianRouletteFactors;

    return true;
  }

  public SetPixel(defaultCamera: Camera, nx: number, ny: number, camera: Camera | null): void {
    const useCamera = camera === null ? defaultCamera : camera;

    this.m_px = -useCamera.pixelWidth * useCamera.xSize / 2.0 + nx * useCamera.pixelWidth;
    this.m_py = -useCamera.pixelHeight * useCamera.ySize / 2.0 + ny * useCamera.pixelHeight;
  }

  public override evalPDF(
    camera: Camera,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    flags = Sampler.BSDF_ALL_COMPONENTS,
    pdf: number[] | null = null,
    pdfRR: number[] | null = null
  ): number {
    void flags;

    let dist2: number;
    let dist: number;
    let cosA: number;
    let cosB: number;
    let localPdf: number;
    const outDir = new Vector3D();

    outDir.subtraction(newNode.m_hit.getPoint(), thisNode.m_hit.getPoint());
    dist2 = outDir.norm2();
    dist = globalThis.Math.sqrt(dist2);
    outDir.inverseScaledCopy(dist, outDir, Numeric.EPSILON_FLOAT);

    cosA = thisNode.m_normal.dotProduct(outDir);

    localPdf = 1.0 / (camera.pixelHeight * camera.pixelWidth * cosA * cosA * cosA);

    cosB = -newNode.m_normal.dotProduct(outDir);
    localPdf = localPdf * cosB / dist2;

    if (pdf !== null && pdf.length > 0) {
      pdf[0] = localPdf;
    }
    if (pdfRR !== null && pdfRR.length > 0) {
      pdfRR[0] = 1.0;
    }

    return localPdf;
  }
}
