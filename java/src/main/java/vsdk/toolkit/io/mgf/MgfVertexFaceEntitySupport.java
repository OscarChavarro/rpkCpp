package vsdk.toolkit.io.mgf;

import java.util.ArrayList;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.dataStructures.LookUpEntity;
import vsdk.toolkit.common.linealAlgebra.CoordinateAxis;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector2D;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.TokenValidationContext;
import vsdk.toolkit.io.context.TransformStackContext;
import vsdk.toolkit.io.context.VertexContext;
import vsdk.toolkit.material.Material;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.Vertex;

public class MgfVertexFaceEntitySupport {
    private static final int MAXIMUM_FACE_VERTICES = 100;

    private static long transformXid(TransformStackContext xf) {
        return xf == null ? 0L : xf.xid;
    }

    /**
The mgf parser already contains some good routines for discrete spheres / cone / cylinder / torus
into polygons. In the official release of the parser library, these routines
are internal (declared static in parse.c and no reference to them in parser.h).
The parser was changed so we can call them in order not to have to duplicate
the code
    */
    private static int doDiscreteConic(int argc, String[] argv, ParseRuntimeContext context) {
        int en = MgfEntityControl.mgfEntity(argv[0], context);

        switch (en) {
            case EntityTypeContext.SPHERE:
                return MgfSphereEntityExpander.handleEntity(argc, argv, context);
            case EntityTypeContext.TORUS:
                return MgfTorusEntityExpander.handleEntity(argc, argv, context);
            case EntityTypeContext.CYLINDER:
                return MgfCylinderEntityExpander.handleEntity(argc, argv, context);
            case EntityTypeContext.RING:
                return MgfRingEntityTessellator.handleEntity(argc, argv, context);
            case EntityTypeContext.CONE:
                return MgfConeEntityTessellator.handleEntity(argc, argv, context);
            case EntityTypeContext.PRISM:
                return MgfPrismEntityTessellator.handleEntity(argc, argv, context);
            default:
                Error.fatal(4, "mgf.c: doDiscreteConic", "Unsupported geometry entity number %d", en);
                return ParseErrorContext.MGF_ERROR_UNKNOWN_ENTITY;
        }
    }

    private static Vector3D installPoint(float x, float y, float z, ParseRuntimeContext context) {
        Vector3D coord = new Vector3D(x, y, z);
        context.currentPointList.add(coord);
        return coord;
    }

    private static Vector3D installNormal(float x, float y, float z, ParseRuntimeContext context) {
        Vector3D norm = new Vector3D(x, y, z);
        context.currentNormalList.add(norm);
        return norm;
    }

    private static Vertex installVertex(Vector3D coord, Vector3D norm, ParseRuntimeContext context) {
        ArrayList<Patch> newPatchList = new ArrayList<Patch>();
        Vertex v = new Vertex(coord, norm, null, newPatchList);
        context.currentVertexList.add(v);
        return v;
    }

    private static Vertex getVertex(String name, ParseRuntimeContext context) {
        VertexContext vp = MgfVertexFaceEntitySupport.getNamedVertex(name, context);
        if (vp == null) {
            return null;
        }

        Vertex theVertex = vp.vertex;
        if (theVertex == null
         || vp.clock >= 1
         || vp.xid != MgfVertexFaceEntitySupport.transformXid(context.transformContext)
         || vp.n.isNull(Numeric.EPSILON)) {
            // New vertex, or updated vertex or same vertex, but other transform, or
            // vertex without normal: create a new Vertex
            Vector3Dd vert = new Vector3Dd();
            Vector3Dd norm = new Vector3Dd();
            Vector3D theNormal;

            MgfTransformationSupport.mgfTransformPoint(vert, vp.p, context);
            Vector3D thePoint = installPoint((float)vert.x, (float)vert.y, (float)vert.z, context);
            if (vp.n.isNull(Numeric.EPSILON)) {
                theNormal = null;
            } else {
                MgfTransformationSupport.mgfTransformVector(norm, vp.n, context);
                theNormal = installNormal((float)norm.x, (float)norm.y, (float)norm.z, context);
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
    private static Vertex getBackFaceVertex(Vertex v, ParseRuntimeContext context) {
        Vertex back = v.back;

        if (back == null) {
            Vector3D point = v.point;
            Vector3D normal = v.normal;
            if (normal != null) {
                normal = installNormal(-normal.x, -normal.y, -normal.z, context);
            }

            back = installVertex(point, normal, context);
            v.back = back;
            back.back = v;
        }

        return back;
    }

    private static Patch newFace(Vertex v1, Vertex v2, Vertex v3, Vertex v4, ParseRuntimeContext context) {
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
        if (theFace.material != null && context.radianceMethod != null) {
            context.radianceMethod.createPatchData(theFace);
        }

        context.currentFaceList.add(theFace);

        return theFace;
    }

    /**
Computes the normal to the patch plane
    */
    private static Vector3D faceNormal(int numberOfVertices, Vertex[] v, Vector3D normal) {
        Vector3D cur = new Vector3D();
        Vector3D n = new Vector3D();

        n.set(0, 0, 0);
        cur.subtraction(v[numberOfVertices - 1].point, v[0].point);
        for (int i = 0; i < numberOfVertices; i++) {
            Vector3D prev = new Vector3D(cur.x, cur.y, cur.z);
            cur.subtraction(v[i].point, v[0].point);
            n.x += (prev.y - cur.y) * (prev.z + cur.z);
            n.y += (prev.z - cur.z) * (prev.x + cur.x);
            n.z += (prev.x - cur.x) * (prev.y + cur.y);
        }
        double localNorm = n.norm();

        if (localNorm < Numeric.EPSILON) {
            // Degenerate normal --> degenerate polygon
            return null;
        }
        n.inverseScaledCopy((float)localNorm, n, Numeric.EPSILON_FLOAT);
        normal.copy(n);

        return normal;
    }

    /**
Given a vector p in 3D space and an index i, which is X, Y
or Z, projects the vector on the YZ, XZ or XY plane respectively
    */
    private static void vectorProject(Vector2D r, Vector3D p, CoordinateAxis i) {
        switch (i) {
            case X:
                r.x = p.y;
                r.y = p.z;
                break;
            case Y:
                r.x = p.x;
                r.y = p.z;
                break;
            case Z:
                r.x = p.x;
                r.y = p.y;
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
    private static boolean faceIsConvex(int numberOfVertices, Vertex[] v, Vector3D normal) {
        Vector2D[] v2d = new Vector2D[MAXIMUM_FACE_VERTICES + 1];
        for (int i = 0; i < v2d.length; i++) {
            v2d[i] = new Vector2D();
        }
        Vector2D p = new Vector2D();
        Vector2D c = new Vector2D();
        int i;

        CoordinateAxis index = normal.dominantCoordinate();
        for (i = 0; i < numberOfVertices; i++) {
            vectorProject(v2d[i], v[i].point, index);
        }

        p.x = v2d[3].x - v2d[2].x;
        p.y = v2d[3].y - v2d[2].y;
        c.x = v2d[0].x - v2d[3].x;
        c.y = v2d[0].y - v2d[3].y;
        int sign = (p.x * c.y > c.x * p.y) ? 1 : -1;

        for (i = 1; i < numberOfVertices; i++) {
            p.x = c.x;
            p.y = c.y;
            c.x = v2d[i].x - v2d[i - 1].x;
            c.y = v2d[i].y - v2d[i - 1].y;
            if (((p.x * c.y > c.x * p.y) ? 1 : -1) != sign) {
                return false;
            }
        }

        return true;
    }

    /**
Returns true if the 2D point p is inside the 2D triangle p1-p2-p3
    */
    private static boolean pointInsideTriangle2D(Vector2D p, Vector2D p1, Vector2D p2, Vector2D p3) {
        // From Graphics Gems I, Didier Badouel, An Efficient Ray-Polygon Intersection, p390
        double u0 = p.x - p1.x;
        double v0 = p.y - p1.y;
        double u1 = p2.x - p1.x;
        double v1 = p2.y - p1.y;
        double u2 = p3.x - p1.x;
        double v2 = p3.y - p1.y;

        double a = 10.0;
        double b = 10.0; // Values large enough so the result would be false
        if (Math.abs(u1) < Numeric.EPSILON) {
            if (Math.abs(u2) > Numeric.EPSILON && Math.abs(v1) > Numeric.EPSILON) {
                b = u0 / u2;
                if (b < Numeric.EPSILON || b > 1.0 - Numeric.EPSILON) {
                    return false;
                } else {
                    a = (v0 - b * v2) / v1;
                }
            }
        } else {
            b = v2 * u1 - u2 * v1;
            if (Math.abs(b) > Numeric.EPSILON) {
                b = (v0 * u1 - u0 * v1) / b;
                if (b < Numeric.EPSILON || b > 1.0 - Numeric.EPSILON) {
                    return false;
                } else {
                    a = (u0 - b * u2) / u1;
                }
            }
        }

        return (a >= Numeric.EPSILON && a <= 1.0 - Numeric.EPSILON && (a + b) <= 1.0 - Numeric.EPSILON);
    }

    /**
Returns true if the 2D segments p1-p2 and p3-p4 intersect
    */
    private static boolean segmentsIntersect2D(Vector2D p1, Vector2D p2, Vector2D p3, Vector2D p4) {
        double a;
        double b;
        double c;
        boolean coLinear = false;

        // From Graphics Gems II, Mukesh Prasad, Intersection of Line Segments, p7
        double du = Math.abs(p2.x - p1.x);
        double dv = Math.abs(p2.y - p1.y);
        if (du > Numeric.EPSILON || dv > Numeric.EPSILON) {
            if (dv > du) {
                a = 1.0;
                b = -(p2.x - p1.x) / (p2.y - p1.y);
                c = -(p1.x + b * p1.y);
            } else {
                a = -(p2.y - p1.y) / (p2.x - p1.x);
                b = 1.0;
                c = -(a * p1.x + p1.y);
            }

            double r3 = a * p3.x + b * p3.y + c;
            double r4 = a * p4.x + b * p4.y + c;

            if (Math.abs(r3) < Numeric.EPSILON && Math.abs(r4) < Numeric.EPSILON) {
                coLinear = true;
            } else if ((r3 > -Numeric.EPSILON && r4 > -Numeric.EPSILON) || (r3 < Numeric.EPSILON && r4 < Numeric.EPSILON)) {
                return false;
            }
        }

        if (coLinear == false) {
            du = Math.abs(p4.x - p3.x);
            dv = Math.abs(p4.y - p3.y);
            if (du > Numeric.EPSILON || dv > Numeric.EPSILON) {
                if (dv > du) {
                    a = 1.0;
                    b = -(p4.x - p3.x) / (p4.y - p3.y);
                    c = -(p3.x + b * p3.y);
                } else {
                    a = -(p4.y - p3.y) / (p4.x - p3.x);
                    b = 1.0;
                    c = -(a * p3.x + p3.y);
                }

                double r1 = a * p1.x + b * p1.y + c;
                double r2 = a * p2.x + b * p2.y + c;

                if (Math.abs(r1) < Numeric.EPSILON && Math.abs(r2) < Numeric.EPSILON) {
                    coLinear = true;
                } else if ((r1 > -Numeric.EPSILON && r2 > -Numeric.EPSILON) || (r1 < Numeric.EPSILON && r2 < Numeric.EPSILON)) {
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
    private static void doComplexFace(int n, Vertex[] v, Vector3D normal, Vertex[] backVertex, ParseRuntimeContext context) {
        // Simplified ear clipping fallback is handled by fan triangulation.
        for (int i = 1; i + 1 < n; i++) {
            Patch face = newFace(v[0], v[i], v[i + 1], null, context);
            if (context.currentMaterial.isSided() == false && face != null) {
                Patch twin = newFace(backVertex[i + 1], backVertex[i], backVertex[0], null, context);
                face.twin = twin;
                if (twin != null) {
                    twin.twin = face;
                }
            }
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
            Material[] holder = new Material[] {context.currentMaterial};
            MgfMaterialEntitySupport.mgfGetCurrentMaterial(holder, context.singleSided, context);
            context.currentMaterial = holder[0];
            context.materialState.currentMaterial = holder[0];
        }

        Vertex[] v = new Vertex[MAXIMUM_FACE_VERTICES + 1];
        Vertex[] backV = new Vertex[MAXIMUM_FACE_VERTICES + 1];

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

        Vector3D normal = new Vector3D();

        if (faceNormal(argc - 1, v, normal) == null) {
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
                face.twin = twin;
                if (twin != null) {
                    twin.twin = face;
                }
            }
        } else if (argc == 5) {
            // Quadrilaterals
            if (context.inComplex || faceIsConvex(argc - 1, v, normal)) {
                face = newFace(v[0], v[1], v[2], v[3], context);
                if (context.currentMaterial.isSided() == false && face != null) {
                    twin = newFace(backV[3], backV[2], backV[1], backV[0], context);
                    face.twin = twin;
                    if (twin != null) {
                        twin.twin = face;
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
            Material[] holder = new Material[] {context.currentMaterial};
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
                currentVertexContext.p.x = Double.parseDouble(av[1]);
                currentVertexContext.p.y = Double.parseDouble(av[2]);
                currentVertexContext.p.z = Double.parseDouble(av[3]);
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
                currentVertexContext.n.x = Double.parseDouble(av[1]);
                currentVertexContext.n.y = Double.parseDouble(av[2]);
                currentVertexContext.n.z = Double.parseDouble(av[3]);
                currentVertexContext.n.normalizeAndGivePreviousNorm(Numeric.EPSILON);
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
