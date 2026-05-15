#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __MGF_HANDLER_GEOMETRY__
#define __MGF_HANDLER_GEOMETRY__

#include "common/linealAlgebra/CoordinateAxis.h"
#include "common/linealAlgebra/Vector2D.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/context/ParseRuntimeContext.h"
#include "io/context/TransformStackContext.h"
#include "io/context/VertexContext.h"
#include "environment/geometry/elements/Patch.h"
#include "environment/geometry/elements/Vertex.h"

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
    static Vector3D *installPoint(float x, float y, float z, const ParseRuntimeContext *context);
    static Vector3D *installNormal(float x, float y, float z, const ParseRuntimeContext *context);
    static Vertex *installVertex(Vector3D *coord, Vector3D *norm, const ParseRuntimeContext *context);
    static Vertex *getVertex(const char *name, ParseRuntimeContext *context);
    static Vertex *getBackFaceVertex(Vertex *v, const ParseRuntimeContext *context);
    static Patch *newFace(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4, const ParseRuntimeContext *context);
    static Vector3D *faceNormal(int numberOfVertices, Vertex **v, Vector3D *normal);
    static void vectorProject(Vector2D &r, const Vector3D &p, CoordinateAxis i);
    static bool faceIsConvex(int numberOfVertices, Vertex **v, const Vector3D *normal);
    static bool pointInsideTriangle2D(const Vector2D *p, const Vector2D *p1, const Vector2D *p2, const Vector2D *p3);
    static bool segmentsIntersect2D(const Vector2D *p1, const Vector2D *p2, const Vector2D *p3, const Vector2D *p4);
    static void doComplexFace(int n, Vertex **v, Vector3D *normal, Vertex **backVertex, ParseRuntimeContext *context);
};

#endif
