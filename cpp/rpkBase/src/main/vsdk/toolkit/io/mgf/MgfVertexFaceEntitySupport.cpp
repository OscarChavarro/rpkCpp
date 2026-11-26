#include <cstring>
#include <cstdlib>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/CoordinateAxis.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/dataStructures/LookUpEntity.h"
#include "vsdk/toolkit/io/context/TokenValidationContext.h"
#include "vsdk/toolkit/io/mgf/MgfEntityControl.h"
#include "vsdk/toolkit/io/mgf/MgfVertexFaceEntitySupport.h"
#include "vsdk/toolkit/io/mgf/MgfMaterialEntitySupport.h"
#include "vsdk/toolkit/io/mgf/MgfObjectNameSupport.h"
#include "vsdk/toolkit/io/mgf/MgfTransformationSupport.h"
#include "vsdk/toolkit/io/mgf/MgfTessellationMath.h"
#include "vsdk/toolkit/io/mgf/MgfVitralPrimitiveTessellator.h"

long
MgfVertexFaceEntitySupport::transformXid(const TransformStackContext *xf) {
    return xf == nullptr ? 0L : xf->xid;
}

/**
Turns a surface primitive (sphere, torus, cylinder, ring, cone or prism)
into faces, through the VITRAL geometry of MgfVitralPrimitiveTessellator
*/
int
MgfVertexFaceEntitySupport::doDiscreteConic(int argc, const char **argv, ParseRuntimeContext *context) {
    int en = MgfEntityControl::mgfEntity(argv[0], context);

    switch ( en ) {
        case EntityTypeContext::SPHERE:
            return MgfVitralPrimitiveTessellator::handleSphere(argc, argv, context);
        case EntityTypeContext::TORUS:
            return MgfVitralPrimitiveTessellator::handleTorus(argc, argv, context);
        case EntityTypeContext::CYLINDER:
            return MgfVitralPrimitiveTessellator::handleCylinder(argc, argv, context);
        case EntityTypeContext::RING:
            return MgfVitralPrimitiveTessellator::handleRing(argc, argv, context);
        case EntityTypeContext::CONE:
            return MgfVitralPrimitiveTessellator::handleCone(argc, argv, context);
        case EntityTypeContext::PRISM:
            return MgfVitralPrimitiveTessellator::handlePrism(argc, argv, context);
        default:
            Logger::reportMessage("MgfVertexFaceEntitySupport", Logger::FATAL_ERROR, "mgf.c: doDiscreteConic", java::String::format("Unsupported geometry entity number %d", en));
    }
    return ParseErrorContext::MGF_ERROR_UNKNOWN_ENTITY; // Not reached: a FATAL_ERROR report throws
}

Vector3Dd *
MgfVertexFaceEntitySupport::installPoint(double x, double y, double z, const ParseRuntimeContext *context) {
    Vector3Dd *coord = new Vector3Dd(x, y, z);
    context->currentPointList->add(coord);
    return coord;
}

Vector3Dd *
MgfVertexFaceEntitySupport::installNormal(double x, double y, double z, const ParseRuntimeContext *context) {
    Vector3Dd *norm = new Vector3Dd(x, y, z);
    context->currentNormalList->add(norm);
    return norm;
}

RpkVertex *
MgfVertexFaceEntitySupport::installVertex(Vector3Dd *coord, Vector3Dd *norm, const ParseRuntimeContext *context) {
    java::ArrayList<Patch *> *newPatchList = new java::ArrayList<Patch *>();
    RpkVertex *v = new RpkVertex(coord, norm, nullptr, newPatchList);
    context->currentVertexList->add(v);
    return v;
}

RpkVertex *
MgfVertexFaceEntitySupport::getVertex(const char *name, ParseRuntimeContext *context) {
    VertexContext *vp = MgfVertexFaceEntitySupport::getNamedVertex(name, context);
    if ( vp == nullptr ) {
        return nullptr;
    }

    RpkVertex *theVertex = vp->vertex;
    if ( !theVertex
         || vp->clock >= 1
         || vp->xid != MgfVertexFaceEntitySupport::transformXid(context->transformContext)
         || MgfTessellationMath::isNull(vp->n, VSDK::EPSILON) ) {
        // New vertex, or updated vertex or same vertex, but other transform, or
        // vertex without normal: create a new Vertex
        Vector3Dd vert;
        Vector3Dd norm;
        Vector3Dd *theNormal;

        MgfTransformationSupport::mgfTransformPoint(&vert, &vp->p, context);
        Vector3Dd *thePoint = installPoint(static_cast<double>(vert.x()), static_cast<double>(vert.y()), static_cast<double>(vert.z()), context);
        if ( MgfTessellationMath::isNull(vp->n, VSDK::EPSILON) ) {
            theNormal = nullptr;
        } else {
            MgfTransformationSupport::mgfTransformVector(&norm, &vp->n, context);
            theNormal = installNormal(static_cast<double>(norm.x()), static_cast<double>(norm.y()), static_cast<double>(norm.z()), context);
        }
        theVertex = installVertex(thePoint, theNormal, context);
        vp->vertex = theVertex;
        vp->xid = MgfVertexFaceEntitySupport::transformXid(context->transformContext);
    }
    vp->clock = 0;

    return theVertex;
}

/**
Create a vertex with given name, but with reversed normal as
the given vertex. For back-faces of two-sided surfaces
*/
RpkVertex *
MgfVertexFaceEntitySupport::getBackFaceVertex(RpkVertex *v, const ParseRuntimeContext *context) {
    RpkVertex *back = v->back;

    if ( !back ) {
        Vector3Dd *point = v->point;
        Vector3Dd *normal = v->normal;
        if ( normal ) {
            normal = installNormal(-normal->x(), -normal->y(), -normal->z(), context);
        }

        back = v->back = installVertex(point, normal, context);
        back->back = v;
    }

    return back;
}

Patch *
MgfVertexFaceEntitySupport::newFace(RpkVertex *v1, RpkVertex *v2, RpkVertex *v3, RpkVertex *v4, const ParseRuntimeContext *context) {
    Patch *theFace;
    int numberOfVertices = v4 ? 4 : 3;

    if ( v1 == nullptr || v2 == nullptr || v3 == nullptr ) {
        return nullptr;
    }

    if ( context->transformContext && context->transformContext->rev ) {
        theFace = new Patch(numberOfVertices, v3, v2, v1, v4);
    } else {
        theFace = new Patch(numberOfVertices, v1, v2, v3, v4);
    }

    // If we are doing radiance computations, create radiance data for the patch
    if ( theFace->getMaterial() != nullptr ) {
        context->radianceMethod->createPatchData(theFace);
    }

    context->currentFaceList->add(theFace);

    return theFace;
}

/**
Computes the normal to the patch plane
*/
Vector3Dd *
MgfVertexFaceEntitySupport::faceNormal(int numberOfVertices, RpkVertex **v, Vector3Dd *normal) {
    Vector3Dd cur;
    Vector3Dd n;

    n = Vector3Dd(0, 0, 0);
    cur = (*(v[numberOfVertices - 1]->point)).subtract(*(v[0]->point));
    for ( int i = 0; i < numberOfVertices; i++ ) {
        Vector3Dd prev = cur;
        cur = (*(v[i]->point)).subtract(*(v[0]->point));
        n = n.withX(n.x() + ((prev.y() - cur.y()) * (prev.z() + cur.z())));
        n = n.withY(n.y() + ((prev.z() - cur.z()) * (prev.x() + cur.x())));
        n = n.withZ(n.z() + ((prev.x() - cur.x()) * (prev.y() + cur.y())));
    }
    double localNorm = n.length();

    if ( localNorm < VSDK::EPSILON ) {
        // Degenerate normal --> degenerate polygon
        return nullptr;
    }
    n = n.multiplyByInverse(static_cast<double>(localNorm), Numeric::Numeric::EPSILON);
    *normal = n;

    return normal;
}

/**
Given a vector p in 3D space and an index i, which is X, Y
or Z, projects the vector on the YZ, XZ or XY plane respectively
*/
void
MgfVertexFaceEntitySupport::vectorProject(Vector2Dd &r, const Vector3Dd &p, const CoordinateAxis i) {
    switch ( i ) {
        case CoordinateAxis::X:
            r = r.withX(p.y());
            r = r.withY(p.z());
            break;
        case CoordinateAxis::Y:
            r = r.withX(p.x());
            r = r.withY(p.z());
            break;
        case CoordinateAxis::Z:
            r = r.withX(p.x());
            r = r.withY(p.y());
            break;
        default:
            break;
    }
}

/**
Tests whether the polygon is convex or concave. This is accomplished by projecting
onto the coordinate plane "most parallel" to the polygon and checking the signs
the cross product of succeeding edges: the signs are all equal for a convex polygon
*/
bool
MgfVertexFaceEntitySupport::faceIsConvex(int numberOfVertices, RpkVertex **v, const Vector3Dd *normal) {
    Vector2Dd v2d[MAXIMUM_FACE_VERTICES + 1];
    Vector2Dd p;
    Vector2Dd c;
    int i;

    int index = static_cast<CoordinateAxis>(normal->dominantAxis());
    for ( i = 0; i < numberOfVertices; i++ ) {
        vectorProject(v2d[i], *(v[i]->point), static_cast<CoordinateAxis>(index));
    }

    p = Vector2Dd(v2d[3].x - v2d[2].x, v2d[3].y - v2d[2].y);
    c = Vector2Dd(v2d[0].x - v2d[3].x, v2d[0].y - v2d[3].y);
    int sign = (p.x * c.y > c.x * p.y) ? 1 : -1;

    for ( i = 1; i < numberOfVertices; i++ ) {
        p = Vector2Dd(c.x, c.y);
        c = Vector2Dd(v2d[i].x - v2d[i - 1].x, v2d[i].y - v2d[i - 1].y);
        if ( ((p.x * c.y > c.x * p.y) ? 1 : -1) != sign ) {
            return false;
        }
    }

    return true;
}

/**
Returns true if the 2D point p is inside the 2D triangle p1-p2-p3
*/
bool
MgfVertexFaceEntitySupport::pointInsideTriangle2D(const Vector2Dd *p, const Vector2Dd *p1, const Vector2Dd *p2, const Vector2Dd *p3) {
    // From Graphics Gems I, Didier Badouel, An Efficient Ray-Polygon Intersection, p390
    double u0 = p->x - p1->x;
    double v0 = p->y - p1->y;
    double u1 = p2->x - p1->x;
    double v1 = p2->y - p1->y;
    double u2 = p3->x - p1->x;
    double v2 = p3->y - p1->y;

    double a = 10.0;
    double b = 10.0; // Values large enough so the result would be false
    if ( java::Math::abs(u1) < VSDK::EPSILON ) {
        if ( java::Math::abs(u2) > VSDK::EPSILON && java::Math::abs(v1) > VSDK::EPSILON ) {
            b = u0 / u2;
            if ( b < VSDK::EPSILON || b > 1.0 - VSDK::EPSILON ) {
                return false;
            } else {
                a = (v0 - b * v2) / v1;
            }
        }
    } else {
        b = v2 * u1 - u2 * v1;
        if ( java::Math::abs(b) > VSDK::EPSILON ) {
            b = (v0 * u1 - u0 * v1) / b;
            if ( b < VSDK::EPSILON || b > 1.0 - VSDK::EPSILON ) {
                return false;
            } else {
                a = (u0 - b * u2) / u1;
            }
        }
    }

    return (a >= VSDK::EPSILON && a <= 1.0 - VSDK::EPSILON && (a + b) <= 1.0 - VSDK::EPSILON);
}

/**
Returns true if the 2D segments p1-p2 and p3-p4 intersect
*/
bool
MgfVertexFaceEntitySupport::segmentsIntersect2D(const Vector2Dd *p1, const Vector2Dd *p2, const Vector2Dd *p3, const Vector2Dd *p4) {
    double a;
    double b;
    double c;
    bool coLinear = false;

    // From Graphics Gems II, Mukesh Prasad, Intersection of Line Segments, p7
    double du = java::Math::abs(p2->x - p1->x);
    double dv = java::Math::abs(p2->y - p1->y);
    if ( du > VSDK::EPSILON || dv > VSDK::EPSILON ) {
        if ( dv > du ) {
            a = 1.0;
            b = -(p2->x - p1->x) / (p2->y - p1->y);
            c = -(p1->x + b * p1->y);
        } else {
            a = -(p2->y - p1->y) / (p2->x - p1->x);
            b = 1.0;
            c = -(a * p1->x + p1->y);
        }

        double r3 = a * p3->x + b * p3->y + c;
        double r4 = a * p4->x + b * p4->y + c;

        if ( java::Math::abs(r3) < VSDK::EPSILON && java::Math::abs(r4) < VSDK::EPSILON ) {
            coLinear = true;
        } else if ((r3 > -VSDK::EPSILON && r4 > -VSDK::EPSILON) || (r3 < VSDK::EPSILON && r4 < VSDK::EPSILON) ) {
            return false;
        }
    }

    if ( !coLinear ) {
        du = java::Math::abs(p4->x - p3->x);
        dv = java::Math::abs(p4->y - p3->y);
        if ( du > VSDK::EPSILON || dv > VSDK::EPSILON ) {
            if ( dv > du ) {
                a = 1.0;
                b = -(p4->x - p3->x) / (p4->y - p3->y);
                c = -(p3->x + b * p3->y);
            } else {
                a = -(p4->y - p3->y) / (p4->x - p3->x);
                b = 1.0;
                c = -(a * p3->x + p3->y);
            }

            double r1 = a * p1->x + b * p1->y + c;
            double r2 = a * p2->x + b * p2->y + c;

            if ( java::Math::abs(r1) < VSDK::EPSILON && java::Math::abs(r2) < VSDK::EPSILON ) {
                coLinear = true;
            } else if ( (r1 > -VSDK::EPSILON && r2 > -VSDK::EPSILON) || (r1 < VSDK::EPSILON && r2 < VSDK::EPSILON) ) {
                return false;
            }
        }
    }

    if ( !coLinear ) {
        return true;
    }

    return false; // Co-linear segments never intersect: do as if they are always
		          // a bit apart from each other
}

/**
Handles concave faces and faces with > 4 vertices. This routine started as an
ANSI-C version of face2tri, but I changed it a lot to make it more robust.
Inspiration comes from Burger and Gillis, Interactive Computer Graphics and
the (indispensable) Graphics Gems books
*/
void
MgfVertexFaceEntitySupport::doComplexFace(int n, RpkVertex **v, Vector3Dd *normal, RpkVertex **backVertex, ParseRuntimeContext *context) {
    Vector3Dd center;

    center = Vector3Dd(0.0, 0.0, 0.0);
    for ( int i = 0; i < n; i++ ) {
        center = center.add(*(v[i]->point));
    }
    center = center.multiplyByInverse(static_cast<double>(n), Numeric::EPSILON);

    double maxD = center.distance(*(v[0]->point));
    int max = 0;
    for ( int i = 1; i < n; i++ ) {
        double d = center.distance(*(v[i]->point));
        if ( d > maxD ) {
            maxD = d;
            max = i;
        }
    }

    char out[MAXIMUM_FACE_VERTICES + 1];

    for ( int i = 0; i < n; i++ ) {
        out[i] = false;
    }

    int p1 = max;
    int p0 = p1 - 1;
    if ( p0 < 0 ) {
        p0 = n - 1;
    }
    int p2 = (p1 + 1) % n;
    *normal = (*(v[p2]->point)).subtract(*(v[p1]->point)).crossProduct((*(v[p0]->point)).subtract(*(v[p1]->point)));
    *normal = normal->normalized(Numeric::EPSILON);
    int index = static_cast<CoordinateAxis>(normal->dominantAxis());

    Vector2Dd q[MAXIMUM_FACE_VERTICES + 1];
    for ( int i = 0; i < n; i++ ) {
        vectorProject(q[i], *(v[i]->point), static_cast<CoordinateAxis>(index));
    }

    int good;
    int corners = n;
    Vector3Dd nn;

    p0 = -1;
    while ( corners >= 3 ) {
        int start = p0;
        double d;
        double a = 0.0;

        do {
            p0 = (p0 + 1) % n;
            while ( out[p0] ) {
                p0 = (p0 + 1) % n;
            }

            p1 = (p0 + 1) % n;
            while ( out[p1] ) {
                p1 = (p1 + 1) % n;
            }

            p2 = (p1 + 1) % n;
            while ( out[p2] ) {
                p2 = (p2 + 1) % n;
            }

            if ( p0 == start ) {
                break;
            }

            nn = (*(v[p2]->point)).subtract(*(v[p1]->point)).crossProduct((*(v[p0]->point)).subtract(*(v[p1]->point)));
            a = nn.length();
            nn = nn.multiplyByInverse(static_cast<double>(a), Numeric::EPSILON);
            d = nn.distance(*normal);

            good = true;
            if ( d <= 1.0 ) {
                for ( int i = 0; i < n && good; i++ ) {
                    if ( out[i] || v[i] == v[p0] || v[i] == v[p1] || v[i] == v[p2] ) {
                        continue;
                    }

                    if ( pointInsideTriangle2D(&q[i], &q[p0], &q[p1], &q[p2]) ) {
                        good = false;
                    }

                    int j = (i + 1) % n;
                    if ( out[j] || v[j] == v[p0] ) {
                        continue;
                    }

                    if ( segmentsIntersect2D(&q[p2], &q[p0], &q[i], &q[j]) ) {
                        good = false;
                    }
                }
            }
        } while ( d > 1.0 || !good );

        if ( p0 == start ) {
            MgfEntityControl::doError("mis-built polygonal face", context);
            return; // Don't stop parsing the input however
        }

        if ( java::Math::abs(a) > VSDK::EPSILON ) {
            // Avoid degenerate faces
            Patch *face = newFace(v[p0], v[p1], v[p2], nullptr, context);
            if ( !context->currentMaterial->isSided() && face != nullptr ) {
                Patch *twin = newFace(backVertex[p2], backVertex[p1], backVertex[p0], nullptr, context);
                face->setTwin(twin);
                if ( twin != nullptr ) {
                    twin->setTwin(face);
                }
            }
        }

        out[p1] = true;
        corners--;
    }
}

int
MgfVertexFaceEntitySupport::handleFaceEntity(int argc, const char **argv, ParseRuntimeContext *context) {
    if ( argc < 4 ) {
        MgfEntityControl::doError("too few vertices in face", context);
        return ParseErrorContext::MGF_OK; // Don't stop parsing the input
    }

    if ( argc - 1 > MAXIMUM_FACE_VERTICES ) {
        MgfEntityControl::doWarning(
            "too many vertices in face. Recompile the program with larger MAXIMUM_FACE_VERTICES constant in read mgf",
            context);
        return ParseErrorContext::MGF_OK; // No reason to stop parsing the input
    }

    if ( !context->inComplex && MgfMaterialEntitySupport::mgfMaterialChanged(context->currentMaterial, context) ) {
        if ( context->inSurface ) {
            MgfObjectNameSupport::mgfObjectSurfaceDone(context);
        }
        MgfObjectNameSupport::mgfObjectNewSurface(context);
        MgfMaterialEntitySupport::mgfGetCurrentMaterial(&context->currentMaterial, context->singleSided, context);
    }

    RpkVertex *v[MAXIMUM_FACE_VERTICES + 1];
    RpkVertex *backV[MAXIMUM_FACE_VERTICES + 1];

    for ( int i = 0; i < argc - 1; i++ ) {
        v[i] = getVertex(argv[i + 1], context);
        if ( v[i] == nullptr ) {
            // This is however a reason to stop parsing the input
            return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
        }
        backV[i] = nullptr;
        if ( !context->currentMaterial->isSided() )
            backV[i] = getBackFaceVertex(v[i], context);
    }

    Vector3Dd normal;

    if ( !faceNormal(argc - 1, v, &normal) ) {
        MgfEntityControl::doWarning("degenerate face", context);
        return ParseErrorContext::MGF_OK; // Just ignore the generated face
    }
    if ( !context->currentMaterial->isSided() ) {
        Vector3Dd backNormal;
        backNormal = normal.multiply(-1.0);
    }

    int errorCode = ParseErrorContext::MGF_OK;

    Patch *face;
    Patch *twin;

    if ( argc == 4 ) {
        // Triangles
        face = newFace(v[0], v[1], v[2], nullptr, context);
        if ( !context->currentMaterial->isSided() && face != nullptr ) {
            twin = newFace(backV[2], backV[1], backV[0], nullptr, context);
            face->setTwin(twin);
            if ( twin != nullptr ) {
                twin->setTwin(face);
            }
        }
    } else if ( argc == 5 ) {
        // Quadrilaterals
        if ( context->inComplex || faceIsConvex(argc - 1, v, &normal) ) {
            face = newFace(v[0], v[1], v[2], v[3], context);
            if ( !context->currentMaterial->isSided() && face != nullptr ) {
                twin = newFace(backV[3], backV[2], backV[1], backV[0], context);
                face->setTwin(twin);
                if ( twin != nullptr ) {
                    twin->setTwin(face);
                }
            }
        } else {
            doComplexFace(argc - 1, v, &normal, backV, context);
            errorCode = ParseErrorContext::MGF_OK;
        }
    } else {
        // More than 4 vertices
        doComplexFace(argc - 1, v, &normal, backV, context);
        errorCode = ParseErrorContext::MGF_OK;
    }

    return errorCode;
}

int
MgfVertexFaceEntitySupport::handleSurfaceEntity(int argc, const char **argv, ParseRuntimeContext *context) {
    if ( context->inComplex ) {
        // mgfEntitySphere calls mgfEntityCone
        return doDiscreteConic(argc, argv, context);
    } else {
        context->inComplex = true;
        if ( context->inSurface ) {
            MgfObjectNameSupport::mgfObjectSurfaceDone(context);
        }
        MgfObjectNameSupport::mgfObjectNewSurface(context);
        MgfMaterialEntitySupport::mgfGetCurrentMaterial(&context->currentMaterial, context->singleSided, context);

        int errcode = doDiscreteConic(argc, argv, context);

        MgfObjectNameSupport::mgfObjectSurfaceDone(context);
        context->inComplex = false;

        return errcode;
    }
}

/**
Eliminates the holes by creating seems to the nearest vertex
on another contour. Creates an argument list for the face
without hole entity handling routine MgfVertexFaceEntitySupport::handleFaceEntity() and calls it
*/
int
MgfVertexFaceEntitySupport::handleFaceWithHolesEntity(int argc, const char **argv, ParseRuntimeContext *context) {
    Vector3Dd v[MAXIMUM_FACE_VERTICES + 1]; // v[i] = location of vertex argv[i]
    const char *argumentsToFaceWithoutHoles[MAXIMUM_FACE_VERTICES + 1]; // Arguments to be passed to the face
                                            // without hole entity handler
    char copied[MAXIMUM_FACE_VERTICES + 1]; // copied[i] is 1 or 0 indicating if
                                       // the vertex argv[i] has been copied to new contour
    int newContour[MAXIMUM_FACE_VERTICES]{}; // newContour[i] will contain the i-th
                                             // vertex of the face with eliminated holes
    int i;

    if ( argc - 1 > MAXIMUM_FACE_VERTICES ) {
        MgfEntityControl::doWarning(
                "too many vertices in face. Recompile the program with larger MAXIMUM_FACE_VERTICES constant in read mgf", context);
        return ParseErrorContext::MGF_OK; // No reason to stop parsing the input
    }

    // Get the location of the vertices: the location of the vertex
    // argv[i] is kept in v[i] (i=1 ... argc-1, and argv[i] not a contour
    // separator)
    for ( i = 1; i < argc; i++ ) {
        if ( *argv[i] == '-' ) {
            // Skip contour separators
            continue;
        }

        const VertexContext *vp = MgfVertexFaceEntitySupport::getNamedVertex(argv[i], context);
        if ( !vp ) {
            // Undefined vertex
            return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
        }
        MgfTransformationSupport::mgfTransformPoint(&v[i], &vp->p, context); // Transform with the current transform

        copied[i] = false; // Vertex not yet copied to argumentsToFaceWithoutHoles
    }

    // Copy the outer contour
    int numberOfVerticesInNewContour = 0;
    for ( i = 1; i < argc && *argv[i] != '-'; i++ ) {
        newContour[numberOfVerticesInNewContour++] = i;
        copied[i] = true;
    }

    // Find next not yet copied vertex in argv (i++ should suffice, but
    // this way we can also skip multiple "-"s ...)
    for ( ; i < argc; i++ ) {
        if ( *argv[i] == '-' ) {
            // Skip contour separators
            continue;
        }
        if ( !copied[i] ) {
            // Not yet copied vertex encountered
            break;
        }
    }

    while ( i < argc ) {
        // First i vertex of a hole that is not yet eliminated
        int nearestOther;
        int first;
        int last;
        int j;
        int k;

        // Find the not yet copied vertex that is nearest to the already
        // copied ones
        int nearestCopied = nearestOther = 0;
        double minimumDistance = Numeric::HUGE_DOUBLE_VALUE;
        for ( j = i; j < argc; j++ ) {
            if ( *argv[j] == '-' || copied[j] ) {
                // Contour separator or already copied vertex
                continue;
            }

            for ( k = 0; k < numberOfVerticesInNewContour; k++ ) {
                int index = newContour[k];
                double d = MgfTessellationMath::distanceSquared(v[j], v[index]);
                if ( d < minimumDistance ) {
                    minimumDistance = d;
                    nearestCopied = k;
                    nearestOther = j;
                }
            }
        }

        // Find first vertex of this nearest contour
        for ( first = nearestOther; *argv[first] != '-'; first-- ) {}
        first++;

        // Find last vertex in this nearest contour
        for ( last = nearestOther; last < argc && *argv[last] != '-'; last++ ) {}
        last--;

        // Number of vertices in the nearest contour
        int num = last - first + 1;

        // Create num+2 extra vertices in new contour
        if ( numberOfVerticesInNewContour + num + 2 > MAXIMUM_FACE_VERTICES ) {
            MgfEntityControl::doWarning(
                    "too many vertices in face. Recompile the program with larger MAXIMUM_FACE_VERTICES constant in read mgf", context);
            return ParseErrorContext::MGF_OK; // No reason to stop parsing the input
        }

        // Shift the elements in new contour starting at position nearestCopied
        // num+2 places further. Vertex new contour[nearestCopied] will be connected
        // to vertex nearestOther ... last, first ... nearestOther and
        // back to newContour[nearestCopied]
        for ( k = numberOfVerticesInNewContour - 1; k >= nearestCopied; k-- ) {
            newContour[k + num + 2] = newContour[k];
        }
        numberOfVerticesInNewContour += num + 2;

        // Insert the vertices of the nearest contour (closing the loop)
        k = nearestCopied + 1;
        for ( j = nearestOther; j <= last; j++ ) {
            newContour[k++] = j;
            copied[j] = true;
        }
        for ( j = first; j <= nearestOther && j <= MAXIMUM_FACE_VERTICES; j++ ) {
            newContour[k++] = j;
            copied[j] = true;
        }

        // Find next not yet copied vertex in argv
        for ( ; i < argc; i++ ) {
            if ( *argv[i] == '-' ) {
                continue;
            }
            // Skip contour separators
            if ( !copied[i] ) {
                // Not yet copied vertex encountered
                break;
            }
        }
    }

    // Build an argument list for the new polygon without holes
    argumentsToFaceWithoutHoles[0] = "f";
    for ( i = 0; i < numberOfVerticesInNewContour; i++ ) {
        argumentsToFaceWithoutHoles[i + 1] = argv[newContour[i]];
    }

    // And handle the face without holes
    return MgfVertexFaceEntitySupport::handleFaceEntity(numberOfVerticesInNewContour + 1, argumentsToFaceWithoutHoles, context);
}

/**
Handle a vertex entity
*/
int
MgfVertexFaceEntitySupport::handleVertexEntity(int ac, const char **av, ParseRuntimeContext *context) {
    LookUpEntity<char *> *lp;
    VertexContext *&currentVertexContext = context->vertexRepository.currentVertex;
    LookUpTable<char *> *vertexLookUpTable = context->vertexRepository.vertexLookUpTable;

    switch ( MgfEntityControl::mgfEntity(av[0], context) ) {
        case EntityTypeContext::VERTEX:
            // get/set vertex context
            if ( ac > 4 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed vertex context
                context->vertexRepository.unNamedVertexContext = context->vertexRepository.defaultVertexContext;
                currentVertexContext = &context->vertexRepository.unNamedVertexContext;
                context->currentVertexName = nullptr;
                return ParseErrorContext::MGF_OK;
            }
            if ( !TokenValidationContext::isName(av[1]) ) {
                return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = vertexLookUpTable->lookUpFind(av[1]);
            // Lookup context
            if ( lp == nullptr ) {
                return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
            }
            context->currentVertexName = lp->key;
            currentVertexContext = reinterpret_cast<VertexContext *>(lp->data);
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( currentVertexContext == nullptr) {
                    return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return ParseErrorContext::MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( currentVertexContext == nullptr  ) {
                // Create new vertex context
                context->currentVertexName = new char[strlen(av[1]) + 1];
                if ( context->currentVertexName == nullptr ) {
                    return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(context->currentVertexName, av[1]);
                lp->key = context->currentVertexName;
                currentVertexContext = reinterpret_cast<VertexContext *>(new char[sizeof(VertexContext)]);
                if ( currentVertexContext == nullptr ) {
                    return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
                }
                lp->data = reinterpret_cast<char *>(currentVertexContext);
            }
            if ( ac == 3 ) {
                // Use default template
                *currentVertexContext = context->vertexRepository.defaultVertexContext;
                return ParseErrorContext::MGF_OK;
            }
            lp = vertexLookUpTable->lookUpFind(av[3]);
            // Lookup template
            if ( lp == nullptr) {
                return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return ParseErrorContext::MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *currentVertexContext = *reinterpret_cast<VertexContext *>(lp->data);
            currentVertexContext->clock++;
            return ParseErrorContext::MGF_OK;
        case EntityTypeContext::MGF_POINT:
            // Set point
            if ( ac != 4 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) || !TokenValidationContext::isFloat(av[3]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            currentVertexContext->p = Vector3Dd(strtod(av[1], nullptr), strtod(av[2], nullptr), strtod(av[3], nullptr));
            currentVertexContext->clock++;
            return ParseErrorContext::MGF_OK;
        case EntityTypeContext::MGF_NORMAL:
            // Set normal
            if ( ac != 4 ) {
                return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !TokenValidationContext::isFloat(av[1]) || !TokenValidationContext::isFloat(av[2]) || !TokenValidationContext::isFloat(av[3]) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            currentVertexContext->n = Vector3Dd(strtod(av[1], nullptr), strtod(av[2], nullptr), strtod(av[3], nullptr));
            MgfTessellationMath::normalizeAndGivePreviousNorm(&currentVertexContext->n, VSDK::EPSILON);
            currentVertexContext->clock++;
            return ParseErrorContext::MGF_OK;
        default:
            break;
    }
    return ParseErrorContext::MGF_ERROR_UNKNOWN_ENTITY;
}

/**
Get a named vertex
*/
VertexContext *
MgfVertexFaceEntitySupport::getNamedVertex(const char *name, ParseRuntimeContext *context) {
    LookUpEntity<char *> *lp = context->vertexRepository.vertexLookUpTable->lookUpFind(name);

    if ( lp == nullptr ) {
        return nullptr;
    }
    return reinterpret_cast<VertexContext *>(lp->data);
}

void
MgfVertexFaceEntitySupport::initGeometryContextTables(ParseRuntimeContext *context) {
    context->vertexRepository.reset();
    context->currentVertexName = nullptr;
}
