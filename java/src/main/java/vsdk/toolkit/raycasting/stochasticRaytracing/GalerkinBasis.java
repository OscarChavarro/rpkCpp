package vsdk.toolkit.raycasting.stochasticRaytracing;

/**
All bases are orthonormal on their standard domain
*/
public class GalerkinBasis {
    public static final int MAX_BASIS_SIZE = 10;

    @FunctionalInterface
    public interface BasisFunction {
        double eval(double u, double v);
    }

    public String description;
    public int size; // Number of basis functions

    // function[alpha](u,v) evaluates \alpha-th basis function at (u,v)
    public BasisFunction[] function;

    // Same, but evaluated dual basis function (on standard domain)
    public BasisFunction[] dualFunction;

    // Push-pull filter coefficients for regular quadtree subdivision.
    // regular_filter[sigma][alpha][beta] is the filter coefficient
    // relating basis function alpha on the parent element with
    // basis function beta on the regular sub-element with index
    // sigma. See pushRadiance() and pullRadiance()
    public double[][][] regularFilter;
}
