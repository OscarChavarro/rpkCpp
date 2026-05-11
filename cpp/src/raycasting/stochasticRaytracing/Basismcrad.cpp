#include "java/util/Formatter.h"
#include "material/RendererConfiguration.h"
#include "numericalAnalysis/QuadCubatureRule.h"
#include "numericalAnalysis/TriangleCubatureRule.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/Basistrimcrad.h"

#ifdef RAYTRACING_ENABLED

#include <cstring>

#include "common/logging/Logger.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingApproximation.h"

double
Basismcrad::oneBasis(double /*u*/, double /*v*/) {
    return 1;
}

StochasticRadiosityBasisState::BasisFunction StochasticRadiosityBasisState::oneBasisTable[1] = {
    Basismcrad::oneBasis
};

StochasticRadiosityBasisState::StochasticRadiosityBasisState():
    approxDesc(),
    basis(),
    triBasis(Basistrimcrad::createBasis()),
    quadBasis(StochasticRadiosityBasisState::stochasticRadiosityCreateQuadBasis()),
    dummyBasis(),
    clusterBasis(),
    quadUpTransform(),
    triangleUpTransform(),
    inited(false)
{
    approxDesc[0] = {"constant", 1};
    approxDesc[1] = {"linear", 3};
    approxDesc[2] = {"bilinear", 4};
    approxDesc[3] = {"quadratic", 6};
    approxDesc[4] = {"cubic", 10};

    dummyBasis = {
        "dummy basis",
        0,
        nullptr,
        nullptr,
        nullptr
    };

    clusterBasis = {
        "cluster basis",
        1,
        StochasticRadiosityBasisState::oneBasisTable,
        StochasticRadiosityBasisState::oneBasisTable,
        nullptr
    };

    quadUpTransform[0] = createTransform(0.5F, 0.0F, 0.0F, 0.5F, 0.0F, 0.0F);
    quadUpTransform[1] = createTransform(0.5F, 0.0F, 0.0F, 0.5F, 0.5F, 0.0F);
    quadUpTransform[2] = createTransform(0.5F, 0.0F, 0.0F, 0.5F, 0.0F, 0.5F);
    quadUpTransform[3] = createTransform(0.5F, 0.0F, 0.0F, 0.5F, 0.5F, 0.5F);

    triangleUpTransform[0] = createTransform(0.5F, 0.0F, 0.0F, 0.5F, 0.0F, 0.0F);
    triangleUpTransform[1] = createTransform(0.5F, 0.0F, 0.0F, 0.5F, 0.5F, 0.0F);
    triangleUpTransform[2] = createTransform(0.5F, 0.0F, 0.0F, 0.5F, 0.0F, 0.5F);
    triangleUpTransform[3] = createTransform(-0.5F, 0.0F, 0.0F, -0.5F, 0.5F, 0.5F);
}

void
StochasticRadiosityBasisState::setActiveState(StochasticRadiosityBasisState &state) {
    activeStatePtr() = &state;
}

StochasticRadiosityBasisState &
StochasticRadiosityBasisState::activeState() {
    StochasticRadiosityBasisState *state = activeStatePtr();
    if ( state == nullptr ) {
        Logger::fatal(-1, "StochasticRadiosityBasisState::activeState", "Stochastic radiosity basis state was not initialized");
    }
    return *state;
}

StochasticRadiosityBasisState *&
StochasticRadiosityBasisState::activeStatePtr() {
    static StochasticRadiosityBasisState *activeState = nullptr;
    return activeState;
}

Matrix2x2
StochasticRadiosityBasisState::createTransform(float m00, float m01, float m10, float m11, float t0, float t1) {
    Matrix2x2 transform{};
    transform.m[0][0] = m00;
    transform.m[0][1] = m01;
    transform.m[1][0] = m10;
    transform.m[1][1] = m11;
    transform.t[0] = t0;
    transform.t[1] = t1;
    return transform;
}

GalerkinBasis
Basismcrad::makeBasis(StochasticRadiosityElementType et, StochasticRaytracingApproximation at) {
    StochasticRadiosityBasisState &basisState = StochasticRadiosityBasisState::activeState();
    GalerkinBasis basis = basisState.quadBasis;
    char desc[100];
    const char *elem;

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
            Logger::fatal(-1, "Basismcrad::makeBasis", "Invalid element type %d", et);
    }

    basis.size = basisState.approxDesc[at].basis_size;

    java::Formatter::format(
        desc, 100, "%s orthonormal basis for %s", basisState.approxDesc[at].name, elem);
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
void
Basismcrad::computeFilterCoefficients(
    const GalerkinBasis *parent_basis,
    const int parent_size,
    const GalerkinBasis *child_basis,
    const int child_size,
    const Matrix2x2 *upxfm,
    const CubatureRule *cr,
    GalerkinBasis::FILTER *filter)
{
    for ( int a = 0; a < parent_size; a++ ) {
        for ( int b = 0; b < child_size; b++ ) {
            double x = 0.0;
            for ( int k = 0; k < cr->numberOfNodes; k++ ) {
                Vector2D up;
                up.u = static_cast<float>(cr->u[k]);
                up.v = static_cast<float>(cr->v[k]);
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
void
Basismcrad::basisGalerkinComputeRegularFilterCoefficients(
    GalerkinBasis *basis,
    const Matrix2x2 *upxfm,
    const CubatureRule *cr)
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
Basismcrad::monteCarloRadiosityInitBasis() {
    StochasticRadiosityBasisState &basisState = StochasticRadiosityBasisState::activeState();
    if ( basisState.inited ) {
        return;
    }

    basisGalerkinComputeRegularFilterCoefficients(
        &basisState.triBasis,
        basisState.triangleUpTransform,
        TriangleCubatureRule::degree8Rule());
    basisGalerkinComputeRegularFilterCoefficients(
        &basisState.quadBasis,
        basisState.quadUpTransform,
        QuadCubatureRule::degree8QuadrilateralRule());

    for ( int et = 0; et < StochasticRadiosityElementTypeInfo::NUMBER_OF_ELEMENT_TYPES; et++ ) {
        for ( int at = 0; at < StochasticRadiosityBasisState::NUMBER_OF_APPROXIMATION_TYPES; at++ )
            basisState.basis[et][at] = makeBasis(static_cast<StochasticRadiosityElementType>(et), static_cast<StochasticRaytracingApproximation>(at));
    }
    basisState.inited = true;
}

/**
Returns color at a given point, with parameters (u,v)
*/
ColorRgb
Basismcrad::colorAtUv(const GalerkinBasis *basis, const ColorRgb *rad, double u, double v) {
    ColorRgb res;
    res.clear();
    for ( int i = 0; i < basis->size; i++ ) {
        double s = basis->function[i](u, v);
        res.addScaled(res, static_cast<float>(s), rad[i]);
    }
    return res;
}

/**
These routine filter the source coefficients down/up and add
the result to the destination coefficients
*/
void
Basismcrad::filterColorDown(const ColorRgb *parent, GalerkinBasis::FILTER *h, ColorRgb *child, int n) {
    for ( int i = 0; i < n; i++ ) {
        for ( int j = 0; j < n; j++ ) {
            child[i].addScaled(child[i], static_cast<float>((*h)[j][i]), parent[j]);
        }
    }
}

void
Basismcrad::filterColorUp(const ColorRgb *child, GalerkinBasis::FILTER *h, ColorRgb *parent, int n, double areaFactor) {
    for ( int i = 0; i < n; i++ ) {
        for ( int j = 0; j < n; j++ ) {
            double H = (*h)[i][j] * areaFactor;
            parent[i].addScaled(parent[i], static_cast<float>(H), child[j]);
        }
    }
}

#endif
