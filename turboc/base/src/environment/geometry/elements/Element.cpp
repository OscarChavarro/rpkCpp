#include "java/util/ArrayList.txx"
#include "common/logging/Logger.h"
#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#endif

#include "environment/geometry/elements/Element.h"

Element::Element():
    id(),
    Ed(),
    Rd(),
    radiance(),
    receivedRadiance(),
    unShotRadiance(),
    area(),
    className(),
    parent(),
    regularSubElements(),
    irregularSubElements(),
    transformToParent()
{
    flags = 0x00;
    Ed = ColorRgbMutable(0.0f, 0.0f, 0.0f);
    Rd = ColorRgbMutable(0.0f, 0.0f, 0.0f);
}

/**
Computes the transform relating a surface element to the toplevel element
in the patch hierarchy by concatenating the up-transforms of the element
and all parent elements. If the element is a toplevel element,
(Matrix4x4 *)NULL is
returned and nothing is filled in in xf (no transform is necessary
to transform positions on the element to the corresponding point on the toplevel
element). In the other case, the composed transform is filled in in xf and
xf (pointer to the transform) is returned
*/
Matrix2x2 *
Element::topTransform(Matrix2x2 *transform) const {
    // Top level element: no transform necessary to transform to top
    if ( !transformToParent ) {
        return NULL;
    }

    const Element *window = this;
    *transform = *window->transformToParent;
    do {
        window = window->parent;
        if ( window != NULL && window->transformToParent != NULL ) {
            window->transformToParent->matrix2DPreConcatTransform(*transform, *transform);
        }
    } while ( window != NULL && window->transformToParent );

    return transform;
}

/**
Call traversalCallbackFunction for each leaf element of element
*/
void
Element::traverseAllLeafElements(void (*traversalCallbackFunction)(Element *) ) {
    for ( int i = 0; irregularSubElements != NULL && i < irregularSubElements->size(); i++ ) {
        irregularSubElements->get(i)->traverseAllLeafElements(traversalCallbackFunction);
    }

    if ( regularSubElements != NULL ) {
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
        for ( int i = 0; irregularSubElements != NULL && i < irregularSubElements->size(); i++ ) {
            if ( irregularSubElements->get(i) != NULL ) {
                irregularSubElements->get(i)->traverseClusterLeafElements(traversalCallbackFunction);
            }
        }
    } else if ( regularSubElements != NULL ) {
        if ( regularSubElements != NULL ) {
            for ( int i = 0; i < 4; i++ ) {
                if ( regularSubElements[i] != NULL ) {
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
    if ( regularSubElements == NULL ) {
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
    return regularSubElements == NULL && (irregularSubElements == NULL || irregularSubElements->size() == 0);
}

Element *
Element::childContainingElement(Element *descendant) {
    while ( descendant != NULL && descendant->parent != this ) {
        descendant = descendant->parent;
    }
    if ( descendant == NULL ) {
        Logger::fatal(-1, "Element::childContainingElement", "descendant is not a descendant of parent");
    }
    return descendant;
}

/**
Returns true if there are children elements and false if top is NULL or a leaf element
*/
bool
Element::traverseAllChildren(void (*traversalCallbackFunction)(Element *)) const {
    if ( isCluster() ) {
        for ( int i = 0; irregularSubElements != NULL && i < irregularSubElements->size(); i++ ) {
            if ( irregularSubElements != NULL ) {
                traversalCallbackFunction(irregularSubElements->get(i));
            }
        }
        return true;
    } else if ( regularSubElements ) {
        if ( regularSubElements != NULL ) {
            for ( int i = 0; i < 4; i++ ) {
                if ( regularSubElements[i] != NULL ) {
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
