import { ColorRgb } from "../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../common/logging/Logger";
import { RenderOptions } from "../common/RenderOptions";
import { Matrix2x2 } from "../common/linealAlgebra/Matrix2x2";
import { ElementFlags } from "./ElementFlags";

export class Element {
  public id: number;
  public Ed: ColorRgb;
  public Rd: ColorRgb;
  public radiance: ColorRgb[] | null;
  public receivedRadiance: ColorRgb[] | null;
  public unShotRadiance: ColorRgb[] | null;
  public area: number;
  public className: number;
  public flags: number;

  public parent: Element | null;
  public regularSubElements: Array<Element | null> | null;
  public irregularSubElements: Element[] | null;
  public transformToParent: Matrix2x2 | null;

  public constructor() {
    this.id = 0;
    this.Ed = new ColorRgb();
    this.Rd = new ColorRgb();
    this.radiance = null;
    this.receivedRadiance = null;
    this.unShotRadiance = null;
    this.area = 0.0;
    this.className = 0;
    this.flags = 0x00;

    this.parent = null;
    this.regularSubElements = null;
    this.irregularSubElements = null;
    this.transformToParent = null;

    this.Ed.clear();
    this.Rd.clear();
  }

  public isCluster(): boolean {
    return (this.flags & ElementFlags.IS_CLUSTER_MASK) !== 0;
  }

  public topTransform(transform: Matrix2x2): Matrix2x2 | null {
    if (this.transformToParent === null) {
      return null;
    }

    let windowElement: Element | null = this;
    Element.copyMatrix(windowElement.transformToParent as Matrix2x2, transform);

    do {
      windowElement = windowElement.parent;
      if (windowElement !== null && windowElement.transformToParent !== null) {
        windowElement.transformToParent.matrix2DPreConcatTransform(transform, transform);
      }
    } while (windowElement !== null && windowElement.transformToParent !== null);

    return transform;
  }

  public traverseAllLeafElements(traversalCallbackFunction: (element: Element) => void): void {
    for (let i = 0; this.irregularSubElements !== null && i < this.irregularSubElements.length; i++) {
      this.irregularSubElements[i].traverseAllLeafElements(traversalCallbackFunction);
    }

    if (this.regularSubElements !== null) {
      for (let i = 0; i < 4; i++) {
        this.regularSubElements[i]?.traverseAllLeafElements(traversalCallbackFunction);
      }
    }

    if (this.irregularSubElements === null && this.regularSubElements === null) {
      traversalCallbackFunction(this);
    }
  }

  public traverseClusterLeafElements(traversalCallbackFunction: (element: Element) => void): void {
    if (this.isCluster()) {
      for (let i = 0; this.irregularSubElements !== null && i < this.irregularSubElements.length; i++) {
        this.irregularSubElements[i]?.traverseClusterLeafElements(traversalCallbackFunction);
      }
    }
    else if (this.regularSubElements !== null) {
      for (let i = 0; i < 4; i++) {
        this.regularSubElements[i]?.traverseClusterLeafElements(traversalCallbackFunction);
      }
    }
    else {
      traversalCallbackFunction(this);
    }
  }

  public traverseQuadTreeLeafs(
    traversalCallbackFunction: (element: Element, renderOptions: RenderOptions) => void,
    renderOptions: RenderOptions
  ): void {
    if (this.regularSubElements === null) {
      traversalCallbackFunction(this, renderOptions);
    }
    else {
      for (let i = 0; i < 4; i++) {
        this.regularSubElements[i]?.traverseQuadTreeLeafs(traversalCallbackFunction, renderOptions);
      }
    }
  }

  public isLeaf(): boolean {
    return this.regularSubElements === null
      && (this.irregularSubElements === null || this.irregularSubElements.length === 0);
  }

  public childContainingElement(descendant: Element | null): Element | null {
    while (descendant !== null && descendant.parent !== this) {
      descendant = descendant.parent;
    }
    if (descendant === null) {
      VsdkLogger.fatal(-1, "Element::childContainingElement", "descendant is not a descendant of parent");
    }
    return descendant;
  }

  public traverseAllChildren(traversalCallbackFunction: (element: Element) => void): boolean {
    if (this.isCluster()) {
      for (let i = 0; this.irregularSubElements !== null && i < this.irregularSubElements.length; i++) {
        traversalCallbackFunction(this.irregularSubElements[i]);
      }
      return true;
    }
    else if (this.regularSubElements !== null) {
      for (let i = 0; i < 4; i++) {
        if (this.regularSubElements[i] !== null) {
          traversalCallbackFunction(this.regularSubElements[i] as Element);
        }
      }
      return true;
    }
    return false;
  }

  private static copyMatrix(source: Matrix2x2, target: Matrix2x2): void {
    target.m[0][0] = source.m[0][0];
    target.m[0][1] = source.m[0][1];
    target.m[1][0] = source.m[1][0];
    target.m[1][1] = source.m[1][1];
    target.t[0] = source.t[0];
    target.t[1] = source.t[1];
  }
}
