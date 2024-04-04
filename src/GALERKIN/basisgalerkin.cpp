/**
Higher order approximations for Galerkin radiosity
*/

#include "common/error.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/clustergalerkincpp.h"

/**
Pulls radiance up: reverse of the above: the radiance coefficients on the
child element are given, determine the radiance coefficients on the parent
element.

With the notations of above:

L_\alpha = \sum_\sigma {A^\sigma \over A}
		\sum_\beta L_{\beta,\sigma} H_{\alpha,\beta}^\sigma

For regular subdivision, the ratio of the area of the child element and
the area of the parent element is always 0.25 for surface elements and
regular (quadtree) subdivision.
*/
static void
basisGalerkinPull(
    GalerkinElement *parent,
    ColorRgb *parent_coefficients,
    GalerkinElement *child,
    ColorRgb *child_coefficients)
{
    GalerkinBasis *basis;
    int alpha;
    int beta;
    int sigma = (unsigned char)child->childNumber;

    if ( parent->isCluster() ) {
        // Clusters only have irregular sub-elements and a constant
        // radiance approximation is used on them
        clusterGalerkinClearCoefficients(parent_coefficients, parent->basisSize);
        colorScale(child->area / parent->area, child_coefficients[0], parent_coefficients[0]);
    } else {
        if ( sigma < 0 || sigma > 3 ) {
            logError("stochasticJacobiPull", "Not yet implemented for non-regular subdivision");
            clusterGalerkinClearCoefficients(parent_coefficients, parent->basisSize);
            parent_coefficients[0] = child_coefficients[0];
            return;
        }

        // Parent and child basis should be the same
        basis = child->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_triBasis : &GLOBAL_galerkin_quadBasis;
        for ( alpha = 0; alpha < parent->basisSize; alpha++ ) {
            colorClear(parent_coefficients[alpha]);
            for ( beta = 0; beta < child->basisSize; beta++ ) {
                double f = basis->regular_filter[sigma][alpha][beta];
                if ( f < -EPSILON || f > EPSILON )
                    colorAddScaled(parent_coefficients[alpha],
                                   (float)f,
                                   child_coefficients[beta],
                                   parent_coefficients[alpha]);
            }
            colorScale(0.25, parent_coefficients[alpha], parent_coefficients[alpha]);
        }
    }
}

/**
Modifies Bdown!
*/
static void
basisGalerkinPushPullRadianceRecursive(GalerkinElement *element, ColorRgb *Bdown, ColorRgb *Bup) {
    // Re-normalize the received radiance at this level and add to Bdown
    for ( int i = 0; i < element->basisSize; i++ ) {
        colorAddScaled(Bdown[i], 1.0f / element->area, element->receivedRadiance[i], Bdown[i]);
        colorClear(element->receivedRadiance[i]);
    }

    clusterGalerkinClearCoefficients(Bup, element->basisSize);

    if ( !element->regularSubElements && !element->irregularSubElements ) {
        // Leaf-element, multiply with reflectivity at the lowest level
        ColorRgb rho = element->patch->radianceData->Rd;
        for ( int i = 0; i < element->basisSize; i++ ) {
            colorProduct(rho, Bdown[i], Bup[i]);
        }

        if ( GLOBAL_galerkin_state.iteration_method == JACOBI || GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ) {
            // Add self-emitted radiance. Bup is a new approximation of the total radiance
            // add this leaf element
            ColorRgb Ed = element->patch->radianceData->Ed;
            colorAdd(Bup[0], Ed, Bup[0]);
        }
    }

    if ( element->regularSubElements != nullptr ) {
        // Regularly subdivided surface element
        for ( int i = 0; i < 4; i++ ) {
            ColorRgb Btmp[MAX_BASIS_SIZE];
            ColorRgb Bdown2[MAX_BASIS_SIZE];
            ColorRgb Bup2[MAX_BASIS_SIZE];

            // 1. Push B-down to the i-th sub-element
            basisGalerkinPush((GalerkinElement *)element, Bdown, (GalerkinElement *)element->regularSubElements[i], Bdown2);

            // 2. Recursive call the push-pull for the sub-element
            basisGalerkinPushPullRadianceRecursive((GalerkinElement *)element->regularSubElements[i], Bdown2, Btmp);

            // 3. Pull the radiance of the sub-element up to this level again
            basisGalerkinPull((GalerkinElement *)element, Bup2, (GalerkinElement *)element->regularSubElements[i], Btmp);

            // 4. Add to Bup
            clusterGalerkinAddCoefficients(Bup, Bup2, element->basisSize);
        }
    }

    if ( element->irregularSubElements ) {
        // A cluster or irregularly subdivided surface element
        for ( int i = 0; element->irregularSubElements != nullptr && i < element->irregularSubElements->size(); i++ ) {
            GalerkinElement *subElement = (GalerkinElement *)element->irregularSubElements->get(i);
            ColorRgb Btmp[MAX_BASIS_SIZE];
            ColorRgb Bdown2[MAX_BASIS_SIZE];
            ColorRgb Bup2[MAX_BASIS_SIZE];

            // 1. Push Bdown to the sub-element if a cluster (don't push to irregular
            // surface sub-elements)
            if ( element->isCluster() ) {
                basisGalerkinPush(element, Bdown, subElement, Bdown2);
            } else {
                clusterGalerkinClearCoefficients(Bdown2, element->basisSize);
            }

            // 2. Recursive call the push-pull for the sub-element
            basisGalerkinPushPullRadianceRecursive(subElement, Bdown2, Btmp);

            // 3. Pull the radiance of the sub-element up to this level again
            basisGalerkinPull(element, Bup2, subElement, Btmp);

            // 4. Add to Bup
            clusterGalerkinAddCoefficients(Bup, Bup2, element->basisSize);
        }
    }

    if ( GLOBAL_galerkin_state.iteration_method == JACOBI || GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ) {
        // Gathering method: Bup is new approximation of the total radiance at this level of detail
        clusterGalerkinCopyCoefficients(element->radiance, Bup, element->basisSize);
    } else {
        // Shooting: add Bup to the total and un-shot radiance at this level
        clusterGalerkinAddCoefficients(element->radiance, Bup, element->basisSize);
        clusterGalerkinAddCoefficients(element->unShotRadiance, Bup, element->basisSize);
    }
}

/**
Computes the filter coefficients for push-pull operations between a
parent and child with given basis and nr of basis functions. 'up transform' is
the transform to be used to find the point on the parent corresponding
to a given point on the child. 'cr' is the cubature rule to be used
for computing the coefficients. The order should be at least the highest
product of the order of a parent and a child basis function. The filter
coefficients are filled in in the table 'filter'. The filter coefficients are:

H_{\alpha\,\beta} = int _S phi_\alpha(u',v') phi_\beta(u,v) du dv

with S the domain on which the basis functions are defined (unit square or
standard triangle), and (u', v') the result of "up-transforming" (u, v).
*/
static void
basisGalerkinComputeFilterCoefficients(
    GalerkinBasis *parent_basis,
    int parent_size,
    GalerkinBasis *child_basis,
    int child_size,
    Matrix2x2 *upTransform,
    CUBARULE *cr,
    double filter[MAX_BASIS_SIZE][MAX_BASIS_SIZE])
{
    int alpha;
    int beta;
    int k;
    double x;

    for ( alpha = 0; alpha < parent_size; alpha++ ) {
        for ( beta = 0; beta < child_size; beta++ ) {
            x = 0.0;
            for ( k = 0; k < cr->numberOfNodes; k++ ) {
                Vector2D up;
                up.u = (float)cr->u[k];
                up.v = (float)cr->v[k];
                transformPoint2D((*upTransform), up, up);
                x += cr->w[k] * parent_basis->function[alpha](up.u, up.v) *
                     child_basis->function[beta](cr->u[k], cr->v[k]);
            }
            filter[alpha][beta] = x;
        }
    }
}

/**
Computes the push-pull filter coefficients for regular subdivision for
elements with given basis and up transform. The cubature rule 'cr' is used
to compute the coefficients. The coefficients are filled in the
basis->regular_filter table
*/
static void
basisGalerkinComputeRegularFilterCoefficients(
    GalerkinBasis *basis,
    Matrix2x2 *upTransform,
    CUBARULE *cr)
{
    for ( int sigma = 0; sigma < 4; sigma++ ) {
        basisGalerkinComputeFilterCoefficients(
            basis,
            basis->size,
            basis,
            basis->size,
            &upTransform[sigma],
            cr,
            basis->regular_filter[sigma]);
    }
}

/**
Given the radiance coefficients, this routine computes the radiance
at the given point on the element
*/
ColorRgb
basisGalerkinRadianceAtPoint(
    GalerkinElement *elem,
    ColorRgb *coefficients,
    double u,
    double v)
{
    ColorRgb rad;
    GalerkinBasis *basis = elem->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_triBasis : &GLOBAL_galerkin_quadBasis;

    colorClear(rad);
    if ( !coefficients ) {
        return rad;
    }

    for ( int i = 0; i < elem->basisSize; i++ ) {
        double f = basis->function[i](u, v);
        colorAddScaled(rad, (float)f, coefficients[i], rad);
    }

    return rad;
}

/**
Pushes radiance down in the element hierarchy from the parent element to the
child element.

Be x a point in 3D space located on the child element \sigma. x is also located
on the parent element. Then, the radiance L(x) at x on the parent element
can be written as \sum _\alpha L_\alpha \phi_\alpha(x), with L_\alpha the
radiance coefficients (given) on the parent element and \phi_\alpha the basis
functions on the parent. The same radiance is also approximated as
\sum_\beta L_\beta^\sigma \phi_\beta^\sigma(x) with L_\beta^\sigma the radiance
coefficients on the child element (to be determined) and \phi_\beta^\sigma the
basis functions on the sub-element (index \sigma).

The radiance coefficients L_\beta^\sigma on the child element are

 L_\beta^\sigma = \sum _\alpha L_\alpha H_{\alpha,\beta}^\sigma

with

H_{\alpha,\beta}^\sigma = {1\over A^\sigma} \times
	\int _{A^\sigma} \phi_\beta^\sigma(x) \phi_\alpha(x) dA_x.

The H_{\alpha,\beta}^\sigma coefficients are precomputed once for all
regular sub-elements. They depend only on the type of domain, the basis functions,
and the up transforms to relate sub-elements with the parent element
*/
void
basisGalerkinPush(
    GalerkinElement *element,
    ColorRgb *parent_coefficients,
    GalerkinElement *child,
    ColorRgb *child_coefficients)
{
    GalerkinBasis *basis;
    int alpha;
    int beta;
    int sigma = (unsigned char)child->childNumber;

    if ( element->isCluster() ) {
        // Clusters have only irregular sub-elements and a constant
        // approximation is used on them
        clusterGalerkinClearCoefficients(child_coefficients, child->basisSize);
        child_coefficients[0] = parent_coefficients[0];
    } else {
        if ( sigma < 0 || sigma > 3 ) {
            logError("stochasticJacobiPush", "Not yet implemented for non-regular subdivision");
            clusterGalerkinClearCoefficients(child_coefficients, child->basisSize);
            child_coefficients[0] = parent_coefficients[0];
            return;
        }

        // Parent and child basis should be the same
        basis = child->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_triBasis : &GLOBAL_galerkin_quadBasis;
        for ( beta = 0; beta < child->basisSize; beta++ ) {
            colorClear(child_coefficients[beta]);
            for ( alpha = 0; alpha < element->basisSize; alpha++ ) {
                double f = basis->regular_filter[sigma][alpha][beta];
                if ( f < -EPSILON || f > EPSILON )
                    colorAddScaled(child_coefficients[beta],
                                   (float)f,
                                   parent_coefficients[alpha],
                                   child_coefficients[beta]);
            }
        }
    }
}

/**
Converts the received radiance of a patch into exit
radiance, making a consistent hierarchical representation
*/
void
basisGalerkinPushPullRadiance(GalerkinElement *top) {
    ColorRgb Bdown[MAX_BASIS_SIZE];
    ColorRgb Bup[MAX_BASIS_SIZE];
    clusterGalerkinClearCoefficients(Bdown, top->basisSize);
    basisGalerkinPushPullRadianceRecursive(top, Bdown, Bup);
}

void
basisGalerkinInitBasis() {
    basisGalerkinComputeRegularFilterCoefficients(&GLOBAL_galerkin_quadBasis, GLOBAL_galerkin_QuadUpTransformMatrix, &GLOBAL_crq8);
    basisGalerkinComputeRegularFilterCoefficients(&GLOBAL_galerkin_triBasis, GLOBAL_galerkin_TriangularUpTransformMatrix, &GLOBAL_crt8);
}
