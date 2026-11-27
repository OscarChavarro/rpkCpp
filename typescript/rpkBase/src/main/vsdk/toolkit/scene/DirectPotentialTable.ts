/**
Direct potential (importance received straight from the eye) of every face of
a scene, indexed by the dense face index (Patch.getFaceIndex()). All values
start at zero; Potential.updateDirectPotential rewrites them whenever the view
changes, and the importance-driven solvers read them. Owned by the scene.
*/
export class DirectPotentialTable {
  private readonly potentials: Float64Array;
  private readonly numberOfFaces: number;

  public constructor(inNumberOfFaces: number) {
    this.numberOfFaces = inNumberOfFaces;
    this.potentials = new Float64Array(inNumberOfFaces > 0 ? inNumberOfFaces : 1);
  }

  public getNumberOfFaces(): number {
    return this.numberOfFaces;
  }

  public get(faceIndex: number): number {
    return this.potentials[faceIndex]!;
  }

  public set(faceIndex: number, potential: number): void {
    this.potentials[faceIndex] = potential;
  }
}
