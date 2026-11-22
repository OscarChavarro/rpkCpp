import type { Element } from "../environment/geometry/elements/Element";

/**
Top-level radiance element of every face of a scene, indexed by the dense face
index (Patch.getFaceIndex()). The current radiance method creates one element
per face when it is selected (Radiance.setRadianceMethod) and destroys them
when it is replaced; the slots are null otherwise. The table only binds faces
to elements: the elements' contents are the solver's state. Owned by the
scene; the elements are owned by the radiance method that created them.
*/
export class RadianceElementTable {
  private readonly elements: Array<Element | null>;
  private readonly numberOfFaces: number;

  public constructor(inNumberOfFaces: number) {
    this.numberOfFaces = inNumberOfFaces;
    this.elements = new Array<Element | null>(inNumberOfFaces > 0 ? inNumberOfFaces : 1).fill(null);
  }

  public getNumberOfFaces(): number {
    return this.numberOfFaces;
  }

  public get(faceIndex: number): Element | null {
    return this.elements[faceIndex]!;
  }

  public set(faceIndex: number, element: Element | null): void {
    this.elements[faceIndex] = element;
  }
}
