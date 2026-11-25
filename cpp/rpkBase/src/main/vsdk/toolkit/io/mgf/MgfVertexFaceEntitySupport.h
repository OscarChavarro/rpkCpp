#ifndef __MGF_VERTEX_FACE_ENTITY_SUPPORT__
#define __MGF_VERTEX_FACE_ENTITY_SUPPORT__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateAxis.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Df.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
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
    static Vector3Df *installPoint(float x, float y, float z, const ParseRuntimeContext *context);
    static Vector3Df *installNormal(float x, float y, float z, const ParseRuntimeContext *context);
    static RpkVertex *installVertex(Vector3Df *coord, Vector3Df *norm, const ParseRuntimeContext *context);
    static RpkVertex *getVertex(const char *name, ParseRuntimeContext *context);
    static RpkVertex *getBackFaceVertex(RpkVertex *v, const ParseRuntimeContext *context);
    static Patch *newFace(RpkVertex *v1, RpkVertex *v2, RpkVertex *v3, RpkVertex *v4, const ParseRuntimeContext *context);
    static Vector3Df *faceNormal(int numberOfVertices, RpkVertex **v, Vector3Df *normal);
    static void vectorProject(Vector2Df &r, const Vector3Df &p, CoordinateAxis i);
    static bool faceIsConvex(int numberOfVertices, RpkVertex **v, const Vector3Df *normal);
    static bool pointInsideTriangle2D(const Vector2Df *p, const Vector2Df *p1, const Vector2Df *p2, const Vector2Df *p3);
    static bool segmentsIntersect2D(const Vector2Df *p1, const Vector2Df *p2, const Vector2Df *p3, const Vector2Df *p4);
    static void doComplexFace(int n, RpkVertex **v, Vector3Df *normal, RpkVertex **backVertex, ParseRuntimeContext *context);
};

#endif
