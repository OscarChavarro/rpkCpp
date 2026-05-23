/**
Random walk generation
*/

import { ArrayList } from "../../../../java/util/ArrayList";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Ray } from "../../common/linealAlgebra/Ray";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { Patch } from "../../environment/geometry/elements/Patch";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { Coefficientsmcrad } from "./Coefficientsmcrad";
import { Localline } from "./Localline";
import { McradP } from "./McradP";
import { Path } from "./Path";
import { Sample4d } from "./Sample4d";
import { StochasticRaytracingPathNode } from "./StochasticRaytracingPathNode";
import { StochasticRelaxation } from "./StochasticRelaxation";

export class Tracepath {
  private static birthProbability: ((patch: Patch) => number) | null = null;
  private static sumProbabilities = 0.0;

  private constructor() {
  }

  /**
Initialises numberOfNodes, nodes allocated to zero and 'nodes' to the nullptr pointer
*/
  private static initPath(path: Path): void {
    path.numberOfNodes = 0;
    path.nodesAllocated = 0;
    path.nodes = null;
  }

  /**
Sets numberOfNodes to zero (forgets old path, but does not free the memory for the nodes
*/
  private static clearPath(path: Path): void {
    path.numberOfNodes = 0;
  }

  /**
Adds a node to the path. Re-allocates more space for the nodes if necessary
*/
  private static pathAddNode(path: Path, patch: Patch, prob: number, inPoint: Vector3D, outpoint: Vector3D): void {
    if (path.numberOfNodes >= path.nodesAllocated) {
      const newNodes = new Array<StochasticRaytracingPathNode>(path.nodesAllocated + 20);
      for (let i = 0; i < newNodes.length; i++) {
        newNodes[i] = new StochasticRaytracingPathNode();
      }
      if (path.nodesAllocated > 0 && path.nodes !== null) {
        for (let i = 0; i < path.numberOfNodes; i++) {
          const dst = newNodes[i]!;
          const src = path.nodes[i]!;
          dst.patch = src.patch;
          dst.probability = src.probability;
          dst.inPoint.copy(src.inPoint);
          dst.outpoint.copy(src.outpoint);
        }
      }
      path.nodes = newNodes;
      path.nodesAllocated += 20;
    }

    const node = path.nodes![path.numberOfNodes]!;
    node.patch = patch;
    node.probability = prob;
    node.inPoint.copy(inPoint);
    node.outpoint.copy(outpoint);
    path.numberOfNodes++;
  }

  /**
Disposes of the memory for storing path nodes
*/
  private static freePathNodes(path: Path): void {
    path.nodes = null;
    path.nodesAllocated = 0;
  }

  /**
Path nodes are filled in 'path', 'path' itself is returned

Traces a random walk originating at 'origin', with birth stochasticJacobiProbability
'globalBirthProbability' (filled in as stochasticJacobiProbability of the origin node: source term
estimation is being suppressed --- survival stochasticJacobiProbability at the origin is
1). Survival stochasticJacobiProbability at other nodes than the origin is calculated by
'survivalProbabilityCallBack()', results are stored in 'path', which should be an
Path, previously initialised by initPath(). If required, photonMapTracePath()
allocates extra space for storing nodes calls to pathAddNode().
freePathNodes() should be called in order to dispose of this memory
when no longer needed
*/
  private static tracePath(
    sceneWorldVoxelGrid: VoxelGrid,
    origin: Patch,
    birthProb: number,
    survivalProbabilityCallBack: (patch: Patch) => number,
    path: Path
  ): Path {
    const inPoint = new Vector3D(0.0, 0.0, 0.0);
    const outpoint = new Vector3D(0.0, 0.0, 0.0);
    let P = origin;
    let survivalProb: number;
    let ray: Ray;
    let hit: RayHit | null;
    const hitStore = new RayHit();

    StochasticRelaxation.activeState().tracedPaths++;
    Tracepath.clearPath(path);
    Tracepath.pathAddNode(path, origin, birthProb, inPoint, outpoint);
    do {
      StochasticRelaxation.activeState().tracedRays++;
      ray = Localline.mcrGenerateLocalLine(P, Sample4d.sample4D((McradP.topLevelStochasticRadiosityElement(P) as any).rayIndex));
      (McradP.topLevelStochasticRadiosityElement(P) as any).rayIndex++;
      if (path.numberOfNodes > 1 && StochasticRelaxation.activeState().continuousRandomWalk !== 0) {
        ray.position.copy(path.nodes![path.numberOfNodes - 1]!.inPoint);
      }
      path.nodes![path.numberOfNodes - 1]!.outpoint.copy(ray.position);

      hit = Localline.mcrShootRay(sceneWorldVoxelGrid, P, ray, hitStore);
      if (hit === null) {
        break;
      }

      P = hit.getPatch()!;
      survivalProb = survivalProbabilityCallBack(P);
      Tracepath.pathAddNode(path, P, survivalProb, hit.getPoint(), outpoint);
    } while (globalThis.Math.random() < survivalProb);

    return path;
  }

  private static patchNormalisedBirthProbability(patch: Patch): number {
    return Tracepath.birthProbability!(patch) / Tracepath.sumProbabilities;
  }

  /**
Traces 'numberOfPaths' paths with given birth probabilities
*/
  public static tracePaths(
    sceneWorldVoxelGrid: VoxelGrid,
    numberOfPaths: number,
    birthProbabilityCallBack: (patch: Patch) => number,
    survivalProbabilityCallBack: (patch: Patch) => number,
    scorePathCallBack: (path: Path, numberOfPaths: number, birthProb: (patch: Patch) => number) => void,
    updateCallBack: (patch: Patch, w: number) => void,
    scenePatches: ArrayList<Patch>
  ): void {
    let rnd: number;
    let pCumulative: number;
    let pathCount: number;
    const path = new Path();

    StochasticRelaxation.activeState().prevTracedRays = StochasticRelaxation.activeState().tracedRays;
    Tracepath.birthProbability = birthProbabilityCallBack;

    Tracepath.sumProbabilities = 0.0;
    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      const patch = scenePatches.get(i)!;
      Tracepath.sumProbabilities += birthProbabilityCallBack(patch);
      Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchReceivedRad(patch), McradP.getTopLevelPatchBasis(patch));
    }
    if (Tracepath.sumProbabilities < Numeric.EPSILON) {
      VsdkLogger.warning("tracePaths", "No sources");
      return;
    }

    Tracepath.initPath(path);
    rnd = globalThis.Math.random();
    pathCount = 0;
    pCumulative = 0.0;
    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      const patch = scenePatches.get(i)!;
      const p = birthProbabilityCallBack(patch) / Tracepath.sumProbabilities;
      const pathsThisPatch =
        globalThis.Math.floor((pCumulative + p) * numberOfPaths + rnd) - pathCount;
      for (let j = 0; j < pathsThisPatch; j++) {
        Tracepath.tracePath(sceneWorldVoxelGrid, patch, p, survivalProbabilityCallBack, path);
        scorePathCallBack(path, numberOfPaths, Tracepath.patchNormalisedBirthProbability);
      }
      pCumulative += p;
      pathCount += pathsThisPatch;
    }

    process.stderr.write("\n");
    Tracepath.freePathNodes(path);

    StochasticRelaxation.activeState().unShotFlux.clear();
    StochasticRelaxation.activeState().unShotYmp = 0.0;
    StochasticRelaxation.activeState().totalFlux.clear();
    StochasticRelaxation.activeState().totalYmp = 0.0;

    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      const patch = scenePatches.get(i)!;
      updateCallBack(patch, numberOfPaths / Tracepath.sumProbabilities);
      StochasticRelaxation.activeState().unShotFlux.addScaled(
        StochasticRelaxation.activeState().unShotFlux,
        globalThis.Math.PI * patch.area,
        McradP.getTopLevelPatchUnShotRad(patch)![0]!
      );
      StochasticRelaxation.activeState().totalFlux.addScaled(
        StochasticRelaxation.activeState().totalFlux,
        globalThis.Math.PI * patch.area,
        McradP.getTopLevelPatchRad(patch)![0]!
      );
    }
  }
}
