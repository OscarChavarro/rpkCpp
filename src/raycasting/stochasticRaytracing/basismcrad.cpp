#include <cstdio>
#include <cstring>

#include "common/error.h"
#include "shared/cubature.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"
#include "raycasting/stochasticRaytracing/elementmcrad.h"

GalerkinBasis basis[NR_ELEMENT_TYPES][NR_APPROX_TYPES];
static int inited = false;

GalerkinBasis dummyBasis = {
        "dummy basis",
        0, nullptr, nullptr
};

static double c(double u, double v) {
    return 1;
}

static double (*f[1])(double, double) = {c};

GalerkinBasis clusterBasis = {
        "cluster basis",
        1, f, f
};

APPROXDESC approxdesc[NR_APPROX_TYPES] = {
        {"constant",  1},
        {"linear",    3},
        {"bilinear",  4},
        {"quadratic", 6},
        {"cubic",     10}
};

static GalerkinBasis MakeBasis(ELEMENT_TYPE et, APPROX_TYPE at) {
    GalerkinBasis basis = mcr_quadBasis;
    char desc[100];
    const char *elem = nullptr;

    switch ( et ) {
        case ET_TRIANGLE:
            basis = mcr_triBasis;
            elem = "triangles";
            break;
        case ET_QUAD:
            basis = mcr_quadBasis;
            elem = "quadrilaterals";
            break;
        default:
            logFatal(-1, "MakeBasis", "Invalid element type %d", et);
    }

    basis.size = approxdesc[at].basis_size;

    sprintf(desc, "%s orthonormal basis for %s", approxdesc[at].name, elem);
    basis.description = strdup(desc);

    return basis;
}

void PrintBasis(GalerkinBasis *basis) {
    int i;
    double u = 0.5, v = 0.5;

    fprintf(stderr, "%s, size=%d, samples at (%g,%g): ",
            basis->description, basis->size, u, v);
    for ( i = 0; i < basis->size; i++ ) {
        fprintf(stderr, "%g ", basis->function[i](u, v));
    }
    fprintf(stderr, "\n");
}

/* Computes the filter coefficients for push-pull operations between a
 * parent and child with given basis and nr of basis functions. 'upxfm' is
 * the transform to be used to find the point on the parent corresponding
 * to a given point on the child. 'cr' is the cubature rule to be used 
 * for computing the coefficients. The order should be at least the highest
 * product of the order of a parent and a child basis function. The filter
 * coefficients are filled in in the table 'filter'. The filter coefficients are:
 *
 * H_{\alpha\,\beta} = int _S phi_\alpha(u',v') phi_\beta(u,v) du dv
 *
 * with S the domain on which the basis functions are defined (unit square or
 * standard triangle), and (u',v') the result of "up-transforming" (u,v).
 */
static void ComputeFilterCoefficients(GalerkinBasis *parent_basis, int parent_size,
                                      GalerkinBasis *child_basis, int child_size,
                                      Matrix2x2 *upxfm, CUBARULE *cr,
                                      FILTER *filter) {
    int a, b, k;
    double x;

    for ( a = 0; a < parent_size; a++ ) {
        for ( b = 0; b < child_size; b++ ) {
            x = 0.;
            for ( k = 0; k < cr->nrnodes; k++ ) {
                Vector2D up;
                up.u = cr->u[k];
                up.v = cr->v[k];
                TRANSFORM_POINT_2D((*upxfm), up, up);
                x += cr->w[k] * parent_basis->function[a](up.u, up.v) *
                     child_basis->function[b](cr->u[k], cr->v[k]);
            }
            (*filter)[a][b] = x;
        }
    }
}

/* Computes the push-pull filter coefficients for regular subdivision for
 * elements with given basis and uptransform. The cubature rule 'cr' is used
 * to compute the coefficients. The coefficients are filled in the
 * basis->regular_filter table. */
static void basisGalerkinComputeRegularFilterCoefficients(GalerkinBasis *basis, Matrix2x2 *upxfm,
                                             CUBARULE *cr) {
    int s;

    for ( s = 0; s < 4; s++ ) {
        ComputeFilterCoefficients(basis, basis->size, basis, basis->size,
                                  &upxfm[s], cr, &(*basis->regular_filter)[s]);
    }
}

void
monteCarloRadiosityInitBasis() {
    int et, at;

    if ( inited ) {
        return;
    }

    basisGalerkinComputeRegularFilterCoefficients(&mcr_triBasis, GLOBAL_stochasticRaytracing_triupxfm, &CRT8);
    basisGalerkinComputeRegularFilterCoefficients(&mcr_quadBasis, GLOBAL_stochasticRaytracing_quadupxfm, &CRQ8);

    for ( et = 0; et < NR_ELEMENT_TYPES; et++ ) {
        for ( at = 0; at < NR_APPROX_TYPES; at++ )
            basis[et][at] = MakeBasis((ELEMENT_TYPE) et, (APPROX_TYPE) at);
    }
    inited = true;
}

COLOR ColorAtUV(GalerkinBasis *basis, COLOR *rad, double u, double v) {
    int i;
    COLOR res;
    colorClear(res);
    for ( i = 0; i < basis->size; i++ ) {
        double s = basis->function[i](u, v);
        colorAddScaled(res, s, rad[i], res);
    }
    return res;
}

/* these routine filter the source coefficients down/up and add
 * the result to the destination coefficients. */
void FilterColorDown(COLOR *parent, FILTER *h, COLOR *child, int n) {
    int a, b;

    for ( b = 0; b < n; b++ ) {
        for ( a = 0; a < n; a++ ) {
            colorAddScaled(child[b], (*h)[a][b], parent[a], child[b]);
        }
    }
}

void FilterColorUp(COLOR *child, FILTER *h, COLOR *parent, int n, double areafactor) {
    int a, b;

    for ( a = 0; a < n; a++ ) {
        for ( b = 0; b < n; b++ ) {
            double H = (*h)[a][b] * areafactor;
            colorAddScaled(parent[a], H, child[b], parent[a]);
        }
    }
}
