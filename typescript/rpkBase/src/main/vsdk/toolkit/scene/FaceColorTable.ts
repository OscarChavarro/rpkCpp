import { ColorRgbMutable } from "../common/color/ColorRgbMutable";

/**
Display color of every face of a scene, indexed by the dense face index
(Patch.getFaceIndex()). Every face starts black; the radiance methods
rewrite a face's color from its radiance after each step, and the vertex
colors (Gouraud display, VRML export) read it. Owned by the scene.
*/
export class FaceColorTable {
  private readonly colors: ColorRgbMutable[];
  private readonly numberOfFaces: number;

  public constructor(inNumberOfFaces: number) {
    this.numberOfFaces = inNumberOfFaces;
    this.colors = new Array<ColorRgbMutable>(inNumberOfFaces > 0 ? inNumberOfFaces : 1);
    for (let i = 0; i < this.colors.length; i++) {
      this.colors[i] = new ColorRgbMutable(0.0, 0.0, 0.0);
    }
  }

  public getNumberOfFaces(): number {
    return this.numberOfFaces;
  }

  public get(faceIndex: number): ColorRgbMutable {
    return this.colors[faceIndex]!;
  }

  /** Stores a copy of color, as the source's assignment by value */
  public set(faceIndex: number, color: ColorRgbMutable): void {
    this.colors[faceIndex]!.set(color.r, color.g, color.b);
  }
}
