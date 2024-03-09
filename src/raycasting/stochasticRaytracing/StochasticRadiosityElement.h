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
    niedindex rayIndex; // Incremented each time a ray is shot from the element
    float quality; // For merging the result of multiple iterations
    float samplingProbability;
    float ng; // Number of samples gathered on the patch

    GalerkinBasis *basis; // Radiosity approximation data
        // Higher order approximations need an array of color values for representing radiance
    COLOR sourceRad; // Always constant source radiosity

    float importance; // For view-importance driven sampling
    float unShotImportance;
    float receivedImportance;
    float sourceImportance;
    niedindex importanceRayIndex; // Ray index for importance propagation

    Vector3D midPoint;
    Vertex *vertices[4]; // Up to 4 vertex pointers for surface elements
    signed char childNumber; // -1 for clusters or toplevel surface elements, 0..3 for regular surface sub-elements
    char numberOfVertices; // Number of surface element vertices
    bool isClusterFlag; // Whether it is a cluster or not

    StochasticRadiosityElement();
    ~StochasticRadiosityElement();
};

#endif
