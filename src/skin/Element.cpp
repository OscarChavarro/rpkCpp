#include "java/util/ArrayList.txx"
#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED
#include "common/error.h"
#endif

#include "skin/Element.h"

Element::Element():
        id(),
        Ed(),
        Rd(),
        radiance(),
        receivedRadiance(),
        unShotRadiance(),
        area(),
        className(),
        patch(),
        geometry(),
        parent(),
        regularSubElements(),
        irregularSubElements(),
        transformToParent()
{
    flags = 0x00;
    Ed.clear();
    Rd.clear();
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
Element::topTransform(Matrix2x2 *xf) const {
    // Top level element: no transform necessary to transform to top
    if ( !transformToParent ) {
        return nullptr;
    }

    const Element *window = this;
    *xf = *window->transformToParent;
    do {
        window = window->parent;
        if ( window != nullptr && window->transformToParent != nullptr ) {
            window->transformToParent->matrix2DPreConcatTransform(*xf, *xf);
        }
    } while ( window != nullptr && window->transformToParent );

    return xf;
}

/**
Call traversalCallbackFunction for each leaf element of element
*/
void
Element::traverseAllLeafElements(void (*traversalCallbackFunction)(Element *) ) {
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

void
Element::traverseQuadTreeLeafs(void (*traversalCallbackFunction)(Element *, const RenderOptions *), const RenderOptions *renderOptions)
{
    if ( regularSubElements == nullptr ) {
        // Trivial case
        traversalCallbackFunction(this, renderOptions);
    } else {
        // Recursive case
        for ( int i = 0; i < 4; i++ ) {
            regularSubElements[i]->traverseQuadTreeLeafs(traversalCallbackFunction, renderOptions);
        }
    }
}

#ifdef RAYTRACING_ENABLED

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
Returns true if there are children elements and false if top is nullptr or a leaf element
*/
bool
Element::traverseAllChildren(void (*traversalCallbackFunction)(Element *)) const {
    if ( isCluster() ) {
        for ( int i = 0; irregularSubElements != nullptr && i < irregularSubElements->size(); i++ ) {
            if ( irregularSubElements != nullptr ) {
                traversalCallbackFunction(irregularSubElements->get(i));
            }
        }
        return true;
    } else if ( regularSubElements ) {
        if ( regularSubElements != nullptr ) {
            for ( int i = 0; i < 4; i++ ) {
                if ( regularSubElements[i] != nullptr ) {
                    traversalCallbackFunction(regularSubElements[i]);
                }
            }
        }
        return true;
    } else {
        // Leaf element
        return false;
    }
}

#endif
