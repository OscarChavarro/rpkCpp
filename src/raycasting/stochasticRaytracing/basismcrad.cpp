#include <cstring>

#include "common/error.h"
#include "common/cubature.h"
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

static GalerkinBasis MakeBasis(ElementType et, APPROX_TYPE at) {
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
    CUBARULE *cr,
    FILTER *filter)
{
    for ( int a = 0; a < parent_size; a++ ) {
        for ( int b = 0; b < child_size; b++ ) {
            double x = 0.0;
            for ( int k = 0; k < cr->numberOfNodes; k++ ) {
                Vector2D up;
                up.u = cr->u[k];
                up.v = cr->v[k];
                transformPoint2D((*upxfm), up, up);
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
    CUBARULE *cr)
{
    for ( int s = 0; s < 4; s++ ) {
        computeFilterCoefficients(
            basis,
            basis->size,
            basis,
            basis->size,
            &upxfm[s],
            cr,
            &(*basis->regular_filter)[s]);
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
            GLOBAL_stochasticRadiosity_basis[et][at] = MakeBasis((ElementType) et, (APPROX_TYPE) at);
    }
    inited = true;
}

/**
Returns color at a given point, with parameters (u,v)
*/
COLOR
colorAtUv(GalerkinBasis *basis, COLOR *rad, double u, double v) {
    int i;
    COLOR res;
    colorClear(res);
    for ( i = 0; i < basis->size; i++ ) {
        double s = basis->function[i](u, v);
        colorAddScaled(res, s, rad[i], res);
    }
    return res;
}

/**
These routine filter the source coefficients down/up and add
the result to the destination coefficients
*/
void
filterColorDown(COLOR *parent, FILTER *h, COLOR *child, int n) {
    for ( int b = 0; b < n; b++ ) {
        for ( int a = 0; a < n; a++ ) {
            colorAddScaled(child[b], (*h)[a][b], parent[a], child[b]);
        }
    }
}

void
filterColorUp(COLOR *child, FILTER *h, COLOR *parent, int n, double areaFactor) {
    for ( int a = 0; a < n; a++ ) {
        for ( int b = 0; b < n; b++ ) {
            double H = (*h)[a][b] * areaFactor;
            colorAddScaled(parent[a], H, child[b], parent[a]);
        }
    }
}
