#ifndef __MGF_VERTEX_FACE_ENTITY_SUPPORT__
#define __MGF_VERTEX_FACE_ENTITY_SUPPORT__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateAxis.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"
#include "vsdk/toolkit/io/context/TransformStackContext.h"
#include "vsdk/toolkit/io/context/VertexContext.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

class MgfVertexFaceEntitySupport {
  public:
    static int handleVertexEntity(int ac, const char **av, ParseRuntimeContext *context);
    static int handleFaceEntity(int argc, const char **argv, ParseRuntimeContext *context);
    static int handleFaceWithHolesEntity(int argc, const char **argv, ParseRuntimeContext *context);
    static int handleSurfaceEntity(int argc, const char **argv, ParseRuntimeContext *context);
    static void initGeometryContextTables(ParseRuntimeContext *context);
    static VertexContext *getNamedVertex(const char *name, ParseRuntimeContext *context);

  private:
    static constexpr int MAXIMUM_FACE_VERTICES = 100;
    static long transformXid(const TransformStackContext *xf);
    static int doDiscreteConic(int argc, const char **argv, ParseRuntimeContext *context);
    static Vector3Dd *installPoint(double x, double y, double z, const ParseRuntimeContext *context);
    static Vector3Dd *installNormal(double x, double y, double z, const ParseRuntimeContext *context);
    static RpkVertex *installVertex(Vector3Dd *coord, Vector3Dd *norm, const ParseRuntimeContext *context);
    static RpkVertex *getVertex(const char *name, ParseRuntimeContext *context);
    static RpkVertex *getBackFaceVertex(RpkVertex *v, const ParseRuntimeContext *context);
    static Patch *newFace(RpkVertex *v1, RpkVertex *v2, RpkVertex *v3, RpkVertex *v4, const ParseRuntimeContext *context);
    static Vector3Dd *faceNormal(int numberOfVertices, RpkVertex **v, Vector3Dd *normal);
    static void vectorProject(Vector2Dd &r, const Vector3Dd &p, CoordinateAxis i);
    static bool faceIsConvex(int numberOfVertices, RpkVertex **v, const Vector3Dd *normal);
    static bool pointInsideTriangle2D(const Vector2Dd *p, const Vector2Dd *p1, const Vector2Dd *p2, const Vector2Dd *p3);
    static bool segmentsIntersect2D(const Vector2Dd *p1, const Vector2Dd *p2, const Vector2Dd *p3, const Vector2Dd *p4);
    static void doComplexFace(int n, RpkVertex **v, Vector3Dd *normal, RpkVertex **backVertex, ParseRuntimeContext *context);
};

#endif
