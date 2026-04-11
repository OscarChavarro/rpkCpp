/**
Photon kd-tree : specialized kd-tree with some photon map specific additions
*/

import { KDTree, NodeCallback } from "../../common/dataStructures/KDTree";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { IrrPhoton } from "./IrrPhoton";
import { NormalQuery } from "./NormalQuery";
import { Photon } from "./Photon";

class NodeData {
  public photon: Photon;
  public flags: number;
  public point: number[];

  public constructor(photon: Photon, flags: number) {
    this.photon = photon;
    this.flags = flags;
    const p = photon.pos();
    this.point = [p.x, p.y, p.z];
  }
}

class QueryCandidate {
  public photon: Photon;
  public distance: number;

  public constructor(photon: Photon, distance: number) {
    this.photon = photon;
    this.distance = distance;
  }
}

export class PhotonKDTree extends KDTree {
  private static qdat_s = new NormalQuery();
  private readonly nodes: NodeData[];

  private static sqrDistance3DPhoton(a: number[], b: number[]): number {
    let tmp = a[0] - b[0];
    let result = tmp * tmp;

    tmp = a[1] - b[1];
    result += tmp * tmp;

    tmp = a[2] - b[2];
    result += tmp * tmp;

    return result;
  }

  public constructor(dataSize: number, copyData: boolean) {
    super(dataSize, copyData);
    this.nodes = [];
  }

  public override addPoint(data: unknown, flags = 0): void {
    if (!(data instanceof Photon)) {
      throw new globalThis.Error("PhotonKDTree only accepts Photon data");
    }
    this.nodes.push(new NodeData(data, flags));
  }

  public override query(point: number[], n: number, results: unknown[]): number;
  public override query(
    point: number[],
    n: number,
    results: unknown[],
    inDistances: number[] | null,
    radius: number,
    excludeFlags: number
  ): number;
  public override query(
    point: number[],
    n: number,
    results: unknown[],
    inDistances: number[] | null = null,
    radius = KDTree.KD_MAX_RADIUS,
    excludeFlags = 0
  ): number {
    const candidates: QueryCandidate[] = [];

    for (let i = 0; i < this.nodes.length; i++) {
      const node = this.nodes[i];
      if ((node.flags & excludeFlags) !== 0) {
        continue;
      }
      const dist = PhotonKDTree.sqrDistance3DPhoton(node.point, point);
      if (dist < radius) {
        candidates.push(new QueryCandidate(node.photon, dist));
      }
    }

    candidates.sort((a, b) => a.distance - b.distance);

    const found = globalThis.Math.min(n, candidates.length);
    if (found <= 0) {
      return 0;
    }

    const usedDistances = inDistances === null ? new Array<number>(globalThis.Math.max(1, found)).fill(0.0) : inDistances;

    const farthest = candidates[found - 1];
    results[0] = farthest.photon;
    usedDistances[0] = farthest.distance;

    let outIndex = 1;
    for (let i = 0; i < found - 1; i++) {
      results[outIndex] = candidates[i].photon;
      usedDistances[outIndex] = candidates[i].distance;
      outIndex++;
    }

    return found;
  }

  public override iterateNodes(callback: NodeCallback, data: unknown): void {
    for (let i = 0; i < this.nodes.length; i++) {
      callback.call(data, this.nodes[i].photon);
    }
  }

  public override balance(): void {
    // This Java migration keeps a linear container. No explicit balancing needed.
  }

  public normalPhotonQuery(
    position: Vector3D,
    normal: Vector3D,
    threshold: number,
    maxR2: number
  ): IrrPhoton | null {
    PhotonKDTree.qdat_s.photon = null;
    PhotonKDTree.qdat_s.normal = new Vector3D(normal.x, normal.y, normal.z);
    PhotonKDTree.qdat_s.point = [position.x, position.y, position.z];
    PhotonKDTree.qdat_s.threshold = threshold;
    PhotonKDTree.qdat_s.maximumDistance = maxR2;

    for (let i = 0; i < this.nodes.length; i++) {
      const node = this.nodes[i];
      if (!(node.photon instanceof IrrPhoton)) {
        continue;
      }

      const dist = PhotonKDTree.sqrDistance3DPhoton(node.point, PhotonKDTree.qdat_s.point as number[]);
      const photon = node.photon;
      if (dist < PhotonKDTree.qdat_s.maximumDistance
        && photon.Normal().dotProduct(PhotonKDTree.qdat_s.normal) > PhotonKDTree.qdat_s.threshold) {
        PhotonKDTree.qdat_s.maximumDistance = dist;
        PhotonKDTree.qdat_s.photon = photon;
      }
    }

    return PhotonKDTree.qdat_s.photon;
  }

  // Java parity helper name; instance scope avoids static private-name clash with KDTree.
  private sqrDistance3D(a: number[], b: number[]): number {
    return PhotonKDTree.sqrDistance3DPhoton(a, b);
  }
}
