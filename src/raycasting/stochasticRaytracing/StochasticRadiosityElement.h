/**
Monte Carlo radiosity element type
*/

#ifndef __STOCHASTIC_RADIOSITY_ELEMENT__
#define __STOCHASTIC_RADIOSITY_ELEMENT__

#include "java/util/ArrayList.h"
#include "skin/Element.h"
#include "QMC/niederreiter.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"

class StochasticRadiosityElement : public Element {
  public:
    java::ArrayList<StochasticRadiosityElement *> *irregularSubElements;

    niedindex ray_index; // Incremented each time a ray is shot from the elem
    float quality; // For merging the result of multiple iterations
    float prob; // Sampling probability
    float ng; // Number of samples gathered on the patch

    GalerkinBasis *basis; // Radiosity approximation data, see basis.h
    // Higher order approximations need an array of color values for representing radiance
    COLOR sourceRad; // Always constant source radiosity

    float imp; // For view-importance driven sampling
    float unShotImp;
    float received_imp;
    float source_imp;
    niedindex imp_ray_index; // Ray index for importance propagation

    Vector3D midpoint; // Element midpoint
    Vertex *vertex[4]; // Up to 4 vertex pointers for surface elements
    signed char childNumber; // -1 for clusters or toplevel surface elements, 0..3 for regular surface sub-elements
    char numberOfVertices; // Number of surface element vertices
    char isCluster; // Whether it is a cluster or not

    StochasticRadiosityElement();
    ~StochasticRadiosityElement();
};

#endif
