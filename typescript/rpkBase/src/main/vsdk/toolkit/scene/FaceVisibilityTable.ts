/**
Whether each face of a scene is directly visible from the camera, indexed by
the dense face index (Patch.getFaceIndex()). Every face starts invisible;
Potential.updateDirectVisibility rewrites the whole table from an ID render
of the current view, and the importance-driven stochastic radiosity reads it
as source importance. Owned by the scene.
*/
export class FaceVisibilityTable {
  private readonly visible: Uint8Array;
  private readonly numberOfFaces: number;

  public constructor(inNumberOfFaces: number) {
    this.numberOfFaces = inNumberOfFaces;
    this.visible = new Uint8Array(inNumberOfFaces > 0 ? inNumberOfFaces : 1);
  }

  public getNumberOfFaces(): number {
    return this.numberOfFaces;
  }

  public isVisible(faceIndex: number): boolean {
    return this.visible[faceIndex] !== 0;
  }

  public setVisible(faceIndex: number, isFaceVisible: boolean): void {
    this.visible[faceIndex] = isFaceVisible ? 1 : 0;
  }
}
