/* elementtype.h: Monte Carlo radiosity element type */

#ifndef _ELEMENT_TYPE_H_
#define _ELEMENT_TYPE_H_

#include "common/linealAlgebra/Matrix2x2.h"
#include "skin/Element.h"
#include "QMC/niederreiter.h"
#include "raycasting/stochasticRaytracing/elementlistmcrad.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"

class StochasticRadiosityElement : public Element {
  public:
    niedindex ray_index; // Incremented each time a ray is shot from the elem
    float quality; // For merging the result of multiple iterations
    float prob; // Sampling probability
    float ng; // Number of samples gathered on the patch
    float area; // Area of all surfaces contained in the element

    GalerkinBasis *basis; // Radiosity approximation data, see basis.h
    // Higher order approximations need an array of color values for representing radiance
    COLOR *rad;
    COLOR *unShotRad;
    COLOR *receivedRad;
    COLOR sourceRad; // Always constant source radiosity

    float imp; // For view-importance driven sampling
    float unshot_imp;
    float received_imp;
    float source_imp;
    niedindex imp_ray_index; // Ray index for importance propagation

    Vector3D midpoint; // Element midpoint
    Vertex *vertex[4]; // Up to 4 vertex pointers for surface elements
    StochasticRadiosityElement *parent; // Parent element in hierarchy
    StochasticRadiosityElement **regularSubElements; // For surface elements with regular quadtree subdivision
    ELEMENTLIST *irregular_subelements; // Clusters
    Matrix2x2 *upTrans; // Relates surface element (u,v) coordinates to patch (u,v) coordinates
    signed char childNumber; // -1 for clusters or toplevel surface elements, 0..3 for regular surface sub-elements
    char numberOfVertices; // Number of surface element vertices
    char isCluster; // whether it is a cluster or not

    ElementTypes className;
    StochasticRadiosityElement() {
        className = ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY;
    };
};

#endif
