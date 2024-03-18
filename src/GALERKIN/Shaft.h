#ifndef __SHAFT_CULLING__
#define __SHAFT_CULLING__

/**
References:

- Haines, E. A. and Wallace, J. R. "Shaft culling for
  efficient ray-traced radiosity", 2nd Euro-graphics Workshop
  on Rendering, Barcelona, Spain, May 1991
*/

#include "java/util/ArrayList.h"
#include "scene/Polygon.h"

class ShaftPlane {
  public:
    float n[3];
    float d;
    int coordinateOffset[3]; // Coordinate offset for nearest corner in box-plane tests
};

// Maximum 16 planes in plane-set: maximum 8 for a box-to-box shaft, maximum 2
// times the total nr of vertices for a patch-to-patch shaft
#define SHAFT_MAX_PLANES 16

/**
Positions of item with respect to a plane or a shaft
*/
enum ShaftPlanePosition {
    INSIDE,
    OVERLAP,
    OUTSIDE,
    COPLANAR
};

// The shaft is the region bounded by extent and ref1 and ref2 (if defined)
// and on the negative side of the planes
class Shaft {
  private:
    static ShaftPlanePosition testPolygonWithRespectToPlane(POLYGON *poly, Vector3D *normal, double d);
    static void fillInPlane(ShaftPlane *plane, float nx, float ny, float nz, float d);
    static bool verifyPolygonWrtPlane(Polygon *polygon, Vector3D *normal, double d, int side);
    static int testPointWrtPlane(Vector3D *p, Vector3D *normal, double d);
    static int compareShaftPlanes(ShaftPlane *plane1, ShaftPlane *plane2);
    static void keep(Geometry *geometry, java::ArrayList<Geometry *> *candidateList);

    void constructPolygonToPolygonPlanes(Polygon *p1, Polygon *p2);
    int shaftPatchTest(Patch *patch);
    bool closedGeometry(Geometry *geometry);
    int uniqueShaftPlane(ShaftPlane *parameterPlane);
    ShaftPlanePosition boundingBoxTest(BoundingBox *parameterBoundingBox);

public:
    BoundingBox *ref1; // Bounding boxes of the reference volumeListsOfItems and the whole shaft
    BoundingBox *ref2;
    BoundingBox boundingBox;
    ShaftPlane plane[SHAFT_MAX_PLANES];
    int planes;  // Number of planes in plane-set
    Patch *omit[2]; // Geometries to be ignored during shaft culling, maximum 2
    int numberOfGeometriesToOmit;
    Geometry *dontOpen[2]; // Geometries not to be opened during shaft culling, maximum 2
    int numberOfGeometriesToNotOpen;
    Vector3D center1; // The line segment from center1 to center2 is guaranteed
				      // to lay within the shaft
    Vector3D center2;
    int cut; // A boolean initialized to FALSE when the shaft is created and set
             // to TRUE during shaft culling if there are patches that cut the shaft. If
			 // after shaft culling, this flag is TRUE, there is full occlusion due to
             // one occluder.
			 //	As soon as such a situation is detected, shaft culling ends and the
             //	occluder in question is the first patch in the returned candidate list.
             //	The candidate list does not contain all occluder!
    Shaft();
    void constructShaft(BoundingBox *boundingBox1, BoundingBox *boundingBox2);
    void constructFromPolygonToPolygon(POLYGON *polygon1, POLYGON *polygon2);

    java::ArrayList<Patch *> *cullPatches(java::ArrayList<Patch *> *patchList);
    int patchIsOnOmitSet(Patch *geometry);
    void shaftCullOpen(Geometry *geometry, java::ArrayList<Geometry *> *candidateList);
    void setShaftOmit(Patch *patch);
    void setShaftDontOpen(Geometry *geometry);
    void doCulling(java::ArrayList<Geometry *> *world, java::ArrayList<Geometry *> *candidateList);
    void cullGeometry(Geometry *geometry, java::ArrayList<Geometry *> *candidateList);
};

enum ShaftCullStrategy {
    KEEP_CLOSED,
    OVERLAP_OPEN,
    ALWAYS_OPEN
};

extern void freeCandidateList(java::ArrayList<Geometry *> *candidateList);

#endif
