#ifndef __SHAFT_CULLING__
#define __SHAFT_CULLING__

/**
References:

- [HAIN1991] Haines, E. A. and Wallace, J. R. "Shaft culling for efficient ray-traced radiosity",
  2nd Euro-graphics Workshop on Rendering, Barcelona, Spain, May 1991
*/

#include "java/util/ArrayList.h"
#include "scene/Polygon.h"
#include "GALERKIN/ShaftPlanePosition.h"
#include "GALERKIN/ShaftPlane.h"
#include "GALERKIN/ShaftCullStrategy.h"

#define MAX_SKIP_ELEMENTS 2

// Maximum 16 numberOfPlanesInSet in plane-set: maximum 8 for a box-to-box shaft (see figure [HAIN1991].2),
// maximum 2 times the total number of vertices for a patch-to-patch shaft
#define SHAFT_MAX_PLANES 16

/**
The shaft is the region bounded by extent and referenceItem1 and referenceItem2 (if defined)
and on the negative side of the numberOfPlanesInSet

Note: the name "Shaft" can be misleading. From [HAIN1991] can be seen that this is more
a kind of convex envelope.
*/
class Shaft {
  private:
    BoundingBox *referenceItem1; // Bounding boxes of the reference items
    BoundingBox *referenceItem2;
    BoundingBox extentBoundingBox;
    ShaftPlane planeSet[SHAFT_MAX_PLANES];
    int numberOfPlanesInSet;  // Number of planes in plane-set

    int patchIdsToOmit[MAX_SKIP_ELEMENTS]; // Geometries to be ignored during shaft culling, maximum 2
    int numberOfGeometriesToOmit;
    int geometryIdsToAvoidOpening[MAX_SKIP_ELEMENTS]; // Geometries not to be opened during shaft culling, maximum 2
    int numberOfGeometriesToAvoidOpen;

    Vector3D center1; // The line segment from center1 to center2 is guaranteed
    // to lay within the shaft
    Vector3D center2;
    bool cut; // A boolean initialized to FALSE when the shaft is created and set
    // to TRUE during shaft culling if there are patches that cut the shaft. If
    // after shaft culling, this flag is TRUE, there is full occlusion due to
    // one occluder.
    // As soon as such a situation is detected, shaft culling ends and the
    // occluder in question is the first patch in the returned candidate list.
    // The candidate list does not contain all occluder!

    static ShaftPlanePosition testPolygonWithRespectToPlane(const Polygon *polygon, const Vector3D *normal, double d);
    static void fillInPlane(ShaftPlane *plane, float nx, float ny, float nz, float d);

    static bool
    verifyPolygonWithRespectToPlane(
        const Polygon *polygon,
        const Vector3D *normal,
        double d,
        ShaftPlanePosition side);

    static ShaftPlanePosition testPointWithRespectToPlane(const Vector3D *p, const Vector3D *normal, double d);
    static int compareShaftPlanes(const ShaftPlane *plane1, const ShaftPlane *plane2);
    static void keep(Geometry *geometry, java::ArrayList<Geometry *> *candidateList);

    void constructPolygonToPolygonPlanes(const Polygon *p1, const Polygon *p2);
    ShaftPlanePosition shaftPatchTest(Patch *patch);
    bool closedGeometry(const Geometry *geometry) const;
    int uniqueShaftPlane(const ShaftPlane *parameterPlane) const;
    ShaftPlanePosition boundingBoxTest(const BoundingBox *parameterBoundingBox) const;
    java::ArrayList<Patch *> *cullPatches(const java::ArrayList<Patch *> *patchList);
    int patchIsOnOmitSet(const Patch *patch) const;
    void shaftCullOpen(Geometry *geometry, java::ArrayList<Geometry *> *candidateList, ShaftCullStrategy strategy);

  public:
    Shaft();

    static void freeCandidateList(java::ArrayList<Geometry *> *candidateList);

    bool isCut() const;
    void constructFromBoundingBoxes(BoundingBox *boundingBox1, BoundingBox *boundingBox2);
    void constructFromPolygonToPolygon(const Polygon *polygon1, const Polygon *polygon2);
    void setShaftOmit(Patch *patch);
    void setShaftDontOpen(Geometry *geometry);

    void
    doCulling(
        const java::ArrayList<Geometry *> *world,
        java::ArrayList<Geometry *> *candidateList,
        ShaftCullStrategy strategy);

    void
    cullGeometry(
        Geometry *geometry,
        java::ArrayList<Geometry *> *candidateList,
        ShaftCullStrategy strategy);
};

#endif
