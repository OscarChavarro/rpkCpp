import { KDTree } from "vitral/dist/vsdk/toolkit/common/dataStructures/KDTree";
import type { KDTreeAdapter } from "vitral/dist/vsdk/toolkit/common/dataStructures/KDTreeAdapter";
import type { KDTreeVisitor } from "vitral/dist/vsdk/toolkit/common/dataStructures/KDTreeVisitor";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { IrrPhoton } from "./IrrPhoton";
import { Photon } from "./Photon";

class PhotonAdapter implements KDTreeAdapter<Photon> {
  public getKDPosition(value: Photon): Vector3Dd { return value.pos(); }
  public copy(value: Photon): Photon { return new Photon(value.pos(), value.power(), value.dir()); }
}
class IrrPhotonAdapter implements KDTreeAdapter<IrrPhoton> {
  public getKDPosition(value: IrrPhoton): Vector3Dd { return value.pos(); }
  public copy(value: IrrPhoton): IrrPhoton {
    const result = new IrrPhoton();
    result.copy(value);
    result.setNormal(value.Normal());
    result.SetIrradiance(value.m_irradiance);
    return result;
  }
}
class NormalSearch {
  public photon: IrrPhoton | null = null;
  public constructor(public readonly point: Vector3Dd, public readonly normal: Vector3Dd,
                     public readonly threshold: number, public maximumDistance: number) {}
}
export class PhotonKDTree {
  private readonly photons: KDTree<Photon> | null;
  private readonly irradiancePhotons: KDTree<IrrPhoton> | null;
  private readonly irradianceResults: IrrPhoton[] = [];

  public constructor(irradiance: boolean) {
    this.photons = irradiance ? null : new KDTree(new PhotonAdapter());
    this.irradiancePhotons = irradiance ? new KDTree(new IrrPhotonAdapter()) : null;
  }
  public addPhoton(photon: Photon, flags: number): void { this.photons!.addPoint(photon, flags); }
  public addIrradiancePhoton(photon: IrrPhoton, flags: number): void { this.irradiancePhotons!.addPoint(photon, flags); }
  public balance(): void {
    if (this.irradiancePhotons !== null) this.irradiancePhotons.balance();
    else this.photons!.balance();
  }
  public query(position: Vector3Dd, count: number, results: Photon[], distances: number[] | null,
               radius: number, excluded = 0): number {
    if (this.irradiancePhotons === null) return this.photons!.query(position, count, results, distances, radius, excluded);
    const found = this.irradiancePhotons.query(position, count, this.irradianceResults, distances, radius, excluded);
    for (let i = 0; i < found; ++i) results[i] = this.irradianceResults[i]!;
    return found;
  }
  public iterateIrradiance(visitor: KDTreeVisitor<IrrPhoton>): void { this.irradiancePhotons!.iterateNodes(visitor); }
  public normalPhotonQuery(position: Vector3Dd, normal: Vector3Dd, threshold: number, maxR2: number): IrrPhoton | null {
    const tree = this.irradiancePhotons;
    if (tree === null || tree.getBalancedCount() === 0 || tree.getUnbalancedCount() !== 0) return null;
    const query = new NormalSearch(position, normal, threshold, maxR2);
    this.normalQueryRec(0, query);
    return query.photon;
  }
  private normalQueryRec(index: number, query: NormalSearch): void {
    const tree = this.irradiancePhotons!;
    const node = tree.getBalancedNode(index);
    const position = node.data.pos();
    if (index < tree.getFirstLeaf()) {
      const axis = node.discriminator();
      let d = position.component(axis) - query.point.component(axis);
      let near = (index << 1) + 1;
      let far = near + 1;
      if (d < 0) { const temp = near; near = far; far = temp; }
      if (near < tree.getBalancedCount()) this.normalQueryRec(near, query);
      d = d * d;
      if (far < tree.getBalancedCount() && d < query.maximumDistance) this.normalQueryRec(far, query);
    }
    let d = position.x() - query.point.x();
    let distance = d * d;
    d = position.y() - query.point.y(); distance = distance + (d * d);
    d = position.z() - query.point.z(); distance = distance + (d * d);
    if (distance < query.maximumDistance && node.data.Normal().dotProduct(query.normal) > query.threshold) {
      query.maximumDistance = distance;
      query.photon = node.data;
    }
  }
}
