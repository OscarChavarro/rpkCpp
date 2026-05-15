/**
Galerkin finite elements: one structure for both surface and cluster elements
*/

#ifndef __GALERKIN_ELEMENT__
#define __GALERKIN_ELEMENT__

#include "java/lang/System.h"
#include "java/util/ArrayList.h"
#include "material/RendererConfiguration.h"
#include "scene/Polygon.h"
#include "galerkin/GalerkinElementRenderMode.h"
#include "galerkin/Interaction.h"

class GalerkinState;

/**
The Galerkin radiosity specific data to be kept with every surface or
cluster element. A flag indicates whether a given element is a cluster or
surface elements. There are only a few differences between surface and cluster
elements: cluster elements always have a constant basis on them, they contain
a pointer to the Geometry to which they are associated, while a surface element has
a pointer to the Patch to which is belongs, they have only irregular sub-elements, and no up trans
*/
class GalerkinElement: public Element{ private:
    /**
    Position and orientation of the regular sub-elements is fully
    determined by the following transforms, that transform (u,v)
    parameters of a point on a sub-element to the (u',v') parameters
    of the same point on the parent element
    */

    static int numberOfElements;
    static int numberOfClusters;

    static const Matrix2x2 quadToParentTransformMatrix[4];
    static const Matrix2x2 triangleToParentTransformMatrix[4];

    void initializeCommon(GalerkinState *inGalerkinState);

    explicit GalerkinElement(GalerkinState *inGalerkinState);

    GalerkinElement *regularSubElementAtPoint(double *u, double *v);

  public:
    Patch *patch;
    Geometry *geometry;
    float potential; // Total potential of the element
    float receivedPotential; // Potential received during the last iteration
    float unShotPotential; // Un-shot potential (progressive refinement radiosity)
    float directPotential;
    ArrayList<Interaction *> *interactions; // Links with other patches: when using
			 // a shooting algorithm, the links are kept with the source element. When doing gathering, // the links are kept with the receiver element
    float minimumArea; // Equal to area for a surface element or the area
                       // of the smallest surface element in a cluster
    float blockerSize; // Equivalent blocker size for multi-resolution visibility
    int numberOfPatches; // Number of patches in a cluster
    int scratchVisibilityUsageCounter; // Used only on Z-depth visibility clustering strategy
    GalerkinElementRenderMode childNumber; // Rang nr of regular sub-element in parent
    char basisSize; // Number of coefficients to represent radiance
    char basisUsed; // Number of coefficients effectively used (<=basis_size)
    GalerkinState *galerkinState;

    explicit GalerkinElement(Patch *patch, GalerkinState *inGalerkinState);
    explicit GalerkinElement(Geometry *inGeometry, GalerkinState *inGalerkinState);
    ~GalerkinElement();

    static int getNumberOfElements();
    static int getNumberOfClusters();
    static int getNumberOfSurfaceElements();
    static GalerkinElement *fromPatch(const Patch *patch);
    static void initializeBasis();
    static int renderMode(const RenderOptions *renderOptions);

    void regularSubDivide();
    GalerkinElement *regularLeafAtPoint(double *u, double *v);
    int vertices(Vector3D *p) const;
    BoundingBox *bounds(BoundingBox *boundingBox) const;
    Vector3D midPoint() const;
    void initPolygon(Polygon *polygon) const;
    void reAllocCoefficients();

    Patch *getPatch() const{ return patch;
    }

    void setPatch(Patch *inPatch){ patch = inPatch;
    }
};

#include "galerkin/GalerkinState.h"

#endif
