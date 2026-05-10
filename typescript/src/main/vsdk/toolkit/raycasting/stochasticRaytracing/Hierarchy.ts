/**
Hierarchical refinement stuff (includes Jan's elementP.h)
*/

import { ArrayList } from "../../../../java/util/ArrayList";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { RenderOptions } from "../../common/RenderOptions";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Statistics } from "../../common/statistics/Statistics";
import { Geometry } from "../../skin/Geometry";
import { Patch } from "../../skin/Patch";
import { Vertex } from "../../skin/Vertex";
import { ElementHierarchyState, ORACLE, REFINE_ACTION } from "./ElementHierarchyState";
import { HierarchyClusteringMode } from "./HierarchyClusteringMode";
import { Link } from "./Link";
import { McradP } from "./McradP";
import { StochasticRadiosityElement } from "./StochasticRadiosityElement";
import { StochasticRelaxation } from "./StochasticRelaxation";
import { StochasticRaytracingMethod } from "./StochasticRaytracingMethod";

export class Hierarchy {
  private static readonly DEFAULT_EH_EPSILON = 5e-4;
  private static readonly DEFAULT_EH_MINIMUM_AREA = 1e-6;
  private static readonly DEFAULT_EH_HIERARCHICAL_MESHING = true;
  private static readonly DEFAULT_EH_T_VERTEX_ELIMINATION = true;
  private static readonly DEFAULT_EH_CLUSTERING = HierarchyClusteringMode.ORIENTED_CLUSTERING;
  private static readonly DONT_REFINE_ACTION: REFINE_ACTION = Hierarchy.dontRefineCallBack;

  private constructor() {
  }

  public static elementHierarchyDefaults(): void {
    ElementHierarchyState.activeState().epsilon = Hierarchy.DEFAULT_EH_EPSILON;
    ElementHierarchyState.activeState().minimumArea = Hierarchy.DEFAULT_EH_MINIMUM_AREA;
    ElementHierarchyState.activeState().do_h_meshing = Hierarchy.DEFAULT_EH_HIERARCHICAL_MESHING ? 1 : 0;
    ElementHierarchyState.activeState().clustering = Hierarchy.DEFAULT_EH_CLUSTERING;
    ElementHierarchyState.activeState().tvertex_elimination = Hierarchy.DEFAULT_EH_T_VERTEX_ELIMINATION ? 1 : 0;
    ElementHierarchyState.activeState().oracle = Hierarchy.powerOracle;
    ElementHierarchyState.activeState().nr_elements = 0;
    ElementHierarchyState.activeState().nr_clusters = 0;
  }

  public static elementHierarchyInit(clusteredWorldGeometry: Geometry): void {
    ElementHierarchyState.activeState().coords = new ArrayList<Vector3D>();
    ElementHierarchyState.activeState().normals = new ArrayList<Vector3D>();
    ElementHierarchyState.activeState().texCoords = new ArrayList<Vector3D>();
    ElementHierarchyState.activeState().vertices = new ArrayList<Vertex>();
    ElementHierarchyState.activeState().topCluster =
      StochasticRadiosityElement.stochasticRadiosityElementCreateFromGeometry(clusteredWorldGeometry);
  }

  public static elementHierarchyTerminate(scenePatches: ArrayList<Patch>): void {
    StochasticRadiosityElement.stochasticRadiosityElementDestroyClusterHierarchy(ElementHierarchyState.activeState().topCluster);
    ElementHierarchyState.activeState().topCluster = null;

    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      const patch = scenePatches.get(i);
      StochasticRadiosityElement.stochasticRadiosityElementDestroy(
        McradP.topLevelStochasticRadiosityElement(patch) as StochasticRadiosityElement
      );
      patch.radianceData = null;
    }

    const vertices = ElementHierarchyState.activeState().vertices;
    if (vertices !== null) {
      for (let i = 0; i < vertices.size(); i++) {
        vertices.set(i, null as unknown as Vertex);
      }
      vertices.clear();
    }
    ElementHierarchyState.activeState().vertices = null;

    if (ElementHierarchyState.activeState().coords !== null) {
      ElementHierarchyState.activeState().coords!.clear();
      ElementHierarchyState.activeState().coords = null;
    }

    if (ElementHierarchyState.activeState().normals !== null) {
      ElementHierarchyState.activeState().normals!.clear();
      ElementHierarchyState.activeState().normals = null;
    }

    if (ElementHierarchyState.activeState().texCoords !== null) {
      ElementHierarchyState.activeState().texCoords!.clear();
      ElementHierarchyState.activeState().texCoords = null;
    }
  }

  private static dontRefineCallBack(
    link: Link,
    rcvtop: StochasticRadiosityElement,
    ur: number[],
    vr: number[],
    srctop: StochasticRadiosityElement,
    us: number[],
    vs: number[],
    renderOptions: RenderOptions
  ): Link {
    void rcvtop;
    void ur;
    void vr;
    void srctop;
    void us;
    void vs;
    void renderOptions;

    return link;
  }

  private static subdivideReceiverCallBack(
    link: Link,
    rcvtop: StochasticRadiosityElement,
    ur: number[],
    vr: number[],
    srctop: StochasticRadiosityElement,
    us: number[],
    vs: number[],
    renderOptions: RenderOptions
  ): Link {
    void srctop;
    void us;
    void vs;

    let rcv = link.rcv as StochasticRadiosityElement;
    if (rcv.isCluster()) {
      rcv = rcv.childContainingElement(rcvtop) as StochasticRadiosityElement;
    }
    else {
      if (rcv.regularSubElements === null) {
        StochasticRadiosityElement.stochasticRadiosityElementRegularSubdivideElement(rcv, renderOptions);
      }
      rcv = StochasticRadiosityElement.stochasticRadiosityElementRegularSubElementAtPoint(rcv, ur, vr)!;
    }
    link.rcv = rcv;
    return link;
  }

  private static subdivideSourceCallBack(
    link: Link,
    rcvtop: StochasticRadiosityElement,
    ur: number[],
    vr: number[],
    srcTop: StochasticRadiosityElement,
    us: number[],
    vs: number[],
    renderOptions: RenderOptions
  ): Link {
    void rcvtop;
    void ur;
    void vr;

    let src = link.src as StochasticRadiosityElement;
    if (src.isCluster()) {
      src = src.childContainingElement(srcTop) as StochasticRadiosityElement;
    }
    else {
      if (src.regularSubElements === null) {
        StochasticRadiosityElement.stochasticRadiosityElementRegularSubdivideElement(src, renderOptions);
      }
      src = StochasticRadiosityElement.stochasticRadiosityElementRegularSubElementAtPoint(src, us, vs)!;
    }
    link.src = src;
    return link;
  }

  public static selfLink(link: Link): boolean {
    return link.rcv === link.src;
  }

  public static formFactorEstimate(rcv: StochasticRadiosityElement, src: StochasticRadiosityElement): number {
    const D = new Vector3D();
    D.subtraction(src.midPoint, rcv.midPoint);

    const d = D.norm();
    const f = src.area / (globalThis.Math.PI * d * d + src.area);
    const f2 = 2.0 * f;
    let c1 = rcv.isCluster() ? 1.0 : globalThis.Math.abs(D.dotProduct(rcv.patch!.normal)) / d;
    if (c1 < f2) {
      c1 = f2;
    }
    let c2 = src.isCluster() ? 1.0 : globalThis.Math.abs(D.dotProduct(src.patch!.normal)) / d;
    if (c2 < f2) {
      c2 = f2;
    }
    return f * c1 * c2;
  }

  public static lowPowerLink(link: Link, statistics: Statistics): boolean {
    const rcv = link.rcv as StochasticRadiosityElement;
    const src = link.src as StochasticRadiosityElement;
    const rhoSrcRad = new ColorRgb();
    const ff = Hierarchy.formFactorEstimate(rcv, src);
    let threshold: number;
    let propagatedPower: number;

    rhoSrcRad.scaledCopy(globalThis.Math.PI, src.radiance![0]);
    if (!rcv.isCluster()) {
      const rd = (McradP.topLevelStochasticRadiosityElement(rcv.patch!) as StochasticRadiosityElement).Rd;
      rhoSrcRad.selfScalarProduct(rd);
    }

    threshold = ElementHierarchyState.activeState().epsilon * statistics.radiance.maxSelfEmittedPower.maximumComponent();
    propagatedPower = rcv.area * ff * rhoSrcRad.maximumComponent();
    if (StochasticRelaxation.activeState().importanceDriven !== 0) {
      propagatedPower *= rcv.importance;
      if (!rcv.isCluster()) {
        propagatedPower *= StochasticRadiosityElement.stochasticRadiosityElementScalarReflectance(rcv);
      }
    }

    return propagatedPower < threshold;
  }

  public static subDivideLargest(link: Link): REFINE_ACTION {
    const rcv = link.rcv as StochasticRadiosityElement;
    const src = link.src as StochasticRadiosityElement;
    if (rcv.area < ElementHierarchyState.activeState().minimumArea && src.area < ElementHierarchyState.activeState().minimumArea) {
      return Hierarchy.DONT_REFINE_ACTION;
    }
    return (rcv.area > src.area) ? Hierarchy.subdivideReceiverCallBack : Hierarchy.subdivideSourceCallBack;
  }

  /**
Well known power-based refinement oracle ([HANR1992] Hanrahan'91, with importance
a la [SMIT1992] Smits'92 when importance-driven sampling is enabled)
*/
  public static powerOracle(link: Link): REFINE_ACTION {
    if (Hierarchy.selfLink(link)) {
      return Hierarchy.subdivideReceiverCallBack;
    }
    else if (Hierarchy.lowPowerLink(link, Statistics.instance())) {
      return Hierarchy.DONT_REFINE_ACTION;
    }
    else {
      return Hierarchy.subDivideLargest(link);
    }
  }

  /**
Constructs a toplevel link for given toplevel surface elements
rcvTop and srcTop: the result is a link between the toplevel
cluster containing the whole scene and itself if clustering is
enabled. If clustering is not enabled, a link between the
given toplevel surface elements is returned
*/
  public static topLink(rcvTop: StochasticRadiosityElement, srcTop: StochasticRadiosityElement): Link {
    let rcv: StochasticRadiosityElement;
    let src: StochasticRadiosityElement;
    const link = new Link();

    if (
      ElementHierarchyState.activeState().do_h_meshing !== 0
      && ElementHierarchyState.activeState().clustering !== HierarchyClusteringMode.NO_CLUSTERING
    ) {
      src = ElementHierarchyState.activeState().topCluster as StochasticRadiosityElement;
      rcv = ElementHierarchyState.activeState().topCluster as StochasticRadiosityElement;
    }
    else {
      src = srcTop;
      rcv = rcvTop;
    }

    link.rcv = rcv;
    link.src = src;

    return link;
  }

  /**
Refines a toplevel link (constructed with TopLink() above). The
returned Link structure contains pointers the admissible
elements and corresponding point coordinates for light transport.
rcvTop and srcTop are toplevel surface elements containing the
endpoint and origin respectively of a line along which light is to
be transported. (ur,vr) and (us,vs) are the uniform parameters of
the endpoint and origin on the toplevel surface elements on input.
They will be replaced by the point parameters on the admissible elements
after refinement
(ur,vr) are the coordinates of the point on the receiver patch,
(us,vs) coordinates of the point on the source patch
*/
  public static hierarchyRefine(
    link: Link,
    rcvTop: StochasticRadiosityElement,
    ur: number[],
    vr: number[],
    srcTop: StochasticRadiosityElement,
    us: number[],
    vs: number[],
    evaluateLink: ORACLE,
    renderOptions: RenderOptions
  ): Link {
    if (ElementHierarchyState.activeState().do_h_meshing === 0) {
      link.rcv = rcvTop;
      link.src = srcTop;
    }
    else {
      let action: REFINE_ACTION;
      while ((action = evaluateLink(link)) !== Hierarchy.DONT_REFINE_ACTION) {
        link = action(link, rcvTop, ur, vr, srcTop, us, vs, renderOptions);
      }
    }
    return link;
  }
}
