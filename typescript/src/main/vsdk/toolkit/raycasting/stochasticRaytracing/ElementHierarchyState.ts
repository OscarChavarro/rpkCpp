import { ArrayList } from "../../../../java/util/ArrayList";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { RenderOptions } from "../../common/RenderOptions";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Vertex } from "../../skin/Vertex";
import { HierarchyClusteringMode } from "./HierarchyClusteringMode";
import { Link } from "./Link";

export type REFINE_ACTION = (
  link: Link,
  rcvtop: any,
  ur: number[],
  vr: number[],
  srctop: any,
  us: number[],
  vs: number[],
  renderOptions: RenderOptions
) => Link;

export type ORACLE = (link: Link) => REFINE_ACTION;

/**
Global parameters controlling hierarchical refinement
*/
export class ElementHierarchyState {
  public epsilon: number;
  public do_h_meshing: number;
  public minimumArea: number;
  public nr_elements: number;
  public nr_clusters: number;
  public tvertex_elimination: number;
  public clustering: HierarchyClusteringMode | null;
  public oracle: ORACLE | null;
  public topCluster: any;
  public coords: ArrayList<Vector3D> | null;
  public normals: ArrayList<Vector3D> | null;
  public texCoords: ArrayList<Vector3D> | null;
  public vertices: ArrayList<Vertex> | null;

  public constructor() {
    this.epsilon = 0.0;
    this.do_h_meshing = 0;
    this.minimumArea = 0.0;
    this.nr_elements = 0;
    this.nr_clusters = 0;
    this.tvertex_elimination = 0;
    this.clustering = null;
    this.oracle = null;
    this.topCluster = null;
    this.coords = null;
    this.normals = null;
    this.texCoords = null;
    this.vertices = null;
  }

  public static setActiveState(state: ElementHierarchyState): void {
    ElementHierarchyState.activeStatePtr = state;
  }

  public static activeState(): ElementHierarchyState {
    if (ElementHierarchyState.activeStatePtr === null) {
      VsdkLogger.fatal(-1, "ElementHierarchyState::activeState", "Element hierarchy state was not initialized");
    }
    return ElementHierarchyState.activeStatePtr!;
  }

  private static activeStatePtr: ElementHierarchyState | null = null;
}
