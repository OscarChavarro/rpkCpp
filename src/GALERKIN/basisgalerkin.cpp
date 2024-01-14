/**
Higher order approximations for Galerkin radiosity
*/

#include "common/error.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/clustergalerkincpp.h"

/**
Given the radiance coefficients, this routine computes the radiance
at the given point on the element
*/
COLOR
basisGalerkinRadianceAtPoint(GalerkingElement *elem, COLOR *coefficients, double u, double v) {
    COLOR rad;
    GalerkinBasis *basis = elem->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_triBasis : &GLOBAL_galerkin_quadBasis;
    int i;

    colorClear(rad);
    if ( !coefficients ) {
        return rad;
    }

    for ( i = 0; i < elem->basis_size; i++ ) {
        double f = basis->function[i](u, v);
        colorAddScaled(rad, f, coefficients[i], rad);
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
basis functions on the subelement (index \sigma).

The radiance coefficients L_\beta^\sigma on the child element are

 L_\beta^\sigma = \sum _\alpha L_\alpha H_{\alpha,\beta}^\sigma

with

H_{\alpha,\beta}^\sigma = {1\over A^\sigma} \times
	\int _{A^\sigma} \phi_\beta^\sigma(x) \phi_\alpha(x) dA_x.

The H_{\alpha,\beta}^\sigma coefficients are precomputed once for all
regular subelements. They depend only on the type of domain, the basis functions,
and the uptransforms to relate subelements with the parent element
*/
void
basisGalerkinPush(
        GalerkingElement *parent,
        COLOR *parent_coefficients,
        GalerkingElement *child,
        COLOR *child_coefficients)
{
    GalerkinBasis *basis;
    int alpha, beta, sigma = child->childnr;

    if ( isCluster(parent)) {
        /* clusters have only irregular subelements and a constant
         * aprroximation is used on them. */
        clusterGalerkinClearCoefficients(child_coefficients, child->basis_size);
        child_coefficients[0] = parent_coefficients[0];
    } else {
        if ( sigma < 0 || sigma > 3 ) {
            logError("Push", "Not yet implemented for non-regular subdivision");
            clusterGalerkinClearCoefficients(child_coefficients, child->basis_size);
            child_coefficients[0] = parent_coefficients[0];
            return;
        }

        /* Parent and child basis should be the same */
        basis = child->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_triBasis : &GLOBAL_galerkin_quadBasis;
        for ( beta = 0; beta < child->basis_size; beta++ ) {
            colorClear(child_coefficients[beta]);
            for ( alpha = 0; alpha < parent->basis_size; alpha++ ) {
                double f = basis->regular_filter[sigma][alpha][beta];
                if ( f < -EPSILON || f > EPSILON )
                    colorAddScaled(child_coefficients[beta], f,
                                   parent_coefficients[alpha],
                                   child_coefficients[beta]);
            }
        }
    }
}

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
void
basisGalerkinPull(
        GalerkingElement *parent,
        COLOR *parent_coefficients,
        GalerkingElement *child,
        COLOR *child_coefficients)
{
    GalerkinBasis *basis;
    int alpha, beta, sigma = child->childnr;

    if ( isCluster(parent)) {
        /* clusters only have irregular subelements and a constant
         * radiance approximation is used on them. */
        clusterGalerkinClearCoefficients(parent_coefficients, parent->basis_size);
        colorScale(child->area / parent->area, child_coefficients[0], parent_coefficients[0]);
    } else {
        if ( sigma < 0 || sigma > 3 ) {
            logError("Pull", "Not yet implemented for non-regular subdivision");
            clusterGalerkinClearCoefficients(parent_coefficients, parent->basis_size);
            parent_coefficients[0] = child_coefficients[0];
            return;
        }

        /* Parent and child basis should be the same */
        basis = child->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_triBasis : &GLOBAL_galerkin_quadBasis;
        for ( alpha = 0; alpha < parent->basis_size; alpha++ ) {
            colorClear(parent_coefficients[alpha]);
            for ( beta = 0; beta < child->basis_size; beta++ ) {
                double f = basis->regular_filter[sigma][alpha][beta];
                if ( f < -EPSILON || f > EPSILON )
                    colorAddScaled(parent_coefficients[alpha], f,
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
basisGalerkinPushPullRadianceRecursive(GalerkingElement *element, COLOR *Bdown, COLOR *Bup) {
    int i;

    // Re-normalize the received radiance at this level and add to Bdown
    for ( i = 0; i < element->basis_size; i++ ) {
        colorAddScaled(Bdown[i], 1.0f / element->area, element->received_radiance[i], Bdown[i]);
        colorClear(element->received_radiance[i]);
    }

    clusterGalerkinClearCoefficients(Bup, element->basis_size);

    if ( !element->regular_subelements && !element->irregular_subelements ) {
        // Leaf element
        // Multiply with reflectivity at the lowest level
        if ( element == nullptr || element->patch == nullptr ) {
            printf("Special case\n");
        }
        printf("  - Processing element %d\n", element->id);
        COLOR rho = REFLECTIVITY(element->patch);
        for ( i = 0; i < element->basis_size; i++ ) {
            colorProduct(rho, Bdown[i], Bup[i]);
        }

        if ( GLOBAL_galerkin_state.iteration_method == JACOBI || GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ) {
            // Add self-emitted radiance. Bup is a new approximation of the total radiance
            // add this leaf element
            COLOR Ed = SELFEMITTED_RADIANCE(element->patch);
            colorAdd(Bup[0], Ed, Bup[0]);
        }
    }

    if ( element->regular_subelements != nullptr ) {
        // Regularly subdivided surface element
        for ( i = 0; i < 4; i++ ) {
            COLOR Btmp[MAXBASISSIZE];
            COLOR Bdown2[MAXBASISSIZE];
            COLOR Bup2[MAXBASISSIZE];

            // 1. Push Bdown to the i-th sub-element
            basisGalerkinPush(element, Bdown, element->regular_subelements[i], Bdown2);

            // 2. Recursive call the push-pull for the sub-element
            basisGalerkinPushPullRadianceRecursive(element->regular_subelements[i], Bdown2, Btmp);

            // 3. Pull the radiance of the sub-element up to this level again
            basisGalerkinPull(element, Bup2, element->regular_subelements[i], Btmp);

            // 4) Add to Bup
            clusterGalerkinAddCoefficients(Bup, Bup2, element->basis_size);
        }
    }

    if ( element->irregular_subelements ) {
        // A cluster or irregularly subdivided surface element
        ELEMENTLIST *subElementList;
        for ( subElementList = element->irregular_subelements; subElementList; subElementList = subElementList->next ) {
            GalerkingElement *subElement = subElementList->element;
            COLOR Btmp[MAXBASISSIZE];
            COLOR Bdown2[MAXBASISSIZE];
            COLOR Bup2[MAXBASISSIZE];

            // 1. push Bdown to the sub-element if a cluster (don't push to irregular
            // surface sub-elements)
            if ( isCluster(element) ) {
                basisGalerkinPush(element, Bdown, subElement, Bdown2);
            } else {
                clusterGalerkinClearCoefficients(Bdown2, element->basis_size);
            }

            // 2. recursive call the push-pull for the sub-element
            basisGalerkinPushPullRadianceRecursive(subElement, Bdown2, Btmp);

            // 3. pull the radiance of the sub-element up to this level again
            basisGalerkinPull(element, Bup2, subElement, Btmp);

            // 4. add to Bup
            clusterGalerkinAddCoefficients(Bup, Bup2, element->basis_size);
        }
    }

    if ( GLOBAL_galerkin_state.iteration_method == JACOBI || GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ) {
        // Gathering method: Bup is new approximation of the total radiance
        // at this level of detail
        clusterGalerkinCopyCoefficients(element->radiance, Bup, element->basis_size);
    } else {
        // Shooting: add Bup to the total and un-shot radiance at this level
        clusterGalerkinAddCoefficients(element->radiance, Bup, element->basis_size);
        clusterGalerkinAddCoefficients(element->unshot_radiance, Bup, element->basis_size);
    }
}

void
basisGalerkinPushPullRadiance(GalerkingElement *topElement) {
    COLOR Bdown[MAXBASISSIZE], Bup[MAXBASISSIZE];
    clusterGalerkinClearCoefficients(Bdown, topElement->basis_size);
    basisGalerkinPushPullRadianceRecursive(topElement, Bdown, Bup);
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
basisGalerkinComputeFilterCoefficients(
    GalerkinBasis *parent_basis,
    int parent_size,
    GalerkinBasis *child_basis,
    int child_size,
    Matrix2x2 *upxfm,
    CUBARULE *cr,
    double filter[MAXBASISSIZE][MAXBASISSIZE])
{
    int alpha;
    int beta;
    int k;
    double x;

    for ( alpha = 0; alpha < parent_size; alpha++ ) {
        for ( beta = 0; beta < child_size; beta++ ) {
            x = 0.;
            for ( k = 0; k < cr->numberOfNodes; k++ ) {
                Vector2D up;
                up.u = cr->u[k];
                up.v = cr->v[k];
                TRANSFORM_POINT_2D((*upxfm), up, up);
                x += cr->w[k] * parent_basis->function[alpha](up.u, up.v) *
                     child_basis->function[beta](cr->u[k], cr->v[k]);
            }
            filter[alpha][beta] = x;
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
basisGalerkinComputeRegularFilterCoefficients(
    GalerkinBasis *basis,
    Matrix2x2 *upxfm,
    CUBARULE *cr)
{
    int sigma;

    for ( sigma = 0; sigma < 4; sigma++ ) {
        basisGalerkinComputeFilterCoefficients(basis, basis->size, basis, basis->size,
                                               &upxfm[sigma], cr, basis->regular_filter[sigma]);
    }
}

void
basisGalerkinInitBasis() {
    basisGalerkinComputeRegularFilterCoefficients(&GLOBAL_galerkin_quadBasis, globalQuadUpTransformMatrix, &GLOBAL_crq8);
    basisGalerkinComputeRegularFilterCoefficients(&GLOBAL_galerkin_triBasis, globalTriangularUpTransformMatrix, &GLOBAL_crt8);
}
