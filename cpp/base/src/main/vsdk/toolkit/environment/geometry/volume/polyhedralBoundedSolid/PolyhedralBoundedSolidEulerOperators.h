#ifndef __POLYHEDRAL_BOUNDED_SOLID_EULER_OPERATORS__
#define __POLYHEDRAL_BOUNDED_SOLID_EULER_OPERATORS__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidLoop;
class _PolyhedralBoundedSolidHalfEdge;
class _PolyhedralBoundedSolidVertex;

class PolyhedralBoundedSolidEulerOperators {
private:
    static _PolyhedralBoundedSolidFace* lmefCircularFace(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidHalfEdge* he, int newFaceId);

public:
    static void mvfs(PolyhedralBoundedSolid* solid, const Vector3Dd& pos, int vertexId, int faceId);
    static void kvfs(PolyhedralBoundedSolid* solid);

    static void lmev(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidHalfEdge* he1, _PolyhedralBoundedSolidHalfEdge* he2, int vertexId, const Vector3Dd& p);
    static void lkev(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidHalfEdge* he1, _PolyhedralBoundedSolidHalfEdge* he2);
    static void lkef(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidHalfEdge* he1, _PolyhedralBoundedSolidHalfEdge* he2);
    static _PolyhedralBoundedSolidFace* lmef(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidHalfEdge* he1, _PolyhedralBoundedSolidHalfEdge* he2, int newFaceId);
    static void lkemr(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidHalfEdge* he1, _PolyhedralBoundedSolidHalfEdge* he2);
    static void lkfmrh(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidFace* face1, _PolyhedralBoundedSolidFace* face2);
    static _PolyhedralBoundedSolidFace* lmfkrh(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidLoop* l, int newFaceId);
    static void lkimrh(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidFace* face1, _PolyhedralBoundedSolidFace* face2);
    static _PolyhedralBoundedSolidFace* lmikrh(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidLoop* l, int newFaceId);
    static void lmekr(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidHalfEdge* he1, _PolyhedralBoundedSolidHalfEdge* he2);

    static bool lringmv(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidLoop* l, _PolyhedralBoundedSolidFace* toFace, bool setAsOuterLoop);

    static bool smev(PolyhedralBoundedSolid* solid, int f1, int v1, int v4, const Vector3Dd& p);
    static bool mev(PolyhedralBoundedSolid* solid, int f1, int f2, int v1, int v2, int v3, int v4, const Vector3Dd& p);
    static bool smef(PolyhedralBoundedSolid* solid, int f1, int v1, int v3, int newFaceId);
    static bool mef(PolyhedralBoundedSolid* solid, int f1, int f2, int v1, int v2, int v3, int v4, int newFaceId);
    static bool kemr(PolyhedralBoundedSolid* solid, int f1, int f2, int v1, int v2, int v3, int v4);
    static bool kfmrh(PolyhedralBoundedSolid* solid, int f1, int f2);
};

#endif
