#include "common/error.h"
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
    flags = 0x0;
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
    do {
        window = window->parent;
        if ( window != nullptr && window->upTrans != nullptr ) {
            matrix2DPreConcatTransform(*window->upTrans, *xf, *xf);
        }
    } while ( window != nullptr && window->upTrans );

    return xf;
}

/**
Returns true if elem is a leaf element
*/
bool
Element::isLeaf() const {
    return regularSubElements == nullptr && (irregularSubElements == nullptr || irregularSubElements->size() == 0);
}

Element *
Element::childContainingElement(Element *descendant) {
    while ( descendant != nullptr && descendant->parent != this ) {
        descendant = descendant->parent;
    }
    if ( descendant == nullptr ) {
        logFatal(-1, "Element::childContainingElement", "descendant is not a descendant of parent");
    }
    return descendant;
}

/**
Call traversalCallbackFunction for each leaf element of element
*/
void
Element::traverseAllLeafElements(void (*traversalCallbackFunction)(Element *)) {
    for ( int i = 0; irregularSubElements != nullptr && i < irregularSubElements->size(); i++ ) {
        irregularSubElements->get(i)->traverseAllLeafElements(traversalCallbackFunction);
    }

    if ( regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            regularSubElements[i]->traverseAllLeafElements(traversalCallbackFunction);
        }
    }

    if ( !irregularSubElements && !regularSubElements ) {
        traversalCallbackFunction(this);
    }
}

void
Element::traverseClusterLeafElements(void (*traversalCallbackFunction)(Element *)) {
    if ( isCluster() ) {
        for ( int i = 0; irregularSubElements != nullptr && i < irregularSubElements->size(); i++ ) {
            if ( irregularSubElements->get(i) != nullptr ) {
                irregularSubElements->get(i)->traverseClusterLeafElements(traversalCallbackFunction);
            }
        }
    } else if ( regularSubElements != nullptr ) {
        if ( regularSubElements != nullptr ) {
            for ( int i = 0; i < 4; i++ ) {
                if ( regularSubElements[i] != nullptr ) {
                    regularSubElements[i]->traverseClusterLeafElements(traversalCallbackFunction);
                }
            }
        }
    } else {
        traversalCallbackFunction(this);
    }
}
