#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __MGF_HANDLER_GEOMETRY__
#define __MGF_HANDLER_GEOMETRY__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateAxis.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Df.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"
#include "vsdk/toolkit/io/context/TransformStackContext.h"
#include "vsdk/toolkit/io/context/VertexContext.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/Vertex.h"

class MgfVertexFaceEntitySupport {
  public:
    static int handleVertexEntity(int ac, const char **av, ParseRuntimeContext *context);
    static int handleFaceEntity(int argc, const char **argv, ParseRuntimeContext *context);
    static int handleFaceWithHolesEntity(int argc, const char **argv, ParseRuntimeContext *context);
    static int handleSurfaceEntity(int argc, const char **argv, ParseRuntimeContext *context);
    static void initGeometryContextTables(ParseRuntimeContext *context);
    static VertexContext *getNamedVertex(const char *name, ParseRuntimeContext *context);

  private:
    #define MAXIMUM_FACE_VERTICES 100
    static long transformXid(const TransformStackContext *xf);
    static int doDiscreteConic(int argc, const char **argv, ParseRuntimeContext *context);
    static Vector3Df *installPoint(float x, float y, float z, const ParseRuntimeContext *context);
    static Vector3Df *installNormal(float x, float y, float z, const ParseRuntimeContext *context);
    static Vertex *installVertex(Vector3Df *coord, Vector3Df *norm, const ParseRuntimeContext *context);
    static Vertex *getVertex(const char *name, ParseRuntimeContext *context);
    static Vertex *getBackFaceVertex(Vertex *v, const ParseRuntimeContext *context);
    static Patch *newFace(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4, const ParseRuntimeContext *context);
    static Vector3Df *faceNormal(int numberOfVertices, Vertex **v, Vector3Df *normal);
    static void vectorProject(Vector2Df &r, const Vector3Df &p, CoordinateAxis i);
    static bool faceIsConvex(int numberOfVertices, Vertex **v, const Vector3Df *normal);
    static bool pointInsideTriangle2D(const Vector2Df *p, const Vector2Df *p1, const Vector2Df *p2, const Vector2Df *p3);
    static bool segmentsIntersect2D(const Vector2Df *p1, const Vector2Df *p2, const Vector2Df *p3, const Vector2Df *p4);
    static void doComplexFace(int n, Vertex **v, Vector3Df *normal, Vertex **backVertex, ParseRuntimeContext *context);
};

#endif
