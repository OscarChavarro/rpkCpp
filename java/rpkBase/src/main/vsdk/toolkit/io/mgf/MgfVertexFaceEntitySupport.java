package vsdk.toolkit.io.mgf;

import java.util.ArrayList;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.dataStructures.LookUpEntity;
import vsdk.toolkit.common.linealAlgebra.CoordinateAxis;
import vsdk.toolkit.skin.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.TokenValidationContext;
import vsdk.toolkit.io.context.TransformStackContext;
import vsdk.toolkit.io.context.VertexContext;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RpkVertex;

public class MgfVertexFaceEntitySupport {
    private static final int MAXIMUM_FACE_VERTICES = 100;

    private static long transformXid(TransformStackContext xf) {
        return xf == null ? 0L : xf.xid;
    }

    /**
Turns a surface primitive (sphere, torus, cylinder, ring, cone or prism)
into faces, through the VITRAL geometry of MgfVitralPrimitiveTessellator
    */
    private static int doDiscreteConic(int argc, String[] argv, ParseRuntimeContext context) {
        int en = MgfEntityControl.mgfEntity(argv[0], context);

        switch (en) {
            case EntityTypeContext.SPHERE:
                return MgfVitralPrimitiveTessellator.handleSphere(argc, argv, context);
            case EntityTypeContext.TORUS:
                return MgfVitralPrimitiveTessellator.handleTorus(argc, argv, context);
            case EntityTypeContext.CYLINDER:
                return MgfVitralPrimitiveTessellator.handleCylinder(argc, argv, context);
            case EntityTypeContext.RING:
                return MgfVitralPrimitiveTessellator.handleRing(argc, argv, context);
            case EntityTypeContext.CONE:
                return MgfVitralPrimitiveTessellator.handleCone(argc, argv, context);
            case EntityTypeContext.PRISM:
                return MgfVitralPrimitiveTessellator.handlePrism(argc, argv, context);
            default:
                Logger.reportMessage(null, Logger.FATAL_ERROR, "mgf.c: doDiscreteConic", String.format("Unsupported geometry entity number %d", en));
                return ParseErrorContext.MGF_ERROR_UNKNOWN_ENTITY;
        }
    }

    private static Vector3Dd installPoint(double x, double y, double z, ParseRuntimeContext context) {
        Vector3Dd coord = new Vector3Dd(x, y, z);
        context.currentPointList.add(coord);
        return coord;
    }

    private static Vector3Dd installNormal(double x, double y, double z, ParseRuntimeContext context) {
        Vector3Dd norm = new Vector3Dd(x, y, z);
        context.currentNormalList.add(norm);
        return norm;
    }

    private static RpkVertex installVertex(Vector3Dd coord, Vector3Dd norm, ParseRuntimeContext context) {
        ArrayList<Patch> newPatchList = new ArrayList<Patch>();
        RpkVertex v = new RpkVertex(coord, norm, null, newPatchList);
        context.currentVertexList.add(v);
        return v;
    }

    private static RpkVertex getVertex(String name, ParseRuntimeContext context) {
        VertexContext vp = MgfVertexFaceEntitySupport.getNamedVertex(name, context);
        if (vp == null) {
            return null;
        }

        RpkVertex theVertex = vp.vertex;
        if (theVertex == null
         || vp.clock >= 1
         || vp.xid != MgfVertexFaceEntitySupport.transformXid(context.transformContext)
         || MgfTessellationMath.isNull(vp.n, VSDK.EPSILON)) {
            // New vertex, or updated vertex or same vertex, but other transform, or
            // vertex without normal: create a new Vertex
            Vector3Dd theNormal;

            Vector3Dd vert = MgfTransformationSupport.mgfTransformPoint(vp.p, context);
            Vector3Dd thePoint = installPoint((double)vert.x(), (double)vert.y(), (double)vert.z(), context);
            if (MgfTessellationMath.isNull(vp.n, VSDK.EPSILON)) {
                theNormal = null;
            } else {
                Vector3Dd norm = MgfTransformationSupport.mgfTransformVector(vp.n, context);
                theNormal = installNormal((double)norm.x(), (double)norm.y(), (double)norm.z(), context);
            }
            theVertex = installVertex(thePoint, theNormal, context);
            vp.vertex = theVertex;
            vp.xid = MgfVertexFaceEntitySupport.transformXid(context.transformContext);
        }
        vp.clock = 0;

        return theVertex;
    }

    /**
Create a vertex with given name, but with reversed normal as
the given vertex. For back-faces of two-sided surfaces
    */
    private static RpkVertex getBackFaceVertex(RpkVertex v, ParseRuntimeContext context) {
        RpkVertex back = v.back;

        if (back == null) {
            Vector3Dd point = v.point;
            Vector3Dd normal = v.normal;
            if (normal != null) {
                normal = installNormal(-normal.x(), -normal.y(), -normal.z(), context);
            }

            back = installVertex(point, normal, context);
            v.back = back;
            back.back = v;
        }

        return back;
    }

    private static Patch newFace(RpkVertex v1, RpkVertex v2, RpkVertex v3, RpkVertex v4, ParseRuntimeContext context) {
        Patch theFace;
        int numberOfVertices = v4 != null ? 4 : 3;

        if (v1 == null || v2 == null || v3 == null) {
            return null;
        }

        if (context.transformContext != null && context.transformContext.rev != 0) {
            theFace = new Patch(numberOfVertices, v3, v2, v1, v4);
        } else {
            theFace = new Patch(numberOfVertices, v1, v2, v3, v4);
        }

        // If we are doing radiance computations, create radiance data for the patch
        if (theFace.getMaterial() != null && context.radianceMethod != null) {
            context.radianceMethod.createPatchData(theFace);
        }

        context.currentFaceList.add(theFace);

        return theFace;
    }

    /**
Computes the normal to the patch plane (null for a degenerate polygon)
    */
    private static Vector3Dd faceNormal(int numberOfVertices, RpkVertex[] v) {
        Vector3Dd n = new Vector3Dd(0, 0, 0);
        Vector3Dd cur = v[numberOfVertices - 1].point.subtract(v[0].point);
        for (int i = 0; i < numberOfVertices; i++) {
            Vector3Dd prev = cur;
            cur = v[i].point.subtract(v[0].point);
            n = new Vector3Dd(
                n.x() + ((prev.y() - cur.y()) * (prev.z() + cur.z())),
                n.y() + ((prev.z() - cur.z()) * (prev.x() + cur.x())),
                n.z() + ((prev.x() - cur.x()) * (prev.y() + cur.y())));
        }
        double localNorm = n.length();

        if (localNorm < VSDK.EPSILON) {
            // Degenerate normal --> degenerate polygon
            return null;
        }
        return n.multiplyByInverse((double)localNorm, Numeric.EPSILON);
    }

    /**
Given a vector p in 3D space and an index i, which is X, Y
or Z, projects the vector on the YZ, XZ or XY plane respectively
    */
    private static Vector2Dd vectorProject(Vector2Dd r, Vector3Dd p, CoordinateAxis i) {
        switch (i) {
            case X:
                return new Vector2Dd(p.y(), p.z());
            case Y:
                return new Vector2Dd(p.x(), p.z());
            case Z:
                return new Vector2Dd(p.x(), p.y());
            default:
                return r;
        }
    }

    /**
Tests whether the polygon is convex or concave. This is accomplished by projecting
onto the coordinate plane "most parallel" to the polygon and checking the signs
the cross product of succeeding edges: the signs are all equal for a convex polygon
    */
    private static boolean faceIsConvex(int numberOfVertices, RpkVertex[] v, Vector3Dd normal) {
        Vector2Dd[] v2d = new Vector2Dd[MAXIMUM_FACE_VERTICES + 1];
        for (int i = 0; i < v2d.length; i++) {
            v2d[i] = new Vector2Dd();
        }
        Vector2Dd p = new Vector2Dd();
        Vector2Dd c = new Vector2Dd();
        int i;

        CoordinateAxis index = CoordinateAxis.values()[normal.dominantAxis()];
        for (i = 0; i < numberOfVertices; i++) {
            v2d[i] = vectorProject(v2d[i], v[i].point, index);
        }

        p = p.withX(v2d[3].x() - v2d[2].x());
        p = p.withY(v2d[3].y() - v2d[2].y());
        c = c.withX(v2d[0].x() - v2d[3].x());
        c = c.withY(v2d[0].y() - v2d[3].y());
        int sign = (p.x() * c.y() > c.x() * p.y()) ? 1 : -1;

        for (i = 1; i < numberOfVertices; i++) {
            p = p.withX(c.x());
            p = p.withY(c.y());
            c = c.withX(v2d[i].x() - v2d[i - 1].x());
            c = c.withY(v2d[i].y() - v2d[i - 1].y());
            if (((p.x() * c.y() > c.x() * p.y()) ? 1 : -1) != sign) {
                return false;
            }
        }

        return true;
    }

    /**
Returns true if the 2D point p is inside the 2D triangle p1-p2-p3
    */
    private static boolean pointInsideTriangle2D(Vector2Dd p, Vector2Dd p1, Vector2Dd p2, Vector2Dd p3) {
        // From Graphics Gems I, Didier Badouel, An Efficient Ray-Polygon Intersection, p390
        double u0 = p.x() - p1.x();
        double v0 = p.y() - p1.y();
        double u1 = p2.x() - p1.x();
        double v1 = p2.y() - p1.y();
        double u2 = p3.x() - p1.x();
        double v2 = p3.y() - p1.y();

        double a = 10.0;
        double b = 10.0; // Values large enough so the result would be false
        if (Math.abs(u1) < VSDK.EPSILON) {
            if (Math.abs(u2) > VSDK.EPSILON && Math.abs(v1) > VSDK.EPSILON) {
                b = u0 / u2;
                if (b < VSDK.EPSILON || b > 1.0 - VSDK.EPSILON) {
                    return false;
                } else {
                    a = (v0 - b * v2) / v1;
                }
            }
        } else {
            b = v2 * u1 - u2 * v1;
            if (Math.abs(b) > VSDK.EPSILON) {
                b = (v0 * u1 - u0 * v1) / b;
                if (b < VSDK.EPSILON || b > 1.0 - VSDK.EPSILON) {
                    return false;
                } else {
                    a = (u0 - b * u2) / u1;
                }
            }
        }

        return (a >= VSDK.EPSILON && a <= 1.0 - VSDK.EPSILON && (a + b) <= 1.0 - VSDK.EPSILON);
    }

    /**
Returns true if the 2D segments p1-p2 and p3-p4 intersect
    */
    private static boolean segmentsIntersect2D(Vector2Dd p1, Vector2Dd p2, Vector2Dd p3, Vector2Dd p4) {
        double a;
        double b;
        double c;
        boolean coLinear = false;

        // From Graphics Gems II, Mukesh Prasad, Intersection of Line Segments, p7
        double du = Math.abs(p2.x() - p1.x());
        double dv = Math.abs(p2.y() - p1.y());
        if (du > VSDK.EPSILON || dv > VSDK.EPSILON) {
            if (dv > du) {
                a = 1.0;
                b = -(p2.x() - p1.x()) / (p2.y() - p1.y());
                c = -(p1.x() + b * p1.y());
            } else {
                a = -(p2.y() - p1.y()) / (p2.x() - p1.x());
                b = 1.0;
                c = -(a * p1.x() + p1.y());
            }

            double r3 = a * p3.x() + b * p3.y() + c;
            double r4 = a * p4.x() + b * p4.y() + c;

            if (Math.abs(r3) < VSDK.EPSILON && Math.abs(r4) < VSDK.EPSILON) {
                coLinear = true;
            } else if ((r3 > -VSDK.EPSILON && r4 > -VSDK.EPSILON) || (r3 < VSDK.EPSILON && r4 < VSDK.EPSILON)) {
                return false;
            }
        }

        if (coLinear == false) {
            du = Math.abs(p4.x() - p3.x());
            dv = Math.abs(p4.y() - p3.y());
            if (du > VSDK.EPSILON || dv > VSDK.EPSILON) {
                if (dv > du) {
                    a = 1.0;
                    b = -(p4.x() - p3.x()) / (p4.y() - p3.y());
                    c = -(p3.x() + b * p3.y());
                } else {
                    a = -(p4.y() - p3.y()) / (p4.x() - p3.x());
                    b = 1.0;
                    c = -(a * p3.x() + p3.y());
                }

                double r1 = a * p1.x() + b * p1.y() + c;
                double r2 = a * p2.x() + b * p2.y() + c;

                if (Math.abs(r1) < VSDK.EPSILON && Math.abs(r2) < VSDK.EPSILON) {
                    coLinear = true;
                } else if ((r1 > -VSDK.EPSILON && r2 > -VSDK.EPSILON) || (r1 < VSDK.EPSILON && r2 < VSDK.EPSILON)) {
                    return false;
                }
            }
        }

        if (coLinear == false) {
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
    private static void doComplexFace(int n, RpkVertex[] v, Vector3Dd normal, RpkVertex[] backVertex, ParseRuntimeContext context) {
        Vector3Dd center = new Vector3Dd();
        center = new Vector3Dd(0.0, 0.0, 0.0);
        for (int i = 0; i < n; i++) {
            center = center.add(v[i].point);
        }
        center = center.multiplyByInverse((double)n, Numeric.EPSILON);

        double maxD = center.distance(v[0].point);
        int max = 0;
        for (int i = 1; i < n; i++) {
            double d = center.distance(v[i].point);
            if (d > maxD) {
                maxD = d;
                max = i;
            }
        }

        boolean[] out = new boolean[MAXIMUM_FACE_VERTICES + 1];
        for (int i = 0; i < n; i++) {
            out[i] = false;
        }

        int p1 = max;
        int p0 = p1 - 1;
        if (p0 < 0) {
            p0 = n - 1;
        }
        int p2 = (p1 + 1) % n;
        normal = v[p2].point.subtract(v[p1].point).crossProduct(v[p0].point.subtract(v[p1].point));
        normal = normal.normalized(Numeric.EPSILON);
        CoordinateAxis index = CoordinateAxis.values()[normal.dominantAxis()];

        Vector2Dd[] q = new Vector2Dd[MAXIMUM_FACE_VERTICES + 1];
        for (int i = 0; i < n; i++) {
            q[i] = new Vector2Dd();
            q[i] = vectorProject(q[i], v[i].point, index);
        }

        int corners = n;
        Vector3Dd nn = new Vector3Dd();

        p0 = -1;
        while (corners >= 3) {
            int start = p0;
            double d;
            double a = 0.0;
            boolean good;

            do {
                p0 = (p0 + 1) % n;
                while (out[p0]) {
                    p0 = (p0 + 1) % n;
                }

                p1 = (p0 + 1) % n;
                while (out[p1]) {
                    p1 = (p1 + 1) % n;
                }

                p2 = (p1 + 1) % n;
                while (out[p2]) {
                    p2 = (p2 + 1) % n;
                }

                if (p0 == start) {
                    break;
                }

                nn = v[p2].point.subtract(v[p1].point).crossProduct(v[p0].point.subtract(v[p1].point));
                a = nn.length();
                nn = nn.multiplyByInverse((double)a, Numeric.EPSILON);
                d = nn.distance(normal);

                good = true;
                if (d <= 1.0) {
                    for (int i = 0; i < n && good; i++) {
                        if (out[i] || v[i] == v[p0] || v[i] == v[p1] || v[i] == v[p2]) {
                            continue;
                        }

                        if (pointInsideTriangle2D(q[i], q[p0], q[p1], q[p2])) {
                            good = false;
                        }

                        int j = (i + 1) % n;
                        if (out[j] || v[j] == v[p0]) {
                            continue;
                        }

                        if (segmentsIntersect2D(q[p2], q[p0], q[i], q[j])) {
                            good = false;
                        }
                    }
                }
            } while (d > 1.0 || !good);

            if (p0 == start) {
                MgfEntityControl.doError("mis-built polygonal face", context);
                return;
            }

            if (Math.abs(a) > VSDK.EPSILON) {
                // Avoid degenerate faces
                Patch face = newFace(v[p0], v[p1], v[p2], null, context);
                if (context.currentMaterial.isSided() == false && face != null) {
                    Patch twin = newFace(backVertex[p2], backVertex[p1], backVertex[p0], null, context);
                    face.setTwin(twin);
                    if (twin != null) {
                        twin.setTwin(face);
                    }
                }
            }

            out[p1] = true;
            corners--;
        }
    }

    public static int handleFaceEntity(int argc, String[] argv, ParseRuntimeContext context) {
        if (argc < 4) {
            MgfEntityControl.doError("too few vertices in face", context);
            return ParseErrorContext.MGF_OK; // Don't stop parsing the input
        }

        if (argc - 1 > MAXIMUM_FACE_VERTICES) {
            MgfEntityControl.doWarning(
                "too many vertices in face. Recompile the program with larger MAXIMUM_FACE_VERTICES constant in read mgf",
                context);
            return ParseErrorContext.MGF_OK; // No reason to stop parsing the input
        }

        if (context.inComplex == false && MgfMaterialEntitySupport.mgfMaterialChanged(context.currentMaterial, context)) {
            if (context.inSurface) {
                MgfObjectNameSupport.mgfObjectSurfaceDone(context);
            }
            MgfObjectNameSupport.mgfObjectNewSurface(context);
            RpkMaterial[] holder = new RpkMaterial[] {context.currentMaterial};
            MgfMaterialEntitySupport.mgfGetCurrentMaterial(holder, context.singleSided, context);
            context.currentMaterial = holder[0];
            context.materialState.currentMaterial = holder[0];
        }

        RpkVertex[] v = new RpkVertex[MAXIMUM_FACE_VERTICES + 1];
        RpkVertex[] backV = new RpkVertex[MAXIMUM_FACE_VERTICES + 1];

        for (int i = 0; i < argc - 1; i++) {
            v[i] = getVertex(argv[i + 1], context);
            if (v[i] == null) {
                // This is however a reason to stop parsing the input
                return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
            }
            backV[i] = null;
            if (context.currentMaterial.isSided() == false) {
                backV[i] = getBackFaceVertex(v[i], context);
            }
        }

        Vector3Dd normal = faceNormal(argc - 1, v);

        if (normal == null) {
            MgfEntityControl.doWarning("degenerate face", context);
            return ParseErrorContext.MGF_OK; // Just ignore the generated face
        }

        int errorCode = ParseErrorContext.MGF_OK;

        Patch face;
        Patch twin;

        if (argc == 4) {
            // Triangles
            face = newFace(v[0], v[1], v[2], null, context);
            if (context.currentMaterial.isSided() == false && face != null) {
                twin = newFace(backV[2], backV[1], backV[0], null, context);
                face.setTwin(twin);
                if (twin != null) {
                    twin.setTwin(face);
                }
            }
        } else if (argc == 5) {
            // Quadrilaterals
            if (context.inComplex || faceIsConvex(argc - 1, v, normal)) {
                face = newFace(v[0], v[1], v[2], v[3], context);
                if (context.currentMaterial.isSided() == false && face != null) {
                    twin = newFace(backV[3], backV[2], backV[1], backV[0], context);
                    face.setTwin(twin);
                    if (twin != null) {
                        twin.setTwin(face);
                    }
                }
            } else {
                doComplexFace(argc - 1, v, normal, backV, context);
                errorCode = ParseErrorContext.MGF_OK;
            }
        } else {
            // More than 4 vertices
            doComplexFace(argc - 1, v, normal, backV, context);
            errorCode = ParseErrorContext.MGF_OK;
        }

        return errorCode;
    }

    public static int handleSurfaceEntity(int argc, String[] argv, ParseRuntimeContext context) {
        if (context.inComplex) {
            // mgfEntitySphere calls mgfEntityCone
            return doDiscreteConic(argc, argv, context);
        } else {
            context.inComplex = true;
            context.geometryBuildState.inComplex = true;
            if (context.inSurface) {
                MgfObjectNameSupport.mgfObjectSurfaceDone(context);
            }
            MgfObjectNameSupport.mgfObjectNewSurface(context);
            RpkMaterial[] holder = new RpkMaterial[] {context.currentMaterial};
            MgfMaterialEntitySupport.mgfGetCurrentMaterial(holder, context.singleSided, context);
            context.currentMaterial = holder[0];
            context.materialState.currentMaterial = holder[0];

            int errcode = doDiscreteConic(argc, argv, context);

            MgfObjectNameSupport.mgfObjectSurfaceDone(context);
            context.inComplex = false;
            context.geometryBuildState.inComplex = false;

            return errcode;
        }
    }

    /**
Eliminates the holes by creating seems to the nearest vertex
on another contour. Creates an argument list for the face
without hole entity handling routine MgfVertexFaceEntitySupport::handleFaceEntity() and calls it
    */
    public static int handleFaceWithHolesEntity(int argc, String[] argv, ParseRuntimeContext context) {
        // Delegate to existing simplified seam generator implementation.
        return MgfFaceWithHolesEntityExpander.handleEntity(argc, argv, context);
    }

    /**
    Handle a vertex entity
    */
    public static int handleVertexEntity(int ac, String[] av, ParseRuntimeContext context) {
        LookUpEntity<VertexContext> lp;
        VertexContext currentVertexContext = context.vertexRepository.currentVertex;

        switch (MgfEntityControl.mgfEntity(av[0], context)) {
            case EntityTypeContext.VERTEX:
                // get/set vertex context
                if (ac > 4) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (ac == 1) {
                    // Set unnamed vertex context
                    context.vertexRepository.unNamedVertexContext.copy(context.vertexRepository.defaultVertexContext);
                    currentVertexContext = context.vertexRepository.unNamedVertexContext;
                    context.vertexRepository.currentVertex = currentVertexContext;
                    context.currentVertexName = null;
                    context.geometryBuildState.currentVertexName = null;
                    return ParseErrorContext.MGF_OK;
                }
                if (TokenValidationContext.isName(av[1]) == false) {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                lp = context.vertexRepository.vertexLookUpTable.lookUpFind(av[1]);
                // Lookup context
                if (lp == null) {
                    return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
                }
                context.currentVertexName = lp.key;
                context.geometryBuildState.currentVertexName = context.currentVertexName;
                currentVertexContext = lp.data;
                context.vertexRepository.currentVertex = currentVertexContext;
                if (ac == 2) {
                    // Re-establish previous context
                    if (currentVertexContext == null) {
                        return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
                    }
                    return ParseErrorContext.MGF_OK;
                }
                if (av[2].length() != 1 || av[2].charAt(0) != '=') {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                if (currentVertexContext == null) {
                    // Create new vertex context
                    context.currentVertexName = av[1];
                    context.geometryBuildState.currentVertexName = context.currentVertexName;
                    lp.key = context.currentVertexName;
                    currentVertexContext = new VertexContext();
                    lp.data = currentVertexContext;
                    context.vertexRepository.currentVertex = currentVertexContext;
                }
                if (ac == 3) {
                    // Use default template
                    currentVertexContext.copy(context.vertexRepository.defaultVertexContext);
                    return ParseErrorContext.MGF_OK;
                }
                lp = context.vertexRepository.vertexLookUpTable.lookUpFind(av[3]);
                // Lookup template
                if (lp == null) {
                    return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
                }
                if (lp.data == null) {
                    return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
                }
                currentVertexContext.copy(lp.data);
                currentVertexContext.clock++;
                return ParseErrorContext.MGF_OK;
            case EntityTypeContext.MGF_POINT:
                // Set point
                if (ac != 4) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (TokenValidationContext.isFloat(av[1]) == false || TokenValidationContext.isFloat(av[2]) == false || TokenValidationContext.isFloat(av[3]) == false) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                currentVertexContext.p = new Vector3Dd(Double.parseDouble(av[1]), Double.parseDouble(av[2]), Double.parseDouble(av[3]));
                currentVertexContext.clock++;
                return ParseErrorContext.MGF_OK;
            case EntityTypeContext.MGF_NORMAL:
                // Set normal
                if (ac != 4) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (TokenValidationContext.isFloat(av[1]) == false || TokenValidationContext.isFloat(av[2]) == false || TokenValidationContext.isFloat(av[3]) == false) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                Vector3Dd[] normalHolder = new Vector3Dd[] {
                    new Vector3Dd(Double.parseDouble(av[1]), Double.parseDouble(av[2]), Double.parseDouble(av[3]))
                };
                MgfTessellationMath.normalizeAndGivePreviousNorm(normalHolder, VSDK.EPSILON);
                currentVertexContext.n = normalHolder[0];
                currentVertexContext.clock++;
                return ParseErrorContext.MGF_OK;
            default:
                break;
        }
        return ParseErrorContext.MGF_ERROR_UNKNOWN_ENTITY;
    }

    /**
    Get a named vertex
    */
    public static VertexContext getNamedVertex(String name, ParseRuntimeContext context) {
        LookUpEntity<VertexContext> lp = context.vertexRepository.vertexLookUpTable.lookUpFind(name);

        if (lp == null) {
            return null;
        }
        return lp.data;
    }

    public static void initGeometryContextTables(ParseRuntimeContext context) {
        context.vertexRepository.reset();
        context.currentVertexName = null;
        context.geometryBuildState.currentVertexName = null;
    }
}
