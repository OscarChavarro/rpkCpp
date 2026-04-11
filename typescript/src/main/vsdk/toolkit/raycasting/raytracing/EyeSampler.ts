import { Error as VsdkError } from "../../common/Error";
import { RayHitFlag } from "../../material/RayHitFlag";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { Sampler } from "./Sampler";

export class EyeSampler extends Sampler {
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
    void sceneVoxelGrid;
    void sceneBackground;
    void x1;
    void x2;
    void doRR;
    void flags;

    if (prevNode !== null || thisNode !== null) {
      VsdkError.warning("EyeSampler::sample", "Not first node in path ?!");
    }

    newNode.m_depth = 0;
    newNode.m_rayType = PathRayType.STOPS;

    const hit = newNode.m_hit;

    hit.init(null, camera.eyePosition, camera.Z, null);
    hit.setNormal(camera.Z);
    const newFlags = hit.getFlags() | RayHitFlag.NORMAL | RayHitFlag.SHADING_FRAME;
    hit.setFlags(newFlags);
    hit.setShadingFrame(camera.X, camera.Y, camera.Z);

    newNode.m_normal.copy(newNode.m_hit.getNormal());
    newNode.m_G = 1.0;

    newNode.m_pdfFromPrev = 1.0;

    newNode.m_pdfFromNext = 0.0;

    newNode.m_useBsdf = null;
    newNode.m_inBsdf = null;
    newNode.m_outBsdf = null;

    newNode.m_accUsedComponents = 0;

    newNode.accumulatedRussianRouletteFactors = 1.0;

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
    void thisNode;
    void newNode;
    void flags;
    void probabilityDensityFunction;
    void probabilityDensityFunctionRR;
    return 1.0;
  }
}
