#ifndef __MGF_HANDLER_GEOMETRY__
#define __MGF_HANDLER_GEOMETRY__

#include "common/linealAlgebra/CoordinateAxis.h"
#include "io/context/ParseSession.h"
#include "io/context/VertexContext.h"

class Patch;
class TransformStackContext;
class Vector2D;
class Vector3D;
class Vertex;

class MgfHandlerGeometry {
  public:
    static int handleVertexEntity(int ac, const char **av, ParseSession *context);
    static int handleFaceEntity(int argc, const char **argv, ParseSession *context);
    static int handleFaceWithHolesEntity(int argc, const char **argv, ParseSession *context);
    static int handleSurfaceEntity(int argc, const char **argv, ParseSession *context);
    static void initGeometryContextTables(ParseSession *context);
    static VertexContext *getNamedVertex(const char *name, ParseSession *context);

  private:
    static long transformXid(const TransformStackContext *xf);
    static int doDiscreteConic(int argc, const char **argv, ParseSession *context);
    static Vector3D *installPoint(float x, float y, float z, const ParseSession *context);
    static Vector3D *installNormal(float x, float y, float z, const ParseSession *context);
    static Vertex *installVertex(Vector3D *coord, Vector3D *norm, const ParseSession *context);
    static Vertex *getVertex(const char *name, ParseSession *context);
    static Vertex *getBackFaceVertex(Vertex *v, const ParseSession *context);
    static Patch *newFace(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4, const ParseSession *context);
    static Vector3D *faceNormal(int numberOfVertices, Vertex **v, Vector3D *normal);
    static void vectorProject(Vector2D &r, const Vector3D &p, CoordinateAxis i);
    static bool faceIsConvex(int numberOfVertices, Vertex **v, const Vector3D *normal);
    static bool pointInsideTriangle2D(const Vector2D *p, const Vector2D *p1, const Vector2D *p2, const Vector2D *p3);
    static bool segmentsIntersect2D(const Vector2D *p1, const Vector2D *p2, const Vector2D *p3, const Vector2D *p4);
    static void doComplexFace(int n, Vertex **v, Vector3D *normal, Vertex **backVertex, ParseSession *context);
};

#endif
