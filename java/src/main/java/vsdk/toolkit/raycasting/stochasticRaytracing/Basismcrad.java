/**
Higher order approximations for Galerkin radiosity
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;
import vsdk.toolkit.common.linealAlgebra.Vector2D;
import vsdk.toolkit.numericalAnalysis.CubatureRule;
import vsdk.toolkit.numericalAnalysis.QuadCubatureRule;
import vsdk.toolkit.numericalAnalysis.TriangleCubatureRule;

public final class Basismcrad {
    private Basismcrad() {
    }

    public static double oneBasis(double u, double v) {
        return 1;
    }

    static GalerkinBasis makeBasis(StochasticRadiosityElementType et, StochasticRaytracingApproximation at) {
        StochasticRadiosityBasisState basisState = StochasticRadiosityBasisState.activeState();
        GalerkinBasis basis = basisState.quadBasis;
        String elem;

        switch ( et ) {
            case ET_TRIANGLE:
                basis = basisState.triBasis;
                elem = "triangles";
                break;
            case ET_QUAD:
                basis = basisState.quadBasis;
                elem = "quadrilaterals";
                break;
            default:
                Error.fatal(-1, "Basismcrad::makeBasis", "Invalid element type %d", et.ordinal());
                return basis;
        }

        GalerkinBasis out = cloneBasis(basis);
        out.size = basisState.approxDesc[at.ordinal()].basis_size;
        out.description = String.format("%s orthonormal basis for %s", basisState.approxDesc[at.ordinal()].name, elem);

        return out;
    }

    /**
Computes the filter coefficients for push-pull operations between a
parent and child with given basis and nr of basis functions. 'upxfm' is
the transform to be used to find the point on the parent corresponding
to a given point on the child. 'cr' is the cubature rule to be used
for computing the coefficients. The order should be at least the highest
product of the order of a parent and a child basis function. The filter
coefficients are filled in in the table 'filter'. The filter coefficients are:

H_{\alpha\,\beta} = int _S phi_\alpha(u',v') phi_\beta(u,v) du dv

with S the domain on which the basis functions are defined (unit square or
standard triangle), and (u',v') the result of "up-transforming" (u,v).
*/
    static void computeFilterCoefficients(
        GalerkinBasis parentBasis,
        int parentSize,
        GalerkinBasis childBasis,
        int childSize,
        Matrix2x2 upxfm,
        CubatureRule cr,
        double[][] filter)
    {
        for ( int a = 0; a < parentSize; a++ ) {
            for ( int b = 0; b < childSize; b++ ) {
                double x = 0.0;
                for ( int k = 0; k < cr.numberOfNodes; k++ ) {
                    Vector2D up = new Vector2D((float)cr.u[k], (float)cr.v[k]);
                    upxfm.transformPoint2D(up, up);
                    x += cr.w[k] * parentBasis.function[a].eval(up.x, up.y) *
                         childBasis.function[b].eval(cr.u[k], cr.v[k]);
                }
                filter[a][b] = x;
            }
        }
    }

    /**
Computes the push-pull filter coefficients for regular subdivision for
elements with given basis and uptransform. The cubature rule 'cr' is used
to compute the coefficients. The coefficients are filled in the
basis->regular_filter table
*/
    static void basisGalerkinComputeRegularFilterCoefficients(
        GalerkinBasis basis,
        Matrix2x2[] upxfm,
        CubatureRule cr)
    {
        for ( int s = 0; s < 4; s++ ) {
            computeFilterCoefficients(
                basis,
                basis.size,
                basis,
                basis.size,
                upxfm[s],
                cr,
                basis.regularFilter[s]);
        }
    }

    /**
Initialises table of bases
*/
    public static void monteCarloRadiosityInitBasis() {
        StochasticRadiosityBasisState basisState = StochasticRadiosityBasisState.activeState();
        if ( basisState.inited ) {
            return;
        }

        basisGalerkinComputeRegularFilterCoefficients(
            basisState.triBasis,
            basisState.triangleUpTransform,
            TriangleCubatureRule.degree8Rule());
        basisGalerkinComputeRegularFilterCoefficients(
            basisState.quadBasis,
            basisState.quadUpTransform,
            QuadCubatureRule.degree8QuadrilateralRule());

        for ( int et = 0; et < StochasticRadiosityElementTypeInfo.NUMBER_OF_ELEMENT_TYPES; et++ ) {
            for ( int at = 0; at < StochasticRadiosityBasisState.NUMBER_OF_APPROXIMATION_TYPES; at++ ) {
                basisState.basis[et][at] = makeBasis(StochasticRadiosityElementType.values()[et], StochasticRaytracingApproximation.values()[at]);
            }
        }
        basisState.inited = true;
    }

    /**
Returns color at a given point, with parameters (u,v)
*/
    public static ColorRgb colorAtUv(GalerkinBasis basis, ColorRgb[] rad, double u, double v) {
        ColorRgb res = new ColorRgb();
        res.clear();
        for ( int i = 0; i < basis.size; i++ ) {
            double s = basis.function[i].eval(u, v);
            res.addScaled(res, (float)s, rad[i]);
        }
        return res;
    }

    /**
These routine filter the source coefficients down/up and add
the result to the destination coefficients
*/
    public static void filterColorDown(ColorRgb[] parent, double[][] h, ColorRgb[] child, int n) {
        for ( int i = 0; i < n; i++ ) {
            for ( int j = 0; j < n; j++ ) {
                child[i].addScaled(child[i], (float)h[j][i], parent[j]);
            }
        }
    }

    public static void filterColorUp(ColorRgb[] child, double[][] h, ColorRgb[] parent, int n, double areaFactor) {
        for ( int i = 0; i < n; i++ ) {
            for ( int j = 0; j < n; j++ ) {
                double H = h[i][j] * areaFactor;
                parent[i].addScaled(parent[i], (float)H, child[j]);
            }
        }
    }

    static GalerkinBasis stochasticRadiosityCreateQuadBasis() {
        return Basisquadmcrad.stochasticRadiosityCreateQuadBasis();
    }

    private static GalerkinBasis cloneBasis(GalerkinBasis in) {
        GalerkinBasis out = new GalerkinBasis();
        out.description = in.description;
        out.size = in.size;
        out.function = in.function;
        out.dualFunction = in.dualFunction;
        out.regularFilter = in.regularFilter;
        return out;
    }

    /**
Cubic orthonormal basis for the unit square [0, 1] ^ 2
*/
    private static final class Basisquadmcrad {
        private Basisquadmcrad() {
        }

        private static double qm0(double u, double v) {
            return 1.000000000000000;
        }

        private static double qm1(double u, double v) {
            return -1.732050807568877 + 3.464101615137753 * u;
        }

        private static double qm2(double u, double v) {
            return -1.732050807568877 + 3.464101615137753 * v;
        }

        private static double qm3(double u, double v) {
            return 3.000000000000003 + -6.000000000000006 * u + -6.000000000000009 * v + 12.000000000000021 * u * v;
        }

        private static double qm4(double u, double v) {
            return 2.236067977499749 + -13.416407864998552 * u + 13.416407864998591 * u * u;
        }

        private static double qm5(double u, double v) {
            return 2.236067977499781 + -13.416407864998723 * v + 13.416407864998760 * v * v;
        }

        private static double qm6(double u, double v) {
            return -2.645751311064023 + 31.749015732770424 * u + -79.372539331927356 * u * u + 52.915026221285316 * u * u * u;
        }

        private static double qm7(double u, double v) {
            return -3.872983346207165 + 23.237900077242056 * u + 7.745966692414697 * v + -46.475800154488844 * u * v +
                   -23.237900077239200 * u * u + 46.475800154488617 * u * u * v;
        }

        private static double qm8(double u, double v) {
            return -3.872983346207866 + 7.745966692416303 * u + 23.237900077246348 * v + -46.475800154495623 * u * v +
                   -23.237900077245619 * v * v + 46.475800154491409 * u * v * v;
        }

        private static double qm9(double u, double v) {
            return -2.645751311064409 + 31.749015732781054 * v + -79.372539331951486 * v * v + 52.915026221299712 * v * v * v;
        }

        private static final GalerkinBasis.BasisFunction[] f = new GalerkinBasis.BasisFunction[] {
            Basisquadmcrad::qm0,
            Basisquadmcrad::qm1,
            Basisquadmcrad::qm2,
            Basisquadmcrad::qm3,
            Basisquadmcrad::qm4,
            Basisquadmcrad::qm5,
            Basisquadmcrad::qm6,
            Basisquadmcrad::qm7,
            Basisquadmcrad::qm8,
            Basisquadmcrad::qm9
        }; // Functions

        private static final double[][][] h = new double[4][GalerkinBasis.MAX_BASIS_SIZE][GalerkinBasis.MAX_BASIS_SIZE];  /* push-pull filter: computed in basis.c */

        private static GalerkinBasis stochasticRadiosityCreateQuadBasis() {
            GalerkinBasis b = new GalerkinBasis();
            b.description = "orthonormal basis on the unit square"; // Description
            b.size = GalerkinBasis.MAX_BASIS_SIZE; // Size
            b.function = f;
            b.dualFunction = f; // Primary and dual canonical basis functions are equal
            b.regularFilter = h; // Push-pull filter coefficients
            return b;
        }
    }
}
