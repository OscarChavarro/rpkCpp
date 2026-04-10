/**
Higher order approximations for Galerkin radiosity
*/

#include "java/util/ArrayList.txx"
#include "common/Error.h"
#include "galerkin/BasisQuadGalerkin.h"
#include "galerkin/BasisTriGalerkin.h"
#include "galerkin/GalerkinBasis.h"

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
GalerkinBasis::pull(
    const GalerkinElement *parent,
    ColorRgb *parentCoefficients,
    const GalerkinElement *child,
    const ColorRgb *childCoefficients)
{
    const GalerkinBasis *basis;
    int sigma = ((unsigned char)(child->childNumber));

    if ( parent->isCluster() ) {
        // Clusters only have irregular sub-elements and a constant
        // radiance approximation is used on them
        ColorRgb::arrayClear(parentCoefficients, parent->basisSize);
        parentCoefficients[0].scaledCopy(child->area / parent->area, childCoefficients[0]);
    } else {
        if ( sigma < 0 || sigma > 3 ) {
            Error::error("stochasticJacobiPull", "Not yet implemented for non-regular subdivision");
            ColorRgb::arrayClear(parentCoefficients, parent->basisSize);
            parentCoefficients[0] = childCoefficients[0];
            return;
        }

        // Parent and child basis should be the same
        basis = GalerkinBasis::basisForVertexCount(child->patch->numberOfVertices);
        for ( int alpha = 0; alpha < parent->basisSize; alpha++ ) {
            parentCoefficients[alpha].clear();
            for ( int beta = 0; beta < child->basisSize; beta++ ) {
                double f = basis->regularFilter[sigma][alpha][beta];
                if ( f < -Numeric::EPSILON || f > Numeric::EPSILON )
                    parentCoefficients[alpha].addScaled(
                            parentCoefficients[alpha],
                            ((float)(f)),
                            childCoefficients[beta]);
            }
            parentCoefficients[alpha].scale(0.25f);
        }
    }
}

/**
Modifies Bdown!
*/
void
GalerkinBasis::pushPullRadianceRecursive(
    GalerkinElement *element,
    ColorRgb *Bdown,
    ColorRgb *Bup,
    GalerkinState *galerkinState)
{
    // Re-normalize the received radiance at this level and add to Bdown
    for ( int i = 0; i < element->basisSize; i++ ) {
        Bdown[i].addScaled(Bdown[i], 1.0f / element->area, element->receivedRadiance[i]);
        element->receivedRadiance[i].clear();
    }

    ColorRgb::arrayClear(Bup, element->basisSize);

    if ( !element->regularSubElements && !element->irregularSubElements ) {
        // Leaf-element, multiply with reflectivity at the lowest level
        ColorRgb rho = element->patch->radianceData->Rd;
        for ( int i = 0; i < element->basisSize; i++ ) {
            Bup[i].scalarProduct(rho, Bdown[i]);
        }

        if ( galerkinState->galerkinIterationMethod == JACOBI
          || galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ) {
            // Add self-emitted radiance. Bup is a new approximation of the total radiance
            // add this leaf element
            ColorRgb Ed = element->patch->radianceData->Ed;
            Bup[0].add(Bup[0], Ed);
        }
    }

    if ( element->regularSubElements != NULL ) {
        // Regularly subdivided surface element
        for ( int i = 0; i < 4; i++ ) {
            ColorRgb Btmp[GALERKIN_MAX_BASIS_SIZE];
            ColorRgb Bdown2[GALERKIN_MAX_BASIS_SIZE];
            ColorRgb Bup2[GALERKIN_MAX_BASIS_SIZE];

            // 1. Push B-down to the i-th sub-element
            push(element, Bdown, ((GalerkinElement *)(element->regularSubElements[i])), Bdown2);

            // 2. Recursive call the push-pull for the sub-element
            pushPullRadianceRecursive(((GalerkinElement *)(element->regularSubElements[i])), Bdown2, Btmp, galerkinState);

            // 3. Pull the radiance of the sub-element up to this level again
            pull(element, Bup2, ((GalerkinElement *)(element->regularSubElements[i])), Btmp);

            // 4. Add to Bup
            ColorRgb::arrayAdd(Bup, Bup2, element->basisSize);
        }
    }

    if ( element->irregularSubElements ) {
        // A cluster or irregularly subdivided surface element
        for ( int i = 0; element->irregularSubElements != NULL && i < element->irregularSubElements->size(); i++ ) {
            GalerkinElement *subElement = ((GalerkinElement *)(element->irregularSubElements->get(i)));
            ColorRgb Btmp[GALERKIN_MAX_BASIS_SIZE];
            ColorRgb Bdown2[GALERKIN_MAX_BASIS_SIZE];
            ColorRgb Bup2[GALERKIN_MAX_BASIS_SIZE];

            // 1. Push Bdown to the sub-element if a cluster (don't push to irregular
            // surface sub-elements)
            if ( element->isCluster() ) {
                push(element, Bdown, subElement, Bdown2);
            } else {
                ColorRgb::arrayClear(Bdown2, element->basisSize);
            }

            // 2. Recursive call the push-pull for the sub-element
            pushPullRadianceRecursive(subElement, Bdown2, Btmp, galerkinState);

            // 3. Pull the radiance of the sub-element up to this level again
            pull(element, Bup2, subElement, Btmp);

            // 4. Add to Bup
            ColorRgb::arrayAdd(Bup, Bup2, element->basisSize);
        }
    }

    if ( galerkinState->galerkinIterationMethod == JACOBI
      || galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ) {
        // Gathering method: Bup is new approximation of the total radiance at this level of detail
        ColorRgb::arrayCopy(element->radiance, Bup, element->basisSize);
    } else {
        // Shooting: add Bup to the total and un-shot radiance at this level
        ColorRgb::arrayAdd(element->radiance, Bup, element->basisSize);
        ColorRgb::arrayAdd(element->unShotRadiance, Bup, element->basisSize);
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
void
GalerkinBasis::computeFilterCoefficients(
    const GalerkinBasis *parentBasis,
    const int parentSize,
    const GalerkinBasis *childBasis,
    const int childSize,
    const Matrix2x2 *upTransform,
    const CubatureRule *cubatureRule,
    double filter[GALERKIN_MAX_BASIS_SIZE][GALERKIN_MAX_BASIS_SIZE])
{
    double x;

    for ( int alpha = 0; alpha < parentSize; alpha++ ) {
        for ( int beta = 0; beta < childSize; beta++ ) {
            x = 0.0;
            for ( int k = 0; k < cubatureRule->numberOfNodes; k++ ) {
                Vector2D up;
                up.u = ((float)(cubatureRule->u[k]));
                up.v = ((float)(cubatureRule->v[k]));
                upTransform->transformPoint2D(up, up);
                x += cubatureRule->w[k] * parentBasis->function[alpha](up.u, up.v) *
                     childBasis->function[beta](cubatureRule->u[k], cubatureRule->v[k]);
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
void
GalerkinBasis::cmptRegFiltCoeff(
    GalerkinBasis *basis,
    const Matrix2x2 upTransform[],
    const CubatureRule *cubaRule)
{
    for ( int sigma = 0; sigma < 4; sigma++ ) {
        computeFilterCoefficients(
            basis,
            basis->size,
            basis,
            basis->size,
            &upTransform[sigma],
            cubaRule,
            basis->regularFilter[sigma]);
    }
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
GalerkinBasis::push(
    const GalerkinElement *element,
    const ColorRgb *parentCoefficients,
    const GalerkinElement *child,
    ColorRgb *childCoefficients)
{
    int sigma = ((unsigned char)(child->childNumber));

    if ( element->isCluster() ) {
        // Clusters have only irregular sub-elements and a constant
        // approximation is used on them
        ColorRgb::arrayClear(childCoefficients, child->basisSize);
        childCoefficients[0] = parentCoefficients[0];
    } else {
        if ( sigma < 0 || sigma > 3 ) {
            Error::error("stochasticJacobiPush", "Not yet implemented for non-regular subdivision");
            ColorRgb::arrayClear(childCoefficients, child->basisSize);
            childCoefficients[0] = parentCoefficients[0];
            return;
        }

        // Parent and child basis should be the same
        const GalerkinBasis *basis = GalerkinBasis::basisForVertexCount(child->patch->numberOfVertices);
        for ( int beta = 0; beta < child->basisSize; beta++ ) {
            childCoefficients[beta].clear();
            for ( int alpha = 0; alpha < element->basisSize; alpha++ ) {
                double f = basis->regularFilter[sigma][alpha][beta];
                if ( f < -Numeric::EPSILON || f > Numeric::EPSILON )
                    childCoefficients[beta].addScaled(
                        childCoefficients[beta],
                        ((float)(f)),
                        parentCoefficients[alpha]);
            }
        }
    }
}

/**
Converts the received radiance of a patch into exit
radiance, making a consistent hierarchical representation
*/
void
GalerkinBasis::pushPullRadiance(GalerkinElement *top, GalerkinState *galerkinState) {
    ColorRgb bDown[GALERKIN_MAX_BASIS_SIZE];
    ColorRgb Bup[GALERKIN_MAX_BASIS_SIZE];
    ColorRgb::arrayClear(bDown, top->basisSize);
    pushPullRadianceRecursive(top, bDown, Bup, galerkinState);
}

/**
Generic interpolator. Note that this traverses the needed terms to contribute for the interpolated
approximation of the solution over CONSTANT (1 term), LINEAR (3 terms), QUADRATIC (6 terms) or
CUBIC (10 terms) degree polynomial.
Given the radiance coefficients, this routine computes the radiance
at the given point on the element
*/
ColorRgb
GalerkinBasis::radianceAtPoint(
    const GalerkinElement *element,
    const ColorRgb *coefficients,
    const double u,
    const double v)
{
    const GalerkinBasis *basis = GalerkinBasis::basisForVertexCount(element->patch->numberOfVertices);

    ColorRgb rad;
    rad.clear();
    if ( coefficients == NULL ) {
        return rad;
    }

    for ( int i = 0; i < element->basisSize; i++ ) {
        float evaluatedTerm = ((float)(basis->function[i](u, v)));
        rad.addScaled(rad, evaluatedTerm, coefficients[i]);
    }

    return rad;
}

const GalerkinBasis *
GalerkinBasis::basisForVertexCount(const int numberOfVertices) {
    return numberOfVertices == 3 ? &BasisTriGalerkin::instance() : &BasisQuadGalerkin::instance();
}

GalerkinBasis *
GalerkinBasis::mutableBasisForVertexCount(const int numberOfVertices) {
    return numberOfVertices == 3 ? &BasisTriGalerkin::instance() : &BasisQuadGalerkin::instance();
}
