/**
Higher order approximations for Galerkin radiosity
*/

package vsdk.toolkit.galerkin;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.numericalAnalysis.CubatureRule;

/**
All bases are orthonormal on their standard domain
*/
public class GalerkinBasis {
    public static final int MAX_BASIS_SIZE = 10;

    public interface BasisFunction {
        double evaluate(double u, double v);
    }

    public String description; // For debugging
    public int size; // Number of basis functions

    // function[alpha](u,v) evaluates phi_\alpha at (u, v)
    public BasisFunction[] function = new BasisFunction[MAX_BASIS_SIZE];

    // Push-pull filter coefficients for regular subdivision.
    // regular_filter[sigma][alpha][beta] is the filter coefficient
    // relating basis function alpha on the parent element with
    // basis function beta on the regular sub-element with index
    // sigma. See PushRadiance() and PullRadiance() in basis.c
    public double[][][] regularFilter = new double[4][MAX_BASIS_SIZE][MAX_BASIS_SIZE];

    public static ColorRgb radianceAtPoint(
        GalerkinElement element,
        ColorRgb[] coefficients,
        double u,
        double v)
    {
        ColorRgb c = new ColorRgb();
        if ( element == null || coefficients == null ) {
            return c;
        }

        GalerkinBasis basis = mutableBasisForVertexCount(
            element.patch != null ? element.patch.getNumberOfVertices() : 4);
        if ( basis == null ) {
            return c;
        }

        int n = Math.min(element.basisUsed, Math.min(basis.size, coefficients.length));
        for ( int i = 0; i < n; i++ ) {
            double f = (double)basis.function[i].evaluate(u, v);
            c = c.addScaled(f, coefficients[i]);
        }
        return c;
    }

    public static void push(
        GalerkinElement element,
        ColorRgb[] parentCoefficients,
        GalerkinElement child,
        ColorRgb[] childCoefficients)
    {
        if ( element == null || child == null || parentCoefficients == null || childCoefficients == null ) {
            return;
        }

        int sigma = child.childNumber;
        if ( element.isCluster() ) {
            ColorRgbMath.arrayClear(childCoefficients, child.basisSize);
            childCoefficients[0] = new ColorRgb(parentCoefficients[0].getR(), parentCoefficients[0].getG(), parentCoefficients[0].getB());
            return;
        }

        if ( sigma < 0 || sigma > 3 ) {
            Logger.reportMessage(null, Logger.ERROR, "GalerkinBasis::push", "Not yet implemented for non-regular subdivision");
            ColorRgbMath.arrayClear(childCoefficients, child.basisSize);
            childCoefficients[0] = new ColorRgb(parentCoefficients[0].getR(), parentCoefficients[0].getG(), parentCoefficients[0].getB());
            return;
        }

        GalerkinBasis basis = basisForVertexCount(child.patch != null ? child.patch.getNumberOfVertices() : 4);
        if ( basis == null ) {
            return;
        }

        int a = Math.min(child.basisSize, childCoefficients.length);
        int b = Math.min(element.basisSize, parentCoefficients.length);
        for ( int beta = 0; beta < a; beta++ ) {
            childCoefficients[beta] = new ColorRgb(0.0, 0.0, 0.0);
            for ( int alpha = 0; alpha < b; alpha++ ) {
                double f = basis.regularFilter[sigma][alpha][beta];
                if ( f < -VSDK.EPSILON || f > VSDK.EPSILON ) {
                    childCoefficients[beta] = childCoefficients[beta].addScaled((double)f, parentCoefficients[alpha]);
                }
            }
        }
    }

    public static void pushPullRadiance(GalerkinElement top, GalerkinState galerkinState) {
        if ( top == null || galerkinState == null ) {
            return;
        }
        ColorRgb[] bdown = new ColorRgb[MAX_BASIS_SIZE];
        ColorRgb[] bup = new ColorRgb[MAX_BASIS_SIZE];
        for ( int i = 0; i < MAX_BASIS_SIZE; i++ ) {
            bdown[i] = new ColorRgb();
            bup[i] = new ColorRgb();
        }
        pushPullRadianceRecursive(top, bdown, bup, galerkinState);
    }

    public static void computeRegularFilterCoefficients(
        GalerkinBasis basis,
        Matrix2x2[] upTransform,
        CubatureRule cubaRule)
    {
        if ( basis == null || upTransform == null || cubaRule == null ) {
            return;
        }
        for ( int sigma = 0; sigma < 4; sigma++ ) {
            computeFilterCoefficients(
                basis,
                basis.size,
                basis,
                basis.size,
                upTransform[sigma],
                cubaRule,
                basis.regularFilter[sigma]);
        }
    }

    public static GalerkinBasis basisForVertexCount(int numberOfVertices) {
        if ( numberOfVertices == 3 ) {
            return BasisTriGalerkin.instance();
        }
        return BasisQuadGalerkin.instance();
    }

    public static GalerkinBasis mutableBasisForVertexCount(int numberOfVertices) {
        return basisForVertexCount(numberOfVertices);
    }

    private static void pull(
        GalerkinElement parent,
        ColorRgb[] parentCoefficients,
        GalerkinElement child,
        ColorRgb[] childCoefficients)
    {
        if ( parent == null || parentCoefficients == null || child == null || childCoefficients == null ) {
            return;
        }

        int sigma = child.childNumber;
        if ( parent.isCluster() ) {
            ColorRgbMath.arrayClear(parentCoefficients, parent.basisSize);
            parentCoefficients[0] = childCoefficients[0].multiply(parent.area > 0.0 ? child.area / parent.area : 0.0);
            return;
        }

        if ( sigma < 0 || sigma > 3 ) {
            Logger.reportMessage(null, Logger.ERROR, "stochasticJacobiPull", "Not yet implemented for non-regular subdivision");
            ColorRgbMath.arrayClear(parentCoefficients, parent.basisSize);
            parentCoefficients[0] = new ColorRgb(childCoefficients[0].getR(), childCoefficients[0].getG(), childCoefficients[0].getB());
            return;
        }

        GalerkinBasis basis = GalerkinBasis.basisForVertexCount(child.patch != null ? child.patch.getNumberOfVertices() : 4);
        for ( int alpha = 0; alpha < parent.basisSize; alpha++ ) {
            parentCoefficients[alpha] = new ColorRgb(0.0, 0.0, 0.0);
            for ( int beta = 0; beta < child.basisSize; beta++ ) {
                double f = basis.regularFilter[sigma][alpha][beta];
                if ( f < -VSDK.EPSILON || f > VSDK.EPSILON ) {
                    parentCoefficients[alpha] = parentCoefficients[alpha].addScaled((double)f, childCoefficients[beta]);
                }
            }
            parentCoefficients[alpha] = parentCoefficients[alpha].multiply(0.25);
        }
    }

    private static void pushPullRadianceRecursive(
        GalerkinElement element,
        ColorRgb[] bdown,
        ColorRgb[] bup,
        GalerkinState galerkinState)
    {
        if ( element == null ) {
            return;
        }

        int n = Math.min(element.basisSize, bdown.length);
        for ( int i = 0; i < n; i++ ) {
            bdown[i] = bdown[i].addScaled(element.area > 0.0 ? 1.0 / element.area : 0.0, element.receivedRadiance[i]);
            element.receivedRadiance[i] = new ColorRgb(0.0, 0.0, 0.0);
            bup[i] = new ColorRgb(0.0, 0.0, 0.0);
        }

        if ( element.regularSubElements == null && element.irregularSubElements == null && element.patch != null ) {
            // Leaf-element, multiply with reflectivity at the lowest level
            ColorRgb rho = element.galerkinState.topLevelElement(element.patch).Rd;
            for ( int i = 0; i < n; i++ ) {
                bup[i] = rho.multiply(bdown[i]);
            }

            if ( galerkinState.isJacobi()
                || galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL ) {
                // Add self-emitted radiance
                ColorRgb Ed = element.galerkinState.topLevelElement(element.patch).Ed;
                bup[0] = bup[0].add(Ed);
            }
        }

        if ( element.regularSubElements != null ) {
            for ( int i = 0; i < 4; i++ ) {
                if ( !(element.regularSubElements[i] instanceof GalerkinElement) ) {
                    continue;
                }
                GalerkinElement child = (GalerkinElement)element.regularSubElements[i];
                ColorRgb[] btmp = freshColorArray();
                ColorRgb[] bdown2 = freshColorArray();
                ColorRgb[] bup2 = freshColorArray();
                push(element, bdown, child, bdown2);
                pushPullRadianceRecursive(child, bdown2, btmp, galerkinState);
                pull(element, bup2, child, btmp);
                ColorRgbMath.arrayAdd(bup, bup2, n);
            }
        }

        if ( element.irregularSubElements != null ) {
            for ( int i = 0; i < element.irregularSubElements.size(); i++ ) {
                if ( !(element.irregularSubElements.get(i) instanceof GalerkinElement) ) {
                    continue;
                }
                GalerkinElement subElement = (GalerkinElement)element.irregularSubElements.get(i);
                ColorRgb[] btmp = freshColorArray();
                ColorRgb[] bdown2 = freshColorArray();
                ColorRgb[] bup2 = freshColorArray();
                if ( element.isCluster() ) {
                    push(element, bdown, subElement, bdown2);
                }
                else {
                    ColorRgbMath.arrayClear(bdown2, n);
                }
                pushPullRadianceRecursive(subElement, bdown2, btmp, galerkinState);
                pull(element, bup2, subElement, btmp);
                ColorRgbMath.arrayAdd(bup, bup2, n);
            }
        }

        if ( galerkinState.isJacobi()
            || galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL ) {
            ColorRgbMath.arrayCopy(element.radiance, bup, n);
        }
        else {
            ColorRgbMath.arrayAdd(element.radiance, bup, n);
            ColorRgbMath.arrayAdd(element.unShotRadiance, bup, n);
        }
    }

    private static void computeFilterCoefficients(
        GalerkinBasis parentBasis,
        int parentSize,
        GalerkinBasis childBasis,
        int childSize,
        Matrix2x2 upTransform,
        CubatureRule cubatureRule,
        double[][] filter)
    {
        for ( int alpha = 0; alpha < parentSize; alpha++ ) {
            for ( int beta = 0; beta < childSize; beta++ ) {
                double x = 0.0;
                for ( int k = 0; k < cubatureRule.numberOfNodes; k++ ) {
                    Vector2Dd up = new Vector2Dd((double)cubatureRule.u[k], (double)cubatureRule.v[k]);
                    up = upTransform.transformPoint(up);
                    x += cubatureRule.w[k]
                        * parentBasis.function[alpha].evaluate(up.x(), up.y())
                        * childBasis.function[beta].evaluate(cubatureRule.u[k], cubatureRule.v[k]);
                }
                filter[alpha][beta] = x;
            }
        }
    }

    private static ColorRgb[] freshColorArray() {
        ColorRgb[] c = new ColorRgb[MAX_BASIS_SIZE];
        for ( int i = 0; i < MAX_BASIS_SIZE; i++ ) {
            c[i] = new ColorRgb();
        }
        return c;
    }
}
