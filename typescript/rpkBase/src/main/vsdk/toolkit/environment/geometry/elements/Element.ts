import { ColorRgbMutable } from "../../../common/color/ColorRgbMutable";
import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { RendererConfiguration } from "../../../material/RendererConfiguration";
import { Matrix2x2 } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Matrix2x2";
import { ElementFlags } from "./ElementFlags";

export class Element {
  public id: number;
  public Ed: ColorRgbMutable;
  public Rd: ColorRgbMutable;
  public radiance: ColorRgbMutable[] | null;
  public receivedRadiance: ColorRgbMutable[] | null;
  public unShotRadiance: ColorRgbMutable[] | null;
  public area: number;
  public className: number;
  public flags: number;

  public parent: Element | null;
  public regularSubElements: Array<Element | null> | null;
  public irregularSubElements: Element[] | null;
  public transformToParent: Matrix2x2 | null;

  public constructor() {
    this.id = 0;
    this.Ed = new ColorRgbMutable();
    this.Rd = new ColorRgbMutable();
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

  public topTransform(): Matrix2x2 | null {
    // The C++ `Matrix2x2 *transform` output is the returned transform
    if (this.transformToParent === null) {
      return null;
    }

    let windowElement: Element | null = this;
    let transform: Matrix2x2 = windowElement.transformToParent as Matrix2x2;

    do {
      windowElement = windowElement.parent;
      if (windowElement !== null && windowElement.transformToParent !== null) {
        transform = windowElement.transformToParent.multiply(transform);
      }
    } while (windowElement !== null && windowElement.transformToParent !== null);

    return transform;
  }

  public traverseAllLeafElements(traversalCallbackFunction: (element: Element) => void): void {
    for (let i = 0; this.irregularSubElements !== null && i < this.irregularSubElements.length; i++) {
      this.irregularSubElements[i]?.traverseAllLeafElements(traversalCallbackFunction);
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
    traversalCallbackFunction: (element: Element, renderOptions: RendererConfiguration) => void,
    renderOptions: RendererConfiguration
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
      VsdkLogger.reportMessage(this, VsdkLogger.FATAL_ERROR, "Element::childContainingElement", "descendant is not a descendant of parent");
    }
    return descendant;
  }

  public traverseAllChildren(traversalCallbackFunction: (element: Element) => void): boolean {
    if (this.isCluster()) {
      for (let i = 0; this.irregularSubElements !== null && i < this.irregularSubElements.length; i++) {
        const child = this.irregularSubElements[i];
        if (child !== undefined) {
          traversalCallbackFunction(child);
        }
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
}
