import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { SurfaceSampler } from "./SurfaceSampler";

export class SpecularSampler extends SurfaceSampler {
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
    flags = 0
  ): boolean {
    void camera;
    void sceneVoxelGrid;
    void sceneBackground;
    void prevNode;
    void thisNode;
    void newNode;
    void x1;
    void x2;
    void doRR;
    void flags;
    return false;
  }

  public override evalPDF(
    camera: Camera,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    flags = 0,
    probabilityDensityFunction: number[] | null = null,
    probabilityDensityFunctionRR: number[] | null = null
  ): number {
    void camera;
    void thisNode;
    void newNode;
    void flags;
    if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
      probabilityDensityFunction[0] = 0.0;
    }
    if (probabilityDensityFunctionRR !== null && probabilityDensityFunctionRR.length > 0) {
      probabilityDensityFunctionRR[0] = 0.0;
    }
    return 0.0;
  }

  public override EvalPDFPrev(
    prevNode: SimpleRaytracingPathNode,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    flags = 0,
    probabilityDensityFunction: number[] | null = null,
    probabilityDensityFunctionRR: number[] | null = null
  ): number {
    void prevNode;
    void thisNode;
    void newNode;
    void flags;
    if (probabilityDensityFunction !== null && probabilityDensityFunction.length > 0) {
      probabilityDensityFunction[0] = 0.0;
    }
    if (probabilityDensityFunctionRR !== null && probabilityDensityFunctionRR.length > 0) {
      probabilityDensityFunctionRR[0] = 0.0;
    }
    return 0.0;
  }
}
