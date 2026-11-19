package vsdk.toolkit.numericalAnalysis;
/**
Note that <u[i], v[i], t[i]> where 0 <= i < numberOfNodes are points that samples valid positions
inside modelled patch geometry (t[i] = 0 for 2D elements). This is used on form factor visibility
computations.
*/


/**
Numerical cubature rules needed to compute form factors
*/
public class CubatureRule {
    public static final int MAXIMUM_NODES = 20;

    /**
    Note that <u[i], v[i], t[i]> where 0 <= i < numberOfNodes are points that samples valid positions
    inside modelled patch geometry (t[i] = 0 for 2D elements). This is used on form factor visibility
    computations.
    */
    public String description; // Description of the rule
    public int numberOfNodes;
    public double[] u; // Abscissa (u, v, [t]) and weights w
    public double[] v;
    public double[] t;
    public double[] w;

    public CubatureRule() {
        description = null;
        numberOfNodes = 0;
        u = new double[MAXIMUM_NODES];
        v = new double[MAXIMUM_NODES];
        t = new double[MAXIMUM_NODES];
        w = new double[MAXIMUM_NODES];
    }

    public CubatureRule(
        String description,
        int numberOfNodes,
        double[] u,
        double[] v,
        double[] t,
        double[] w) {
        this();
        this.description = description;
        this.numberOfNodes = numberOfNodes;
        copyValues(this.u, u);
        copyValues(this.v, v);
        copyValues(this.t, t);
        copyValues(this.w, w);
    }

    private static void copyValues(double[] destination, double[] source) {
        if (destination == null || source == null) {
            return;
        }
        int n = Math.min(destination.length, source.length);
        System.arraycopy(source, 0, destination, 0, n);
    }
}
