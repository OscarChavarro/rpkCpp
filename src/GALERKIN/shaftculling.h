#ifndef __SHAFT_CULLING__
#define __SHAFT_CULLING__

/** References:
 *
 * - Haines, E. A. and Wallace, J. R. "Shaft culling for
 *   efficient ray-traced radiosity", 2nd Eurographics Workshop
 *   on Rendering, Barcelona, Spain, May 1991
 */

#include "java/util/ArrayList.h"
#include "scene/polygon.h"

class SHAFTPLANE {
  public:
    float n[3];
    float d;
    int coord_offset[3]; // coord. offset for nearest corner in box-plane tests
};

#define SHAFTMAXPLANES    16    /* max. 16 planes in plane-set: maximum 8
				 * for a box-to-box shaft, maximum 2 times
				 * the total nr of vertices for a 
				 * patch-to-patch shaft. */

/* The shaft is the region bounded by extent and ref1 and ref2 (if defined)
 * and on the negative side of the planes. */
class SHAFT {
  public:
    float *ref1, *ref2, extent[6]; /* bounding boxes of the reference
				 * volumeListsOfItems and the whole shaft. */
    SHAFTPLANE plane[SHAFTMAXPLANES];
    int planes;        /* nr of planes in plane-set */
    Geometry *omit[2];    /* geometries to be ignored during shaftculling. max. 2! */
    int nromit;        /* nr of geometries to be ignored */
    Geometry *dontopen[2];    /* geometries not to be opened during shaftculling. max. 2! */
    int nrdontopen;    /* nr of geometries not to be opened */
    Vector3D center1, center2;  /* the line segment from center1
				 * to center2 is guaranteed to lay within 
				 * the shaft. */
    int cut;        /* A boolean initialized to FALSE when the shaft
				 * is created and set to TRUE during shaft culling
				 * if there are patches that cut the shaft. If
				 * after shaft culling, this flag is TRUE, there is
				 * full occlusion due to one occluder. 
				 * As soon as such a situaiton is detected, 
				 * shaftculling ends and the occluder in
				 * question is the first patch in the returned
				 * candidate list. The candidate list does
				 * not contain all occluders! */
};

enum SHAFTCULLSTRATEGY {
    KEEP_CLOSED, OVERLAP_OPEN, ALWAYS_OPEN
};

extern SHAFT *constructShaft(float *ref1, float *ref2, SHAFT *shaft);
extern SHAFT *constructPolygonToPolygonShaft(POLYGON *p1, POLYGON *p2, SHAFT *shaft);
extern void shaftOmit(SHAFT *shaft, Geometry *geom);
extern void shaftDontOpen(SHAFT *shaft, Geometry *geom);
extern GeometryListNode *doShaftCulling(GeometryListNode *world, SHAFT *shaft, GeometryListNode *candidateList);
extern GeometryListNode *shaftCullGeom(Geometry *geom, SHAFT *shaft, GeometryListNode *candlist);
extern PatchSet *shaftCullPatchList(PatchSet *pl, SHAFT *shaft, PatchSet *culledPatchList);
extern void freeCandidateList(GeometryListNode *candidateList);

#endif
