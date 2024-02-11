#ifndef __SHAFT_CULLING__
#define __SHAFT_CULLING__

/**
References:

- Haines, E. A. and Wallace, J. R. "Shaft culling for
  efficient ray-traced radiosity", 2nd Eurographics Workshop
  on Rendering, Barcelona, Spain, May 1991
*/

#include "java/util/ArrayList.h"
#include "scene/polygon.h"

class SHAFTPLANE {
  public:
    float n[3];
    float d;
    int coord_offset[3]; // Coord. offset for nearest corner in box-plane tests
};

#define SHAFTMAXPLANES 16 // Max. 16 planes in plane-set: maximum 8 for a
    // box-to-box shaft, maximum 2 times the total nr of vertices for a
    // patch-to-patch shaft

// The shaft is the region bounded by extent and ref1 and ref2 (if defined)
// and on the negative side of the planes
class SHAFT {
  public:
    float *ref1; // Bounding boxes of the reference volumeListsOfItems and the whole shaft
    float *ref2;
    float extent[6];
    SHAFTPLANE plane[SHAFTMAXPLANES];
    int planes;  // Number of planes in plane-set
    Patch *omit[2]; // Geometries to be ignored during shaft culling. max. 2!
    int nromit; // Number of geometries to be ignored
    Geometry *dontOpen[2]; // Geometries not to be opened during shaft culling. max. 2!
    int nrdontopen; // Number of geometries not to be opened
    Vector3D center1; // The line segment from center1 to center2 is guaranteed
				      // to lay within the shaft
    Vector3D center2;
    int cut; // A boolean initialized to FALSE when the shaft is created and set
             // to TRUE during shaft culling if there are patches that cut the shaft. If
			 // after shaft culling, this flag is TRUE, there is full occlusion due to
             // one occluder.
			 //	As soon as such a situation is detected, shaft culling ends and the
             //	occluder in question is the first patch in the returned candidate list.
             //	The candidate list does not contain all occluders!
};

enum SHAFTCULLSTRATEGY {
    KEEP_CLOSED,
    OVERLAP_OPEN,
    ALWAYS_OPEN
};

extern SHAFT *constructShaft(float *ref1, float *ref2, SHAFT *shaft);
extern SHAFT *constructPolygonToPolygonShaft(POLYGON *p1, POLYGON *p2, SHAFT *shaft);
extern void setShaftOmit(SHAFT *shaft, Patch *geom);
extern void setShaftDontOpen(SHAFT *shaft, Geometry *geom);
extern void doShaftCulling(java::ArrayList<Geometry *> *world, SHAFT *shaft, java::ArrayList<Geometry *> *candidateList);
extern void shaftCullGeometry(Geometry *geometry, SHAFT *shaft, java::ArrayList<Geometry *> *candidateList);
extern java::ArrayList<Patch *> *shaftCullPatchList(java::ArrayList<Patch *> *patchList, SHAFT *shaft);
extern void freeCandidateList(java::ArrayList<Geometry *> *candidateList);

#endif
