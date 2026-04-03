package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.ColorRgb;

public final class Coefficientsmcrad {
    private Coefficientsmcrad() {
    }

    public static void stochasticRadiosityClearCoefficients(ColorRgb[] c, GalerkinBasis galerkinBasis) {
        if ( c == null || galerkinBasis == null ) {
            return;
        }
        for ( int i = 0; i < galerkinBasis.size; i++ ) {
            if ( c[i] == null ) {
                c[i] = new ColorRgb();
            }
            c[i].clear();
        }
    }

    public static void stochasticRadiosityCopyCoefficients(ColorRgb[] dst, ColorRgb[] src, GalerkinBasis galerkinBasis) {
        if ( dst == null || src == null || galerkinBasis == null ) {
            return;
        }
        for ( int i = 0; i < galerkinBasis.size; i++ ) {
            if ( dst[i] == null ) {
                dst[i] = new ColorRgb();
            }
            if ( src[i] == null ) {
                src[i] = new ColorRgb();
            }
            dst[i].set(src[i].r, src[i].g, src[i].b);
        }
    }

    public static void stochasticRadiosityAddCoefficients(ColorRgb[] dst, ColorRgb[] extra, GalerkinBasis galerkinBasis) {
        if ( dst == null || extra == null || galerkinBasis == null ) {
            return;
        }
        for ( int i = 0; i < galerkinBasis.size; i++ ) {
            if ( dst[i] == null ) {
                dst[i] = new ColorRgb();
            }
            if ( extra[i] == null ) {
                extra[i] = new ColorRgb();
            }
            dst[i].add(dst[i], extra[i]);
        }
    }

    public static void stochasticRadiosityScaleCoefficients(float scale, ColorRgb[] color, GalerkinBasis galerkinBasis) {
        if ( color == null || galerkinBasis == null ) {
            return;
        }
        for ( int i = 0; i < galerkinBasis.size; i++ ) {
            if ( color[i] == null ) {
                color[i] = new ColorRgb();
            }
            color[i].scale(scale);
        }
    }

    public static void stochasticRadiosityMultiplyCoefficients(ColorRgb color, ColorRgb[] coefficients, GalerkinBasis galerkinBasis) {
        if ( coefficients == null || galerkinBasis == null || color == null ) {
            return;
        }
        ColorRgb c = new ColorRgb(color.r, color.g, color.b);

        for ( int i = 0; i < galerkinBasis.size; i++ ) {
            if ( coefficients[i] == null ) {
                coefficients[i] = new ColorRgb();
            }
            coefficients[i].selfScalarProduct(c);
        }
    }

    /**
Disposes previously allocated coefficients
*/
    public static void disposeCoefficients(StochasticRadiosityElement elem) {
        if ( elem.basis != null && elem.basis != StochasticRadiosityBasisState.activeState().dummyBasis && elem.radiance != null ) {
            elem.radiance = null;
            elem.unShotRadiance = null;
            elem.receivedRadiance = null;
        }
        initCoefficients(elem);
    }

    /**
Determines basis based on element type and currently desired approximation
*/
    private static GalerkinBasis actualBasis(StochasticRadiosityElement elem) {
        if ( elem.isCluster() ) {
            return StochasticRadiosityBasisState.activeState().clusterBasis;
        }

        int et = McradP.numberOfVertices(elem) == 3
            ? StochasticRadiosityElementType.ET_TRIANGLE.ordinal()
            : StochasticRadiosityElementType.ET_QUAD.ordinal();
        int at = StochasticRelaxation.activeState().approximationOrderType.ordinal();
        return StochasticRadiosityBasisState.activeState().basis[et][at];
    }

    /**
Allocates memory for radiance coefficients
*/
    public static void allocCoefficients(StochasticRadiosityElement elem) {
        disposeCoefficients(elem);
        elem.basis = actualBasis(elem);
        elem.radiance = createColors(elem.basis.size);
        elem.unShotRadiance = createColors(elem.basis.size);
        elem.receivedRadiance = createColors(elem.basis.size);
    }

    /**
Re-allocates memory for radiance coefficients if
the currently desired approximation order is not the same
as the approximation order for which the element has
been initialised before
*/
    public static void reAllocCoefficients(StochasticRadiosityElement elem) {
        if ( elem != null && elem.basis != actualBasis(elem) ) {
            allocCoefficients(elem);
        }
    }

    /**
Basically sets rad to nullptr
*/
    public static void initCoefficients(StochasticRadiosityElement elem) {
        if ( !StochasticRadiosityElement.coefficientPoolsAreInitialized() ) {
            StochasticRadiosityElement.markCoefficientPoolsInitialized();
        }

        elem.radiance = null;
        elem.unShotRadiance = null;
        elem.receivedRadiance = null;
        elem.basis = StochasticRadiosityBasisState.activeState().dummyBasis;
    }

    private static ColorRgb[] createColors(int n) {
        ColorRgb[] data = new ColorRgb[n];
        for ( int i = 0; i < n; i++ ) {
            data[i] = new ColorRgb();
        }
        return data;
    }
}
