/**
Monte Carlo radiosity element type
*/

import { ArrayList } from "../../../../java/util/ArrayList";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { BsdfComponent } from "../../material/BsdfComponent";
import { PhongEmittanceDistributionFunction } from "../../material/PhongEmittanceDistributionFunction";
import { RayHitFlag } from "../../environment/geometry/elements/RayHitFlag";
import { XxdfComponentFlag } from "../../material/XxdfComponentFlag";
import { PatchVisitor } from "../../numericalAnalysis/PatchVisitor";
import { Niederreiter } from "../../numericalAnalysis/quasiMonteCarlo/Niederreiter";
import { Geometry } from "../../skin/Geometry";
import { Element } from "../../environment/geometry/elements/Element";
import { ElementFlags } from "../../environment/geometry/elements/ElementFlags";
import { ElementTypes } from "../../environment/geometry/elements/ElementTypes";
import { Patch } from "../../environment/geometry/elements/Patch";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { Vertex } from "../../environment/geometry/elements/Vertex";
import { ToneMap } from "../../tonemap/ToneMap";
import { Basismcrad } from "./Basismcrad";
import { Coefficientsmcrad } from "./Coefficientsmcrad";
import { ElementHierarchyState } from "./ElementHierarchyState";
import { GalerkinBasis } from "./GalerkinBasis";
import { StochasticRadiosityBasisState } from "./StochasticRadiosityBasisState";
import { StochasticRelaxation } from "./StochasticRelaxation";
import { WhatToShow } from "./WhatToShow";

export class StochasticRadiosityElement extends Element {
  public patch: Patch | null;
  public geometry: Geometry | null;
  public rayIndex: number;
  public quality: number;
  public samplingProbability: number;
  public ng: number;

  public basis: GalerkinBasis | null;
  public sourceRad: ColorRgb;

  public importance: number;
  public unShotImportance: number;
  public receivedImportance: number;
  public sourceImportance: number;
  public importanceRayIndex: number;

  public midPoint: Vector3D;
  public vertices: Array<Vertex | null>;
  public childNumber: number;
  public numberOfVertices: number;

  private static coefficientPoolsInitialized = 0;
  private static nextId = 1;

  public constructor() {
    super();
    this.patch = null;
    this.geometry = null;
    this.rayIndex = 0;
    this.quality = 0.0;
    this.samplingProbability = 0.0;
    this.ng = 0.0;
    this.basis = null;
    this.sourceRad = new ColorRgb();
    this.importance = 0.0;
    this.unShotImportance = 0.0;
    this.receivedImportance = 0.0;
    this.sourceImportance = 0.0;
    this.importanceRayIndex = 0;
    this.midPoint = new Vector3D();
    this.vertices = [null, null, null, null];
    this.childNumber = -1;
    this.numberOfVertices = 0;
    this.className = ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY;
  }

  public destroy(): void {
  }

  public static coefficientPoolsAreInitialized(): boolean {
    return StochasticRadiosityElement.coefficientPoolsInitialized !== 0;
  }

  public static markCoefficientPoolsInitialized(): void {
    StochasticRadiosityElement.coefficientPoolsInitialized = 1;
  }

  private static vertexAttachElement(v: Vertex, elem: StochasticRadiosityElement): void {
    elem.className = ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY;
    if (v.radianceData === null) {
      v.radianceData = [];
    }
    v.radianceData.push(elem);
  }

  private static createElement(): StochasticRadiosityElement {
    const elem = new StochasticRadiosityElement();

    elem.patch = null;
    elem.geometry = null;
    elem.id = StochasticRadiosityElement.nextId;
    StochasticRadiosityElement.nextId++;
    elem.area = 0.0;
    Coefficientsmcrad.initCoefficients(elem);

    elem.Ed.clear();
    elem.Rd.clear();

    elem.rayIndex = 0;
    elem.quality = 0;
    elem.ng = 0.0;

    elem.importance = 0.0;
    elem.unShotImportance = 0.0;
    elem.sourceImportance = 0.0;
    elem.importanceRayIndex = 0;

    elem.midPoint.set(0.0, 0.0, 0.0);
    elem.vertices[0] = null;
    elem.vertices[1] = null;
    elem.vertices[2] = null;
    elem.vertices[3] = null;
    elem.parent = null;
    elem.regularSubElements = null;
    elem.irregularSubElements = null;
    elem.transformToParent = null;
    elem.childNumber = -1;
    elem.numberOfVertices = 0;
    elem.flags = 0x00;

    ElementHierarchyState.activeState().nr_elements++;

    return elem;
  }

  public static stochasticRadiosityElementCreateFromPatch(patch: Patch): StochasticRadiosityElement {
    const elem = StochasticRadiosityElement.createElement();
    elem.patch = patch;
    elem.flags = 0x00;
    elem.area = patch.area;
    elem.midPoint.copy(patch.midPoint);
    elem.numberOfVertices = patch.numberOfVertices;
    for (let i = 0; i < elem.numberOfVertices; i++) {
      elem.vertices[i] = patch.vertex[i];
      StochasticRadiosityElement.vertexAttachElement(elem.vertices[i]!, elem);
    }

    Coefficientsmcrad.allocCoefficients(elem);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.radiance, elem.basis);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);

    elem.Ed = PatchVisitor.averageEmittance(patch, XxdfComponentFlag.DIFFUSE_COMPONENT);
    elem.Ed.scaleInverse(globalThis.Math.PI, elem.Ed);
    elem.Rd = PatchVisitor.averageNormalAlbedo(patch, BsdfComponent.BRDF_DIFFUSE_COMPONENT);

    return elem;
  }

  private static monteCarloRadiosityCreateCluster(geometry: Geometry): StochasticRadiosityElement {
    const elem = StochasticRadiosityElement.createElement();

    elem.geometry = geometry;
    elem.flags = ElementFlags.IS_CLUSTER_MASK;

    elem.Rd.setMonochrome(1.0);
    elem.Ed.clear();

    elem.midPoint = geometry.boundingBox.center();

    Coefficientsmcrad.allocCoefficients(elem);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.radiance, elem.basis);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
    elem.importance = 0.0;
    elem.unShotImportance = 0.0;
    elem.receivedImportance = 0.0;

    ElementHierarchyState.activeState().nr_clusters++;

    return elem;
  }

  private static monteCarloRadiosityCreateSurfaceElementChild(patch: Patch, parent: StochasticRadiosityElement): void {
    const elem = patch.radianceData as StochasticRadiosityElement;
    elem.parent = parent;

    elem.className = ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY;
    if (parent.irregularSubElements === null) {
      parent.irregularSubElements = [];
    }
    parent.irregularSubElements.push(elem);
  }

  private static monteCarloRadiosityCreateClusterChild(geometry: Geometry, parent: StochasticRadiosityElement): void {
    const subCluster = StochasticRadiosityElement.monteCarloRadiosityCreateClusterHierarchyRecursive(geometry);
    subCluster.parent = parent;
    subCluster.className = ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY;
    if (parent.irregularSubElements === null) {
      parent.irregularSubElements = [];
    }
    parent.irregularSubElements.push(subCluster);
  }

  private static monteCarloRadiosityInitClusterPull(parent: StochasticRadiosityElement, child: StochasticRadiosityElement): void {
    parent.area += child.area;
    StochasticRadiosityElement.stochasticRadiosityElementPullRadiance(parent, child, parent.radiance!, child.radiance!);
    StochasticRadiosityElement.stochasticRadiosityElementPullRadiance(parent, child, parent.unShotRadiance!, child.unShotRadiance!);
    StochasticRadiosityElement.stochasticRadiosityElementPullRadiance(parent, child, parent.receivedRadiance!, child.receivedRadiance!);
    parent.importance += child.area / parent.area * child.importance;
    parent.unShotImportance += child.area / parent.area * child.unShotImportance;
    parent.receivedImportance += child.area / parent.area * child.receivedImportance;

    parent.Ed.addScaled(parent.Ed, child.area, child.Ed);
  }

  private static monteCarloRadiosityCreateClusterChildren(parent: StochasticRadiosityElement): void {
    const geometry = parent.geometry!;

    if (geometry.isCompound()) {
      const geometryList = Geometry.primitiveListCopy(geometry);
      for (let i = 0; geometryList !== null && i < geometryList.length; i++) {
        StochasticRadiosityElement.monteCarloRadiosityCreateClusterChild(geometryList[i], parent);
      }
    }
    else {
      const patchList = Geometry.patchListReference(geometry);
      for (let i = 0; patchList !== null && i < patchList.length; i++) {
        StochasticRadiosityElement.monteCarloRadiosityCreateSurfaceElementChild(patchList[i], parent);
      }
    }

    for (let i = 0; parent.irregularSubElements !== null && i < parent.irregularSubElements.length; i++) {
      StochasticRadiosityElement.monteCarloRadiosityInitClusterPull(parent, parent.irregularSubElements[i] as StochasticRadiosityElement);
    }
    parent.Ed.scaleInverse(parent.area, parent.Ed);
  }

  private static monteCarloRadiosityCreateClusterHierarchyRecursive(world: Geometry): StochasticRadiosityElement {
    const topCluster = StochasticRadiosityElement.monteCarloRadiosityCreateCluster(world);
    world.radianceData = topCluster;
    StochasticRadiosityElement.monteCarloRadiosityCreateClusterChildren(topCluster);
    return topCluster;
  }

  public static stochasticRadiosityElementCreateFromGeometry(world: Geometry | null): StochasticRadiosityElement | null {
    if (world === null) {
      return null;
    }
    return StochasticRadiosityElement.monteCarloRadiosityCreateClusterHierarchyRecursive(world);
  }

  /**
Determine the (u, v) coordinate range of the element w.r.t. the patch to
which it belongs when using regular quadtree subdivision in
order to efficiently generate samples with Niederreiter::NextNiedInRange()
in the Niederreiter core implementation. Niederreiter::NextNiedInRange() creates a sample on a quadrilateral
subdomain, called a "dyadic box" in QMC literature. All samples in
such a dyadic box have the same most significant bits. This routine
basically computes what these most significant bits are. The computation
is based on the regular quadtree subdivision of a quadrilateral, as
shown above. If a triangular element is to be sampled, the quadrilateral
sample needs to be "folded" into the triangle. FoldSample() in sample4d.c
does this
*/
  public static stochasticRadiosityElementRange(
    elem: StochasticRadiosityElement,
    numberOfBits: number[],
    mostSignificantBits1: bigint[],
    rMostSignificantBits2: bigint[]
  ): void {
    let nb = 0;
    let b1 = 0n;
    let b2 = 0n;
    let current = elem;
    while (current.childNumber >= 0) {
      nb++;
      b1 = (b1 << 1n) | BigInt(current.childNumber & 1);
      b2 = (b2 >> 1n) | (BigInt(current.childNumber & 2) << BigInt(Niederreiter.NBITS - 2));
      current = current.parent as StochasticRadiosityElement;
    }

    numberOfBits[0] = nb;
    mostSignificantBits1[0] = b1;
    rMostSignificantBits2[0] = b2;
  }

  /**
Determines the regular sub-element at point (u,v) of the given parent
element. Returns the parent element itself if there are no regular sub-elements.
The point is transformed to the corresponding point on the sub-element
*/
  public static stochasticRadiosityElementRegularSubElementAtPoint(
    parent: StochasticRadiosityElement,
    u: number[],
    v: number[]
  ): StochasticRadiosityElement | null {
    let child: StochasticRadiosityElement | null = null;
    const _u = u[0];
    const _v = v[0];

    if (parent.isCluster() || parent.regularSubElements === null) {
      return null;
    }

    switch (parent.numberOfVertices) {
      case 3:
        if (_u + _v <= 0.5) {
          child = parent.regularSubElements[0] as StochasticRadiosityElement;
          u[0] = _u * 2.0;
          v[0] = _v * 2.0;
        }
        else if (_u > 0.5) {
          child = parent.regularSubElements[1] as StochasticRadiosityElement;
          u[0] = (_u - 0.5) * 2.0;
          v[0] = _v * 2.0;
        }
        else if (_v > 0.5) {
          child = parent.regularSubElements[2] as StochasticRadiosityElement;
          u[0] = _u * 2.0;
          v[0] = (_v - 0.5) * 2.0;
        }
        else {
          child = parent.regularSubElements[3] as StochasticRadiosityElement;
          u[0] = (0.5 - _u) * 2.0;
          v[0] = (0.5 - _v) * 2.0;
        }
        break;
      case 4:
        if (_v <= 0.5) {
          if (_u < 0.5) {
            child = parent.regularSubElements[0] as StochasticRadiosityElement;
            u[0] = _u * 2.0;
          }
          else {
            child = parent.regularSubElements[1] as StochasticRadiosityElement;
            u[0] = (_u - 0.5) * 2.0;
          }
          v[0] = _v * 2.0;
        }
        else {
          if (_u < 0.5) {
            child = parent.regularSubElements[2] as StochasticRadiosityElement;
            u[0] = _u * 2.0;
          }
          else {
            child = parent.regularSubElements[3] as StochasticRadiosityElement;
            u[0] = (_u - 0.5) * 2.0;
          }
          v[0] = (_v - 0.5) * 2.0;
        }
        break;
      default:
        VsdkLogger.fatal(-1, "galerkinElementRegularSubElementAtPoint", "Can handle only triangular or quadrilateral elements");
        break;
    }

    return child;
  }

  /**
Returns the leaf regular sub-element of 'top' at the point (u,v) (uniform
coordinates!). (u,v) is transformed to the coordinates of the corresponding
point on the leaf element. 'top' is a surface element, not a cluster
*/
  public static stochasticRadiosityElementRegularLeafElementAtPoint(top: StochasticRadiosityElement, u: number[], v: number[]): StochasticRadiosityElement {
    let leaf: StochasticRadiosityElement = top;

    while (leaf.regularSubElements !== null) {
      leaf = StochasticRadiosityElement.stochasticRadiosityElementRegularSubElementAtPoint(leaf, u, v)!;
    }

    return leaf;
  }

  private static monteCarloRadiosityInstallCoordinate(coord: Vector3D): Vector3D {
    const v = new Vector3D(coord.x, coord.y, coord.z);
    ElementHierarchyState.activeState().coords!.add(v);
    return v;
  }

  private static monteCarloRadiosityInstallNormal(normal: Vector3D): Vector3D {
    const v = new Vector3D(normal.x, normal.y, normal.z);
    ElementHierarchyState.activeState().normals!.add(v);
    return v;
  }

  private static monteCarloRadiosityInstallTexCoord(texCoord: Vector3D): Vector3D {
    const t = new Vector3D(texCoord.x, texCoord.y, texCoord.z);
    ElementHierarchyState.activeState().texCoords!.add(t);
    return t;
  }

  private static monteCarloRadiosityInstallVertex(coord: Vector3D, normal: Vector3D, texCoord: Vector3D | null): Vertex {
    const newPatchList: Patch[] = [];
    const v = new Vertex(coord, normal, texCoord, newPatchList);
    ElementHierarchyState.activeState().vertices!.add(v);
    return v;
  }

  private static monteCarloRadiosityNewMidpointVertex(elem: StochasticRadiosityElement, v1: Vertex, v2: Vertex): Vertex {
    const coord = new Vector3D();
    const norm = new Vector3D();
    const texCoord = new Vector3D();
    let p: Vector3D;
    let n: Vector3D;
    let t: Vector3D | null;

    coord.midPoint(v1.point, v2.point);
    p = StochasticRadiosityElement.monteCarloRadiosityInstallCoordinate(coord);

    if (v1.normal !== null && v2.normal !== null) {
      norm.midPoint(v1.normal, v2.normal);
      n = StochasticRadiosityElement.monteCarloRadiosityInstallNormal(norm);
    }
    else {
      n = elem.patch!.normal;
    }

    if (v1.textureCoordinates !== null && v2.textureCoordinates !== null) {
      texCoord.midPoint(v1.textureCoordinates, v2.textureCoordinates);
      t = StochasticRadiosityElement.monteCarloRadiosityInstallTexCoord(texCoord);
    }
    else {
      t = null;
    }

    return StochasticRadiosityElement.monteCarloRadiosityInstallVertex(p, n, t);
  }

  /**
Finds the surface element adjacent to 'elem' across the edge with index
'edgeNumber'. That edge is defined by:
  elem->vertices[edgeNumber]
  elem->vertices[(edgeNumber + 1) % elem->numberOfVertices]
The method searches the stochastic radiosity elements attached to the second
vertex and returns the first element (different from 'elem') that contains the
same edge with opposite orientation. Returns nullptr when no neighbour is
found.
*/
  private static monteCarloRadiosityElementNeighbour(elem: StochasticRadiosityElement, edgeNumber: number): StochasticRadiosityElement | null {
    const from = elem.vertices[edgeNumber]!;
    const to = elem.vertices[(edgeNumber + 1) % elem.numberOfVertices]!;

    for (let i = 0; to.radianceData !== null && i < to.radianceData.length; i++) {
      const element = to.radianceData[i];
      if (element.className !== ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY) {
        continue;
      }
      const e = element as StochasticRadiosityElement;
      if (
        e !== elem && (
          (
            e.numberOfVertices === 3
            && (
              (e.vertices[0] === to && e.vertices[1] === from)
              || (e.vertices[1] === to && e.vertices[2] === from)
              || (e.vertices[2] === to && e.vertices[0] === from)
            )
          )
          || (
            e.numberOfVertices === 4
            && (
              (e.vertices[0] === to && e.vertices[1] === from)
              || (e.vertices[1] === to && e.vertices[2] === from)
              || (e.vertices[2] === to && e.vertices[3] === from)
              || (e.vertices[3] === to && e.vertices[0] === from)
            )
          )
        )
      ) {
        return e;
      }
    }

    return null;
  }

  public static stochasticRadiosityElementEdgeMidpointVertex(elem: StochasticRadiosityElement, edgeNumber: number): Vertex | null {
    let v: Vertex | null = null;
    const to = elem.vertices[(edgeNumber + 1) % elem.numberOfVertices]!;
    const neighbour = StochasticRadiosityElement.monteCarloRadiosityElementNeighbour(elem, edgeNumber);

    if (neighbour !== null && neighbour.regularSubElements !== null) {
      let index: number;

      if (to === neighbour.vertices[0]) {
        index = 0;
      }
      else if (to === neighbour.vertices[1]) {
        index = 1;
      }
      else if (to === neighbour.vertices[2]) {
        index = 2;
      }
      else if (to === neighbour.vertices[3]) {
        index = 3;
      }
      else {
        index = -1;
      }

      switch (neighbour.numberOfVertices) {
        case 3:
          switch (index) {
            case 0:
              v = (neighbour.regularSubElements[0] as StochasticRadiosityElement).vertices[1]!;
              break;
            case 1:
              v = (neighbour.regularSubElements[1] as StochasticRadiosityElement).vertices[2]!;
              break;
            case 2:
              v = (neighbour.regularSubElements[2] as StochasticRadiosityElement).vertices[0]!;
              break;
            default:
              VsdkLogger.error("EdgeMidpointVertex", "Invalid vertex index %d", index);
              break;
          }
          break;
        case 4:
          switch (index) {
            case 0:
              v = (neighbour.regularSubElements[0] as StochasticRadiosityElement).vertices[1]!;
              break;
            case 1:
              v = (neighbour.regularSubElements[1] as StochasticRadiosityElement).vertices[2]!;
              break;
            case 2:
              v = (neighbour.regularSubElements[3] as StochasticRadiosityElement).vertices[3]!;
              break;
            case 3:
              v = (neighbour.regularSubElements[2] as StochasticRadiosityElement).vertices[0]!;
              break;
            default:
              VsdkLogger.error("EdgeMidpointVertex", "Invalid vertex index %d", index);
              break;
          }
          break;
        default:
          VsdkLogger.fatal(-1, "EdgeMidpointVertex", "only triangular and quadrilateral elements are supported");
          break;
      }
    }

    return v;
  }

  private static monteCarloRadiosityNewEdgeMidpointVertex(elem: StochasticRadiosityElement, edgeNumber: number): Vertex {
    let v = StochasticRadiosityElement.stochasticRadiosityElementEdgeMidpointVertex(elem, edgeNumber);
    if (v === null) {
      const from = elem.vertices[edgeNumber]!;
      const to = elem.vertices[(edgeNumber + 1) % elem.numberOfVertices]!;
      v = StochasticRadiosityElement.monteCarloRadiosityNewMidpointVertex(elem, from, to);
    }
    return v;
  }

  private static galerkinElementMidpoint(elem: StochasticRadiosityElement): Vector3D {
    elem.midPoint.set(0.0, 0.0, 0.0);
    for (let i = 0; i < elem.numberOfVertices; i++) {
      elem.midPoint.addition(elem.midPoint, elem.vertices[i]!.point);
    }
    elem.midPoint.inverseScaledCopy(elem.numberOfVertices, elem.midPoint, Numeric.EPSILON_FLOAT);

    return elem.midPoint;
  }

  /**
Only for surface elements
*/
  public static stochasticRadiosityElementIsTextured(elem: StochasticRadiosityElement): boolean {
    if (elem.isCluster()) {
      VsdkLogger.fatal(-1, "stochasticRadiosityElementIsTextured", "this routine should not be called for cluster elements");
      return false;
    }
    const mat = elem.patch!.material;
    return mat !== null
      && mat.getBsdf() !== null
      && (mat.getBsdf().splitBsdfIsTextured() || PhongEmittanceDistributionFunction.edfIsTextured());
  }

  /**
Uses elem->Rd for surface elements
*/
  public static stochasticRadiosityElementScalarReflectance(elem: StochasticRadiosityElement): number {
    let rd: number;

    if (elem.isCluster()) {
      return 1.0;
    }

    rd = elem.Rd.maximumComponent();
    if (rd < Numeric.EPSILON) {
      rd = Numeric.EPSILON_FLOAT;
    }
    return rd;
  }

  /**
Computes average reflectance and emittance of a surface sub-element
*/
  private static monteCarloRadiosityElementComputeAverageReflectanceAndEmittance(elem: StochasticRadiosityElement): void {
    const patch = elem.patch!;
    const nbits = [0];
    const msb1 = [0n];
    const rMostSignificantBit2 = [0n];
    const n = [1n];
    const albedo = new ColorRgb();
    const emittance = new ColorRgb();
    const hit = new RayHit();
    hit.init(patch, patch.midPoint, patch.normal, patch.material);

    const isTextured = StochasticRadiosityElement.stochasticRadiosityElementIsTextured(elem);
    const numberOfSamples = isTextured ? 100 : 1;
    albedo.clear();
    emittance.clear();
    StochasticRadiosityElement.stochasticRadiosityElementRange(elem, nbits, msb1, rMostSignificantBit2);

    for (let i = 0; i < numberOfSamples; i++, n[0]++) {
      let sample: ColorRgb;
      const xi = Niederreiter.NextNiedInRange(n, +1, nbits[0], msb1[0], rMostSignificantBit2[0]);
      if (xi === null) {
        continue;
      }
      hit.setUv(Number(xi[0]) * Niederreiter.RECIP, Number(xi[1]) * Niederreiter.RECIP);
      const newFlags = hit.getFlags() | RayHitFlag.UV;
      hit.setFlags(newFlags);
      const position = hit.getPoint();
      patch.uniformPoint(hit.getUv().u, hit.getUv().v, position);
      if (patch.material !== null && patch.material.getBsdf() !== null) {
        sample = patch.material.getBsdf().splitBsdfScatteredPower(hit, BsdfComponent.BRDF_DIFFUSE_COMPONENT);
        albedo.add(albedo, sample);
      }
      if (patch.material !== null && patch.material.getEdf() !== null) {
        sample = patch.material.getEdf().phongEmittance(hit, XxdfComponentFlag.DIFFUSE_COMPONENT);
        emittance.add(emittance, sample);
      }
    }
    elem.Rd.scaleInverse(numberOfSamples, albedo);
    elem.Ed.scaleInverse(numberOfSamples, emittance);
  }

  /**
Initial push operation for surface sub-elements
*/
  private static monteCarloRadiosityInitSurfacePush(parent: StochasticRadiosityElement, child: StochasticRadiosityElement): void {
    child.sourceRad = new ColorRgb(parent.sourceRad.r, parent.sourceRad.g, parent.sourceRad.b);
    StochasticRadiosityElement.stochasticRadiosityElementPushRadiance(parent, child, parent.radiance!, child.radiance!);
    StochasticRadiosityElement.stochasticRadiosityElementPushRadiance(parent, child, parent.unShotRadiance!, child.unShotRadiance!);

    const parentImportance = [parent.importance];
    const childImportance = [child.importance];
    StochasticRadiosityElement.stochasticRadiosityElementPushImportance(parentImportance, childImportance);
    child.importance = childImportance[0];

    const parentSourceImportance = [parent.sourceImportance];
    const childSourceImportance = [child.sourceImportance];
    StochasticRadiosityElement.stochasticRadiosityElementPushImportance(parentSourceImportance, childSourceImportance);
    child.sourceImportance = childSourceImportance[0];

    const parentUnShotImportance = [parent.unShotImportance];
    const childUnShotImportance = [child.unShotImportance];
    StochasticRadiosityElement.stochasticRadiosityElementPushImportance(parentUnShotImportance, childUnShotImportance);
    child.unShotImportance = childUnShotImportance[0];

    child.rayIndex = parent.rayIndex;
    child.quality = parent.quality;
    child.samplingProbability = parent.samplingProbability * child.area / parent.area;

    child.Rd = new ColorRgb(parent.Rd.r, parent.Rd.g, parent.Rd.b);
    child.Ed = new ColorRgb(parent.Ed.r, parent.Ed.g, parent.Ed.b);
    StochasticRadiosityElement.monteCarloRadiosityElementComputeAverageReflectanceAndEmittance(child);
  }

  /**
Creates a sub-element of the element "*parent", stores it as the
sub-element number "childNumber". Tha value of "v3" is unused in the
process of triangle subdivision.
*/
  private static monteCarloRadiosityCreateSurfaceSubElement(
    parent: StochasticRadiosityElement,
    childNumber: number,
    v0: Vertex,
    v1: Vertex,
    v2: Vertex,
    v3: Vertex | null
  ): StochasticRadiosityElement {
    const basisState = StochasticRadiosityBasisState.activeState();
    const elem = StochasticRadiosityElement.createElement();
    parent.regularSubElements![childNumber] = elem;

    elem.patch = parent.patch;
    elem.numberOfVertices = parent.numberOfVertices;
    elem.vertices[0] = v0;
    elem.vertices[1] = v1;
    elem.vertices[2] = v2;
    elem.vertices[3] = v3;
    for (let i = 0; i < elem.numberOfVertices; i++) {
      StochasticRadiosityElement.vertexAttachElement(elem.vertices[i]!, elem);
    }

    elem.area = 0.25 * parent.area;
    elem.midPoint = StochasticRadiosityElement.galerkinElementMidpoint(elem);

    elem.parent = parent;
    elem.childNumber = childNumber;
    elem.transformToParent = elem.numberOfVertices === 3
      ? basisState.triangleUpTransform[childNumber]
      : basisState.quadUpTransform[childNumber];

    Coefficientsmcrad.allocCoefficients(elem);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.radiance, elem.basis);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
    elem.importance = 0.0;
    elem.unShotImportance = 0.0;
    elem.receivedImportance = 0.0;
    StochasticRadiosityElement.monteCarloRadiosityInitSurfacePush(parent, elem);

    return elem;
  }

  /**
Create sub-elements: regular subdivision, see drawings above
*/
  private static monteCarloRadiosityRegularSubdivideTriangle(element: StochasticRadiosityElement, renderOptions: RendererConfiguration): StochasticRadiosityElement[] {
    void renderOptions;

    const v0 = element.vertices[0]!;
    const v1 = element.vertices[1]!;
    const v2 = element.vertices[2]!;
    const m0 = StochasticRadiosityElement.monteCarloRadiosityNewEdgeMidpointVertex(element, 0);
    const m1 = StochasticRadiosityElement.monteCarloRadiosityNewEdgeMidpointVertex(element, 1);
    const m2 = StochasticRadiosityElement.monteCarloRadiosityNewEdgeMidpointVertex(element, 2);

    StochasticRadiosityElement.monteCarloRadiosityCreateSurfaceSubElement(element, 0, v0, m0, m2, null);
    StochasticRadiosityElement.monteCarloRadiosityCreateSurfaceSubElement(element, 1, m0, v1, m1, null);
    StochasticRadiosityElement.monteCarloRadiosityCreateSurfaceSubElement(element, 2, m2, m1, v2, null);
    StochasticRadiosityElement.monteCarloRadiosityCreateSurfaceSubElement(element, 3, m1, m2, m0, null);

    return StochasticRadiosityElement.castElementArray(element.regularSubElements)!;
  }

  private static monteCarloRadiosityRegularSubdivideQuad(element: StochasticRadiosityElement, renderOptions: RendererConfiguration): StochasticRadiosityElement[] {
    void renderOptions;

    const v0 = element.vertices[0]!;
    const v1 = element.vertices[1]!;
    const v2 = element.vertices[2]!;
    const v3 = element.vertices[3]!;
    const m0 = StochasticRadiosityElement.monteCarloRadiosityNewEdgeMidpointVertex(element, 0);
    const m1 = StochasticRadiosityElement.monteCarloRadiosityNewEdgeMidpointVertex(element, 1);
    const m2 = StochasticRadiosityElement.monteCarloRadiosityNewEdgeMidpointVertex(element, 2);
    const m3 = StochasticRadiosityElement.monteCarloRadiosityNewEdgeMidpointVertex(element, 3);
    const mm = StochasticRadiosityElement.monteCarloRadiosityNewMidpointVertex(element, m0, m2);

    StochasticRadiosityElement.monteCarloRadiosityCreateSurfaceSubElement(element, 0, v0, m0, mm, m3);
    StochasticRadiosityElement.monteCarloRadiosityCreateSurfaceSubElement(element, 1, m0, v1, m1, mm);
    StochasticRadiosityElement.monteCarloRadiosityCreateSurfaceSubElement(element, 2, m3, mm, m2, v3);
    StochasticRadiosityElement.monteCarloRadiosityCreateSurfaceSubElement(element, 3, mm, m1, v2, m2);

    return StochasticRadiosityElement.castElementArray(element.regularSubElements)!;
  }

  /**
Subdivides given triangle or quadrangle into four sub-elements if not yet
done so before. Returns the list of created sub-elements
*/
  public static stochasticRadiosityElementRegularSubdivideElement(
    element: StochasticRadiosityElement,
    renderOptions: RendererConfiguration
  ): StochasticRadiosityElement[] | null {
    if (element.regularSubElements !== null) {
      return StochasticRadiosityElement.castElementArray(element.regularSubElements);
    }

    if (element.isCluster()) {
      VsdkLogger.fatal(-1, "galerkinElementRegularSubDivide", "Cannot regularly subdivide cluster elements");
      return null;
    }

    if (element.patch!.jacobian !== null) {
      VsdkLogger.warning(
        "galerkinElementRegularSubDivide",
        "irregular quadrilateral patches are not correctly handled (but you probably will not notice it)"
      );
    }

    element.regularSubElements = new Array<Element | null>(4);
    switch (element.numberOfVertices) {
      case 3:
        StochasticRadiosityElement.monteCarloRadiosityRegularSubdivideTriangle(element, renderOptions);
        break;
      case 4:
        StochasticRadiosityElement.monteCarloRadiosityRegularSubdivideQuad(element, renderOptions);
        break;
      default:
        VsdkLogger.fatal(-1, "galerkinElementRegularSubDivide", "invalid element: not 3 or 4 vertices");
        break;
    }
    return StochasticRadiosityElement.castElementArray(element.regularSubElements);
  }

  private static monteCarloRadiosityDestroyElement(elem: StochasticRadiosityElement | null): void {
    if (elem === null) {
      return;
    }
    if (elem.isCluster()) {
      ElementHierarchyState.activeState().nr_clusters--;
    }
    ElementHierarchyState.activeState().nr_elements--;

    if (elem.irregularSubElements !== null) {
      elem.irregularSubElements.length = 0;
      elem.irregularSubElements = null;
    }

    if (elem.regularSubElements !== null) {
      elem.regularSubElements = null;
    }

    Coefficientsmcrad.disposeCoefficients(elem);
  }

  private static monteCarloRadiosityDestroySurfaceElement(elem: StochasticRadiosityElement | null): void {
    if (elem === null) {
      return;
    }
    if (elem.regularSubElements !== null) {
      for (let i = 0; i < 4; i++) {
        StochasticRadiosityElement.monteCarloRadiosityDestroySurfaceElement(elem.regularSubElements[i] as StochasticRadiosityElement);
      }
    }
    StochasticRadiosityElement.monteCarloRadiosityDestroyElement(elem);
  }

  public static stochasticRadiosityElementDestroy(elem: StochasticRadiosityElement): void {
    StochasticRadiosityElement.monteCarloRadiosityDestroySurfaceElement(elem);
  }

  public static stochasticRadiosityElementDestroyClusterHierarchy(top: StochasticRadiosityElement | null): void {
    if (top === null || !top.isCluster()) {
      return;
    }
    for (let i = 0; top.irregularSubElements !== null && i < top.irregularSubElements.length; i++) {
      const element = top.irregularSubElements[i] as StochasticRadiosityElement;
      if (element.isCluster()) {
        StochasticRadiosityElement.stochasticRadiosityElementDestroyClusterHierarchy(element);
      }
    }
    StochasticRadiosityElement.monteCarloRadiosityDestroyElement(top);
  }

  private static regularChild(child: StochasticRadiosityElement): boolean {
    return child.childNumber >= 0 && child.childNumber <= 3;
  }

  public static stochasticRadiosityElementPushRadiance(
    parent: StochasticRadiosityElement,
    child: StochasticRadiosityElement,
    parentRadiance: ColorRgb[],
    childRadiance: ColorRgb[]
  ): void {
    if (parent.isCluster() || child.basis!.size === 1) {
      childRadiance[0].add(childRadiance[0], parentRadiance[0]);
    }
    else if (StochasticRadiosityElement.regularChild(child) && child.basis === parent.basis) {
      Basismcrad.filterColorDown(
        parentRadiance,
        child.basis!.regularFilter![child.childNumber],
        childRadiance,
        child.basis!.size
      );
    }
    else {
      VsdkLogger.fatal(
        -1,
        "stochasticRadiosityElementPushRadiance",
        "Not implemented for higher order approximations on irregular child elements or for different parent and child basis"
      );
    }
  }

  public static stochasticRadiosityElementPushImportance(parentImportance: number[], childImportance: number[]): void {
    childImportance[0] += parentImportance[0];
  }

  public static stochasticRadiosityElementPullRadiance(
    parent: StochasticRadiosityElement,
    child: StochasticRadiosityElement,
    parentRad: ColorRgb[],
    childRad: ColorRgb[]
  ): void {
    const areaFactor = child.area / parent.area;
    if (parent.isCluster() || child.basis!.size === 1) {
      parentRad[0].addScaled(parentRad[0], areaFactor, childRad[0]);
    }
    else if (StochasticRadiosityElement.regularChild(child) && child.basis === parent.basis) {
      Basismcrad.filterColorUp(
        childRad,
        child.basis!.regularFilter![child.childNumber],
        parentRad,
        child.basis!.size,
        areaFactor
      );
    }
    else {
      VsdkLogger.fatal(
        -1,
        "stochasticRadiosityElementPullRadiance",
        "Not implemented for higher order approximations on irregular child elements or for different parent and child basis"
      );
    }
  }

  public static stochasticRadiosityElementPullImportance(
    parent: StochasticRadiosityElement,
    child: StochasticRadiosityElement,
    parentImportance: number[],
    childImportance: number[]
  ): void {
    parentImportance[0] += child.area / parent.area * childImportance[0];
  }

  public static stochasticRadiosityElementColor(element: StochasticRadiosityElement): ColorRgb {
    const color = new ColorRgb();

    switch (StochasticRelaxation.activeState().show) {
      case WhatToShow.SHOW_TOTAL_RADIANCE:
      case WhatToShow.SHOW_INDIRECT_RADIANCE:
        ToneMap.radianceToRgb(
          StochasticRadiosityElement.stochasticRadiosityElementDisplayRadiance(element),
          color,
          StochasticRelaxation.activeState().toneMapOptions!
        );
        break;
      case WhatToShow.SHOW_IMPORTANCE: {
        let gray: number;

        if (element.importance > 1.0) {
          gray = 1.0;
        }
        else {
          gray = element.importance < 0.0 ? 0.0 : element.importance;
        }

        color.set(gray, gray, gray);
        break;
      }
      default:
        VsdkLogger.fatal(
          -1,
          "stochasticRadiosityElementColor",
          "Do not know what to display (StochasticRelaxation::activeState().show = %d)",
          StochasticRelaxation.activeState().show
        );
        break;
    }

    return color;
  }

  private static vertexRadiance(v: Vertex): ColorRgb {
    let count = 0;
    const radiance = new ColorRgb();

    radiance.clear();
    for (let i = 0; v.radianceData !== null && i < v.radianceData.length; i++) {
      const element = v.radianceData[i];
      if (element.className !== ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY) {
        continue;
      }
      const elem = element as StochasticRadiosityElement;
      if (elem.regularSubElements === null) {
        const elementRadiosity = StochasticRadiosityElement.stochasticRadiosityElementDisplayRadiance(elem);
        radiance.add(radiance, elementRadiosity);
        count++;
      }
    }

    if (count > 0) {
      radiance.scaleInverse(count, radiance);
    }

    return radiance;
  }

  /**
Same as above but for importance
*/
  private static vertexImportance(v: Vertex): number {
    let count = 0;
    let imp = 0.0;

    for (let i = 0; v.radianceData !== null && i < v.radianceData.length; i++) {
      const genericElement = v.radianceData[i];
      if (genericElement.className !== ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY) {
        continue;
      }
      const element = genericElement as StochasticRadiosityElement;
      if (element.regularSubElements === null) {
        imp += element.importance;
        count++;
      }
    }

    if (count > 0) {
      imp /= count;
    }

    return imp;
  }

  private static vertexColor(v: Vertex): ColorRgb {
    switch (StochasticRelaxation.activeState().show) {
      case WhatToShow.SHOW_TOTAL_RADIANCE:
      case WhatToShow.SHOW_INDIRECT_RADIANCE:
        ToneMap.radianceToRgb(
          StochasticRadiosityElement.vertexRadiance(v),
          v.color,
          StochasticRelaxation.activeState().toneMapOptions!
        );
        break;
      case WhatToShow.SHOW_IMPORTANCE: {
        let gray = StochasticRadiosityElement.vertexImportance(v);
        if (gray > 1.0) {
          gray = 1.0;
        }
        if (gray < 0.0) {
          gray = 0.0;
        }
        v.color.set(gray, gray, gray);
        break;
      }
      default:
        VsdkLogger.fatal(
          -1,
          "vertexColor",
          "Do not know what to display (StochasticRelaxation::activeState().show = %d)",
          StochasticRelaxation.activeState().show
        );
        break;
    }

    return v.color;
  }

  /**
Compute new vertex colors
*/
  public static stochasticRadiosityElementComputeNewVertexColors(element: Element): void {
    const stochasticRadiosityElement = element as StochasticRadiosityElement;
    StochasticRadiosityElement.vertexColor(stochasticRadiosityElement.vertices[0]!);
    StochasticRadiosityElement.vertexColor(stochasticRadiosityElement.vertices[1]!);
    StochasticRadiosityElement.vertexColor(stochasticRadiosityElement.vertices[2]!);
    if (stochasticRadiosityElement.numberOfVertices > 3) {
      StochasticRadiosityElement.vertexColor(stochasticRadiosityElement.vertices[3]!);
    }
  }

  public static stochasticRadiosityElementAdjustTVertexColors(element: Element): void {
    const stochasticRadiosityElement = element as StochasticRadiosityElement;
    const m: Array<Vertex | null> = [null, null, null, null];
    let n = 0;
    for (let i = 0; i < stochasticRadiosityElement.numberOfVertices; i++) {
      m[i] = StochasticRadiosityElement.stochasticRadiosityElementEdgeMidpointVertex(stochasticRadiosityElement, i);
      if (m[i] !== null) {
        n++;
      }
    }

    if (n > 0) {
      const color = StochasticRadiosityElement.stochasticRadiosityElementColor(stochasticRadiosityElement);
      for (let i = 0; i < stochasticRadiosityElement.numberOfVertices; i++) {
        if (m[i] !== null) {
          m[i]!.color.r = (m[i]!.color.r + color.r) * 0.5;
          m[i]!.color.g = (m[i]!.color.g + color.g) * 0.5;
          m[i]!.color.b = (m[i]!.color.b + color.b) * 0.5;
        }
      }
    }
  }

  public static stochasticRadiosityElementDisplayRadiance(elem: StochasticRadiosityElement): ColorRgb {
    const radiance = new ColorRgb();
    radiance.subtract(elem.radiance![0], elem.sourceRad);

    if (StochasticRelaxation.activeState().show !== WhatToShow.SHOW_INDIRECT_RADIANCE) {
      radiance.add(radiance, elem.sourceRad);
      if (StochasticRelaxation.activeState().indirectOnly !== 0 || StochasticRelaxation.activeState().doNonDiffuseFirstShot !== 0) {
        radiance.add(radiance, elem.Ed);
      }
    }
    return radiance;
  }

  public static stochasticRadiosityElementDisplayRadianceAtPoint(
    elem: StochasticRadiosityElement,
    u: number,
    v: number,
    renderOptions: RendererConfiguration
  ): ColorRgb {
    let radiance = new ColorRgb();
    if (elem.basis!.size === 1) {
      if (renderOptions.smoothShading) {
        const rad: ColorRgb[] = [new ColorRgb(), new ColorRgb(), new ColorRgb(), new ColorRgb()];
        for (let i = 0; i < elem.numberOfVertices; i++) {
          rad[i] = StochasticRadiosityElement.vertexRadiance(elem.vertices[i]!);
        }
        switch (elem.numberOfVertices) {
          case 3:
            radiance.interpolateBarycentric(rad[0], rad[1], rad[2], u, v);
            break;
          case 4:
            radiance.interpolateBiLinear(rad[0], rad[1], rad[2], rad[3], u, v);
            break;
          default:
            VsdkLogger.fatal(
              -1,
              "stochasticRadiosityElementDisplayRadianceAtPoint",
              "can only handle triangular or quadrilateral elements"
            );
            break;
        }
      }
      else {
        radiance = StochasticRadiosityElement.stochasticRadiosityElementDisplayRadiance(elem);
      }
    }
    else {
      radiance = Basismcrad.colorAtUv(elem.basis!, elem.radiance!, u, v);
      if (StochasticRelaxation.activeState().show === WhatToShow.SHOW_INDIRECT_RADIANCE) {
        radiance.subtract(radiance, elem.sourceRad);
      }
    }
    return radiance;
  }

  public getPatch(): Patch | null {
    return this.patch;
  }

  public setPatch(inPatch: Patch): void {
    this.patch = inPatch;
  }

  public getGeometry(): Geometry | null {
    return this.geometry;
  }

  public setGeometry(inGeometry: Geometry): void {
    this.geometry = inGeometry;
  }

  private static castElementArray(array: Array<Element | null> | null): StochasticRadiosityElement[] | null {
    if (array === null) {
      return null;
    }
    const out = new Array<StochasticRadiosityElement>(array.length);
    for (let i = 0; i < array.length; i++) {
      out[i] = array[i] as StochasticRadiosityElement;
    }
    return out;
  }
}
