#ifndef _SHAFTCULLING_H_
#define _SHAFTCULLING_H_

/* References:
 *
 * - Haines, E. A. and Wallace, J. R. "Shaft culling for
 *   efficient ray-traced radiosity", 2nd Eurographics Workshop
 *   on Rendering, Barcelona, Spain, May 1991
 */

#include "skin/geomlist.h"
#include "skin/patchlist.h"
#include "scene/polygon.h"
#include "scene/grid.h"

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
				 * items and the whole shaft. */
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

/* Specify a shaftculling strategy. Defaults is OVERLAP_OPEN. Returns
 * previous strategy. */
extern SHAFTCULLSTRATEGY SetShaftCullStrategy(SHAFTCULLSTRATEGY strategy);

/* Constructs a shaft for two given bounding boxes. Supply a pointer to a SHAFT
 * structure. This structure will be filled in and the pointer returned if succesfull.
 * nullptr is returned if something goes wrong. */
extern SHAFT *ConstructShaft(float *ref1, float *ref2, SHAFT *shaft);

/* Constructs a shaft enclosing the two given polygons. */
extern SHAFT *ConstructPolygonToPolygonShaft(POLYGON *p1, POLYGON *p2, SHAFT *shaft);

/* marks a geometry as to be omitted during shaftculling: it will not be added to the
 * candidatelist, even if the geometry overlaps or is inside the shaft */
extern void ShaftOmit(SHAFT *shaft, Geometry *geom);

/* marks a geometry as one not to be opened during shaft culling. */
extern void ShaftDontOpen(SHAFT *shaft, Geometry *geom);

/* adds all objects from world that overlap or lay inside the shaft to
 * candlist, returns the new candidate list */
extern GEOMLIST *DoShaftCulling(GEOMLIST *world, SHAFT *shaft, GEOMLIST *candlist);

/* Tests the geom w.r.t. the shaft: if the geom is inside or overlaps
 * the shaft, it is copied to the shaft or broken open depending on
 * the current shaft culling strategy. */
extern GEOMLIST *ShaftCullGeom(Geometry *geom, SHAFT *shaft, GEOMLIST *candlist);

/* shaftculling for patch lists */
extern PATCHLIST *ShaftCullPatchlist(PATCHLIST *pl, SHAFT *shaft, PATCHLIST *culledpatchlist);

/* frees the memory occupied by a candidatelist produced by DoShaftCulling */
extern void FreeCandidateList(GEOMLIST *candlist);

#endif
