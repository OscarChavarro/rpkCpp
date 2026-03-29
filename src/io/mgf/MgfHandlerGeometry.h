#ifndef __MGF_HANDLER_GEOMETRY__
#define __MGF_HANDLER_GEOMETRY__

#include "common/linealAlgebra/CoordinateAxis.h"
#include "io/context/MgfParseSession.h"
#include "io/mgf/MgfVertexContext.h"

class Patch;
class TransformStackContext;
class Vector2D;
class Vector3D;
class Vertex;

class MgfHandlerGeometry {
  public:
    static int handleVertexEntity(int ac, const char **av, MgfParseSession *context);
    static int handleFaceEntity(int argc, const char **argv, MgfParseSession *context);
    static int handleFaceWithHolesEntity(int argc, const char **argv, MgfParseSession *context);
    static int handleSurfaceEntity(int argc, const char **argv, MgfParseSession *context);
    static void initGeometryContextTables(MgfParseSession *context);
    static MgfVertexContext *getNamedVertex(const char *name, MgfParseSession *context);

  private:
    static long transformXid(const TransformStackContext *xf);
    static int doDiscreteConic(int argc, const char **argv, MgfParseSession *context);
    static Vector3D *installPoint(float x, float y, float z, const MgfParseSession *context);
    static Vector3D *installNormal(float x, float y, float z, const MgfParseSession *context);
    static Vertex *installVertex(Vector3D *coord, Vector3D *norm, const MgfParseSession *context);
    static Vertex *getVertex(const char *name, MgfParseSession *context);
    static Vertex *getBackFaceVertex(Vertex *v, const MgfParseSession *context);
    static Patch *newFace(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4, const MgfParseSession *context);
    static Vector3D *faceNormal(int numberOfVertices, Vertex **v, Vector3D *normal);
    static void vectorProject(Vector2D &r, const Vector3D &p, CoordinateAxis i);
    static int faceIsConvex(int numberOfVertices, Vertex **v, const Vector3D *normal);
    static int pointInsideTriangle2D(const Vector2D *p, const Vector2D *p1, const Vector2D *p2, const Vector2D *p3);
    static int segmentsIntersect2D(const Vector2D *p1, const Vector2D *p2, const Vector2D *p3, const Vector2D *p4);
    static void doComplexFace(int n, Vertex **v, Vector3D *normal, Vertex **backVertex, MgfParseSession *context);
};

#endif
