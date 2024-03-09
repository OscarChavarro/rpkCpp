#include "skin/Element.h"

Element::Element():
    id(),
    Ed(),
    Rd(),
    patch(),
    geometry(),
    radiance(),
    receivedRadiance(),
    unShotRadiance(),
    parent(),
    regularSubElements(),
    irregularSubElements(),
    upTrans(),
    area(),
    className()
{
}

/**
Computes the transform relating a surface element to the toplevel element
in the patch hierarchy by concatenating the up-transforms of the element
and all parent elements. If the element is a toplevel element,
(Matrix4x4 *)nullptr is
returned and nothing is filled in in xf (no transform is necessary
to transform positions on the element to the corresponding point on the toplevel
element). In the other case, the composed transform is filled in in xf and
xf (pointer to the transform) is returned
*/
Matrix2x2 *
Element::topTransform(Matrix2x2 *xf) {
    // Top level element: no transform necessary to transform to top
    if ( !upTrans ) {
        return nullptr;
    }

    Element *window = this;
    *xf = *window->upTrans;
    while ( (window = window->parent) && window->upTrans ) {
        matrix2DPreConcatTransform(*window->upTrans, *xf, *xf);
    }

    return xf;
}
