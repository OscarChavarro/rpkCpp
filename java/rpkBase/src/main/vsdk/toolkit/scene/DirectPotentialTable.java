package vsdk.toolkit.scene;

/**
Direct potential (importance received straight from the eye) of every face of
a scene, indexed by the dense face index (Patch::getFaceIndex()). All values
start at zero; Potential::updateDirectPotential rewrites them whenever the view
changes, and the importance-driven solvers read them. Owned by the scene.
*/
public class DirectPotentialTable {
    private final double[] potentials;
    private final int numberOfFaces;

    public DirectPotentialTable(int inNumberOfFaces) {
        numberOfFaces = inNumberOfFaces;
        potentials = new double[numberOfFaces > 0 ? numberOfFaces : 1];
    }

    public int getNumberOfFaces() {
        return numberOfFaces;
    }

    public double get(int faceIndex) {
        return potentials[faceIndex];
    }

    public void set(int faceIndex, double potential) {
        potentials[faceIndex] = potential;
    }
}
