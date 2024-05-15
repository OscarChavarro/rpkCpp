#include "common/numericalAnalysis/QuadCubatureRule.h"
#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include <cstring>

#include "common/error.h"
#include "common/numericalAnalysis/CubatureRule.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"

GalerkinBasis GLOBAL_stochasticRadiosity_basis[NR_ELEMENT_TYPES][NR_APPROX_TYPES];
GalerkinBasis GLOBAL_stochasticRadiosity_dummyBasis = {
        "dummy basis",
        0, nullptr, nullptr
};

static int inited = false;

static double
oneBasis(double u, double v) {
    return 1;
}

static double (*f[1])(double, double) = {
    oneBasis
};

GalerkinBasis GLOBAL_stochasticRadiosity_clusterBasis = {
    "cluster basis",
    1,
    f,
    f
};

ApproximationTypeDescription GLOBAL_stochasticRadiosity_approxDesc[NR_APPROX_TYPES] = {
    {"constant",  1},
    {"linear",    3},
    {"bilinear",  4},
    {"quadratic", 6},
    {"cubic",     10}
};

static GalerkinBasis MakeBasis(ElementType et, StochasticRaytracingApproximation at) {
    GalerkinBasis basis = GLOBAL_stochasticRadiosity_quadBasis;
    char desc[100];
    const char *elem = nullptr;

    switch ( et ) {
        case ET_TRIANGLE:
            basis = GLOBAL_stochasticRadiosity_triBasis;
            elem = "triangles";
            break;
        case ET_QUAD:
            basis = GLOBAL_stochasticRadiosity_quadBasis;
            elem = "quadrilaterals";
            break;
        default:
            logFatal(-1, "MakeBasis", "Invalid element type %d", et);
    }

    basis.size = GLOBAL_stochasticRadiosity_approxDesc[at].basis_size;

    snprintf(desc, 100, "%s orthonormal basis for %s", GLOBAL_stochasticRadiosity_approxDesc[at].name, elem);
    basis.description = strdup(desc);

    return basis;
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
static void
computeFilterCoefficients(
        GalerkinBasis *parent_basis,
        int parent_size,
        GalerkinBasis *child_basis,
        int child_size,
        Matrix2x2 *upxfm,
        CubatureRule *cr,
        FILTER *filter)
{
    for ( int a = 0; a < parent_size; a++ ) {
        for ( int b = 0; b < child_size; b++ ) {
            double x = 0.0;
            for ( int k = 0; k < cr->numberOfNodes; k++ ) {
                Vector2D up;
                up.u = cr->u[k];
                up.v = cr->v[k];
                upxfm->transformPoint2D(up, up);
                x += cr->w[k] * parent_basis->function[a](up.u, up.v) *
                     child_basis->function[b](cr->u[k], cr->v[k]);
            }
            (*filter)[a][b] = x;
        }
    }
}

/**
Computes the push-pull filter coefficients for regular subdivision for
elements with given basis and uptransform. The cubature rule 'cr' is used
to compute the coefficients. The coefficients are filled in the
basis->regular_filter table
*/
static void
basisGalerkinComputeRegularFilterCoefficients(
        GalerkinBasis *basis,
        Matrix2x2 *upxfm,
        CubatureRule *cr)
{
    for ( int s = 0; s < 4; s++ ) {
        computeFilterCoefficients(
            basis,
            basis->size,
            basis,
            basis->size,
            &upxfm[s],
            cr,
            &(*basis->regularFilter)[s]);
    }
}

/**
Initialises table of bases
*/
void
monteCarloRadiosityInitBasis() {
    int et;
    int at;

    if ( inited ) {
        return;
    }

    basisGalerkinComputeRegularFilterCoefficients(&GLOBAL_stochasticRadiosity_triBasis, GLOBAL_stochasticRaytracing_triangleUpTransform, &GLOBAL_crt8);
    basisGalerkinComputeRegularFilterCoefficients(&GLOBAL_stochasticRadiosity_quadBasis, GLOBAL_stochasticRaytracing_quadUpTransform, &GLOBAL_crq8);

    for ( et = 0; et < NR_ELEMENT_TYPES; et++ ) {
        for ( at = 0; at < NR_APPROX_TYPES; at++ )
            GLOBAL_stochasticRadiosity_basis[et][at] = MakeBasis((ElementType) et, (StochasticRaytracingApproximation) at);
    }
    inited = true;
}

/**
Returns color at a given point, with parameters (u,v)
*/
ColorRgb
colorAtUv(GalerkinBasis *basis, ColorRgb *rad, double u, double v) {
    int i;
    ColorRgb res;
    res.clear();
    for ( i = 0; i < basis->size; i++ ) {
        double s = basis->function[i](u, v);
        res.addScaled(res, s, rad[i]);
    }
    return res;
}

/**
These routine filter the source coefficients down/up and add
the result to the destination coefficients
*/
void
filterColorDown(ColorRgb *parent, FILTER *h, ColorRgb *child, int n) {
    for ( int i = 0; i < n; i++ ) {
        for ( int j = 0; j < n; j++ ) {
            child[i].addScaled(child[i], (*h)[j][i], parent[j]);
        }
    }
}

void
filterColorUp(ColorRgb *child, FILTER *h, ColorRgb *parent, int n, double areaFactor) {
    for ( int i = 0; i < n; i++ ) {
        for ( int j = 0; j < n; j++ ) {
            double H = (*h)[i][j] * areaFactor;
            parent[i].addScaled(parent[i], H, child[j]);
        }
    }
}

#endif
