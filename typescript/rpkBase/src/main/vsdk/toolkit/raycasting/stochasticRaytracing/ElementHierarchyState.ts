import { ArrayList } from "vitral/dist/java/util/ArrayList";
import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { Vertex } from "../../environment/geometry/elements/Vertex";
import { HierarchyClusteringMode } from "./HierarchyClusteringMode";
import { Link } from "./Link";
import type { RadianceElementTable } from "../../scene/RadianceElementTable";

export type REFINE_ACTION = (
  link: Link,
  rcvtop: any,
  ur: number[],
  vr: number[],
  srctop: any,
  us: number[],
  vs: number[],
  renderOptions: RendererConfiguration
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
  public coords: ArrayList<Vector3Df> | null;
  public normals: ArrayList<Vector3Df> | null;
  public texCoords: ArrayList<Vector3Df> | null;
  public vertices: ArrayList<Vertex> | null;
  // The scene's table of top-level elements, bound when the method is initialized;
  // null while the method has not been initialized (nothing to terminate then)
  private radianceElements: RadianceElementTable | null;

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
    this.radianceElements = null;
  }

  public getRadianceElements(): RadianceElementTable | null {
    return this.radianceElements;
  }

  public setRadianceElements(inRadianceElements: RadianceElementTable | null): void {
    this.radianceElements = inRadianceElements;
  }

  public static setActiveState(state: ElementHierarchyState): void {
    ElementHierarchyState.activeStatePtr = state;
  }

  public static activeState(): ElementHierarchyState {
    if (ElementHierarchyState.activeStatePtr === null) {
      VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "ElementHierarchyState::activeState", "Element hierarchy state was not initialized");
    }
    return ElementHierarchyState.activeStatePtr!;
  }

  private static activeStatePtr: ElementHierarchyState | null = null;
}
