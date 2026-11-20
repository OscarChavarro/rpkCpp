#ifndef SHAFT_CULLING__
#define SHAFT_CULLING__

/**
References:

- [HAIN1991] Haines, E. A. and Wallace, J. R. "Shaft culling for efficient ray-traced radiosity",
  2nd Euro-graphics Workshop on Rendering, Barcelona, Spain, May 1991
*/

#include "vsdk/toolkit/scene/FaceBoundsTable.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/environment/geometry/elements/PatchSet.h"
#include "vsdk/toolkit/scene/Polygon.h"
#include "vsdk/toolkit/galerkin/ShaftCullStrategy.h"
#include "vsdk/toolkit/galerkin/ShaftPlane.h"
#include "vsdk/toolkit/galerkin/ShaftPlanePosition.h"

/**
The shaft is the region bounded by extent and referenceItem1 and referenceItem2 (if defined)
and on the negative side of the numberOfPlanesInSet

Note: the name "Shaft" can be misleading. From [HAIN1991] can be seen that this is more
a kind of convex envelope.
*/
class Shaft {
  private:
    static MemoryPool<PatchSet> patchSetPool;
    static bool patchSetPoolInitialized;

    static constexpr int MAX_SKIP_ELEMENTS = 2;
    // Maximum 16 numberOfPlanesInSet in plane-set: maximum 8 for a box-to-box shaft (see figure [HAIN1991].2),
    // maximum 2 times the total number of vertices for a patch-to-patch shaft
    static constexpr int SHAFT_MAX_PLANES = 16;
    static constexpr int MIN_MAX_DIMENSIONS = 6;
    static constexpr int NONE = -1;

    const FaceBoundsTable *const faceBounds; // Owned by the scene; bounds of the culled patches
    RpkAxisAlignedBoundingBox *referenceItem1; // Bounding boxes of the reference items
    RpkAxisAlignedBoundingBox *referenceItem2;
    RpkAxisAlignedBoundingBox extentBoundingBox;
    ShaftPlane planeSet[SHAFT_MAX_PLANES];
    long numberOfPlanesInSet;  // Number of planes in plane-set

    unsigned patchIdsToOmit[MAX_SKIP_ELEMENTS]; // Geometries to be ignored during shaft culling, maximum 2
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
    static void ensurePatchSetPoolInitialized();
    static PatchSet *createPatchSetWithPool(const java::ArrayList<Patch *> *patches);
    static void keep(RpkGeometry *geometry, java::ArrayList<RpkGeometry *> *candidateList);

    void constructPolygonToPolygonPlanes(const Polygon *p1, const Polygon *p2);
    ShaftPlanePosition shaftPatchTest(Patch *patch);
    bool closedGeometry(const RpkGeometry *geometry) const;
    int uniqueShaftPlane(const ShaftPlane *parameterPlane) const;
    ShaftPlanePosition boundingBoxTest(const RpkAxisAlignedBoundingBox *parameterBoundingBox) const;
    void cullPatches(const java::ArrayList<Patch *> *patchList, java::ArrayList<Patch *> *culledPatchList);
    bool patchIsOnOmitSet(unsigned id) const;
    void shaftCullOpen(RpkGeometry *geometry, java::ArrayList<RpkGeometry *> *candidateList, ShaftCullStrategy strategy);

public:
    explicit Shaft(const FaceBoundsTable *inFaceBounds);

    static void freeCandidateList(java::ArrayList<RpkGeometry *> *candidateList);

    bool isCut() const;
    void constructFromBoundingBoxes(RpkAxisAlignedBoundingBox *boundingBox1, RpkAxisAlignedBoundingBox *boundingBox2);
    void constructFromPolygonToPolygon(const Polygon *polygon1, const Polygon *polygon2);
    void setShaftOmit(Patch *patch);
    void setShaftDontOpen(RpkGeometry *geometry);

    void
    doCulling(
        const java::ArrayList<RpkGeometry *> *world,
        java::ArrayList<RpkGeometry *> *candidateList,
        ShaftCullStrategy strategy);

    void
    cullGeometry(
        RpkGeometry *geometry,
        java::ArrayList<RpkGeometry *> *candidateList,
        ShaftCullStrategy strategy);
};

#endif
