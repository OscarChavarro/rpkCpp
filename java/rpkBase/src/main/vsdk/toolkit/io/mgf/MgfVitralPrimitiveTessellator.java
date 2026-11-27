package vsdk.toolkit.io.mgf;

import java.util.ArrayList;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.volume.Cone;
import vsdk.toolkit.environment.geometry.volume.Prism;
import vsdk.toolkit.environment.geometry.volume.Sphere;
import vsdk.toolkit.environment.geometry.volume.Torus;
import vsdk.toolkit.environment.geometry.volume.Tube;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolid;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.ReaderContext;
import vsdk.toolkit.io.context.TokenValidationContext;
import vsdk.toolkit.io.context.VertexContext;

/**
Handlers of the MGF surface primitives (`sph`, `cyl`, `cone`, `torus`,
`ring` and `prism`) that build each one as the matching VITRAL geometry
(`Sphere`, `Cone`, `Torus`, `Tube.createWasherLamina` and `Prism`),
export it to a `PolyhedralBoundedSolid` and turn its faces into MGF faces
(see `MgfBoundaryFaceEmitter`).

The tessellation follows the classic MGF one: 4 * n divisions around each
circle and 2 * n along the meridians of the sphere, for n quarter circle
divisions, quadrilaterals with triangle fans at the poles and apexes, open
cones and cylinders, and the vertex normals of `MgfSurfaceNormalModel`.
The local frame of each primitive is the one of the classic tessellation
(`MgfTessellationMath.mgfMakeAxes` on the axis of the primitive), so the
vertices are at the same angles.

VITRAL primitives have positive sizes: an MGF primitive with negative radii
(whose surface faces inwards) is built with the absolute values and its
faces are turned inside out.
*/
public final class MgfVitralPrimitiveTessellator {
    private MgfVitralPrimitiveTessellator() {
    }

    private static int circleDivisions(ParseRuntimeContext context) {
        return 4 * Math.max(1, context.numberOfQuarterCircleDivisions);
    }

    private static double roundedDeltaToZero(double value) {
        double[] holder = {value};
        Numeric.roundDeltaToZero(holder, VSDK.EPSILON);
        return holder[0];
    }

    /**
    sph center radius
    */
    public static int handleSphere(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        if ( argumentCount != 3 ) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        VertexContext center = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1], context);
        if ( center == null ) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        if ( !TokenValidationContext.isFloat(argumentValues[2]) ) {
            return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        double radius = Double.parseDouble(argumentValues[2]);
        if ( Math.abs(radius) <= VSDK.EPSILON ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        // As the classic tessellation, the axis of the sphere is +Z
        Vector3Dd[] u = {new Vector3Dd()};
        Vector3Dd[] v = {new Vector3Dd()};
        Vector3Dd w = new Vector3Dd(0.0, 0.0, 1.0);
        MgfTessellationMath.mgfMakeAxes(u, v, w, VSDK.EPSILON);

        int n = circleDivisions(context);
        Sphere sphere = new Sphere(Math.abs(radius));
        PolyhedralBoundedSolid solid = sphere.exportToPolyhedralBoundedSolid(n, n / 2, true);
        MgfBoundaryFaceEmitter emitter = new MgfBoundaryFaceEmitter(center.p, u[0], v[0], w, radius < 0.0);
        return emitter.emitSolid(solid, MgfSurfaceNormalModel.sphere(), context);
    }

    /**
    cyl bottomCenter radius topCenter: a cone with equal radii
    */
    public static int handleCylinder(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        if ( argumentCount != 4 ) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        String[] coneArguments = {
            context.entityNames[EntityTypeContext.CONE],
            argumentValues[1],
            argumentValues[2],
            argumentValues[3],
            argumentValues[2]
        };
        return handleCone(5, coneArguments, context);
    }

    /**
    cone center1 radius1 center2 radius2: the open lateral surface of a cone.
    Radii of opposite signs are illegal, one radius may be 0 (the apex) and
    negative radii turn the surface inwards
    */
    public static int handleCone(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        if ( argumentCount != 5 ) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        VertexContext v1Context = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1], context);
        VertexContext v2Context = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[3], context);
        if ( v1Context == null || v2Context == null ) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        if ( !TokenValidationContext.isFloat(argumentValues[2]) || !TokenValidationContext.isFloat(argumentValues[4]) ) {
            return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        double radius1 = roundedDeltaToZero(Double.parseDouble(argumentValues[2]));
        double radius2 = roundedDeltaToZero(Double.parseDouble(argumentValues[4]));

        if ( radius1 == 0.0 && radius2 == 0.0 ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        if ( radius1 != 0.0 && radius2 != 0.0 && ((radius1 < 0.0) != (radius2 < 0.0)) ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        // The base of the VITRAL cone is the end with the non zero radius,
        // the classic tessellation took the end 2 when it could
        if ( radius2 == 0.0 ) {
            return emitCone(v1Context.p, radius1, v2Context.p, radius2, context);
        }
        return emitCone(v2Context.p, radius2, v1Context.p, radius1, context);
    }

    private static int emitCone(
        Vector3Dd bottomCenter,
        double bottomRadius,
        Vector3Dd topCenter,
        double topRadius,
        ParseRuntimeContext context)
    {
        Vector3Dd[] w = {new Vector3Dd(
            topCenter.x() - bottomCenter.x(), topCenter.y() - bottomCenter.y(), topCenter.z() - bottomCenter.z())};
        double height = MgfTessellationMath.normalizeAndGivePreviousNorm(w, VSDK.EPSILON);
        if ( Numeric.doubleEqual(height, 0.0, VSDK.EPSILON) ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        Vector3Dd[] u = {new Vector3Dd()};
        Vector3Dd[] v = {new Vector3Dd()};
        MgfTessellationMath.mgfMakeAxes(u, v, w[0], VSDK.EPSILON);

        boolean inwards = bottomRadius < 0.0;
        double bottom = Math.abs(bottomRadius);
        double top = Math.abs(topRadius);
        Cone cone = new Cone(bottom, top, height);
        PolyhedralBoundedSolid solid = cone.exportToPolyhedralBoundedSolid(circleDivisions(context), 1, false);
        MgfBoundaryFaceEmitter emitter = new MgfBoundaryFaceEmitter(bottomCenter, u[0], v[0], w[0], inwards);
        return emitter.emitSolid(solid, MgfSurfaceNormalModel.coneLamina(bottom, top, height), context);
    }

    /**
    torus center minorRadius maxRadius: the center vertex gives the axis as its
    normal. The distance from the axis goes from the first radius to the second
    one; both negative turn the surface inwards
    */
    public static int handleTorus(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        if ( argumentCount != 4 ) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        VertexContext center = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1], context);
        if ( center == null ) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        if ( MgfTessellationMath.isNull(center.n, VSDK.EPSILON) ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        if ( !TokenValidationContext.isFloat(argumentValues[2]) || !TokenValidationContext.isFloat(argumentValues[3]) ) {
            return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        double minRadius = roundedDeltaToZero(Double.parseDouble(argumentValues[2]));
        double maxRadius = Double.parseDouble(argumentValues[3]);
        if ( minRadius == 0.0 || (minRadius > 0.0 ? maxRadius - minRadius : minRadius - maxRadius) <= 0.0 ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        Vector3Dd[] u = {new Vector3Dd()};
        Vector3Dd[] v = {new Vector3Dd()};
        MgfTessellationMath.mgfMakeAxes(u, v, center.n, VSDK.EPSILON);

        double majorRadius = Math.abs(0.5 * (minRadius + maxRadius));
        double tubeRadius = Math.abs(0.5 * (maxRadius - minRadius));
        int n = circleDivisions(context);
        Torus torus = new Torus(majorRadius, tubeRadius);
        PolyhedralBoundedSolid solid = torus.exportToPolyhedralBoundedSolid(n, n);
        MgfBoundaryFaceEmitter emitter = new MgfBoundaryFaceEmitter(center.p, u[0], v[0], center.n, minRadius < 0.0);
        return emitter.emitSolid(solid, MgfSurfaceNormalModel.torus(majorRadius), context);
    }

    /**
    ring center innerRadius outerRadius: a planar disk (inner radius 0) or
    annulus facing the normal of the center vertex
    */
    public static int handleRing(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        if ( argumentCount != 4 ) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        VertexContext center = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[1], context);
        if ( center == null ) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        if ( MgfTessellationMath.isNull(center.n, VSDK.EPSILON) ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        if ( !TokenValidationContext.isFloat(argumentValues[2]) || !TokenValidationContext.isFloat(argumentValues[3]) ) {
            return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        double minRadius = roundedDeltaToZero(Double.parseDouble(argumentValues[2]));
        double maxRadius = Double.parseDouble(argumentValues[3]);
        if ( minRadius < 0.0 || maxRadius <= minRadius ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        Vector3Dd[] u = {new Vector3Dd()};
        Vector3Dd[] v = {new Vector3Dd()};
        MgfTessellationMath.mgfMakeAxes(u, v, center.n, VSDK.EPSILON);

        double innerRadius = Numeric.doubleEqual(minRadius, 0.0, VSDK.EPSILON) ? 0.0 : minRadius;
        PolyhedralBoundedSolid solid = Tube.createWasherLamina(innerRadius, maxRadius, 0.0, circleDivisions(context), true);
        MgfBoundaryFaceEmitter emitter = new MgfBoundaryFaceEmitter(center.p, u[0], v[0], center.n, false);
        return emitter.emitSolid(solid, MgfSurfaceNormalModel.flatLamina(), context);
    }

    /**
    prism v1 v2 ... vn length: the polygon v1 .. vn (counterclockwise around its
    normal) extruded `length` units against its normal; a negative length
    extrudes it along the normal, with the surface facing inwards. The walls
    take the normals of the vertices of the polygon, if any of them has one
    */
    public static int handlePrism(int argumentCount, String[] argumentValues, ParseRuntimeContext context) {
        final int maximumVertices = ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT - 2;
        VertexContext[] polygon = new VertexContext[ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT];

        if ( argumentCount < 5 || argumentCount - 2 > maximumVertices ) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if ( !TokenValidationContext.isFloat(argumentValues[argumentCount - 1]) ) {
            return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        double length = Double.parseDouble(argumentValues[argumentCount - 1]);
        if ( length <= VSDK.EPSILON && length >= -VSDK.EPSILON ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        // Normal of the polygon, as the classic tessellation
        int numberOfVertices = argumentCount - 2;
        boolean hasNormals = false;
        Vector3Dd[] normal = {new Vector3Dd(0.0, 0.0, 0.0)};
        Vector3Dd previous = new Vector3Dd(0.0, 0.0, 0.0);
        for ( int i = 0; i < numberOfVertices; i++ ) {
            polygon[i] = MgfVertexFaceEntitySupport.getNamedVertex(argumentValues[i + 1], context);
            if ( polygon[i] == null ) {
                return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
            }
            if ( i == 0 ) {
                continue;
            }
            if ( !MgfTessellationMath.isNull(polygon[i].n, VSDK.EPSILON) ) {
                hasNormals = true;
            }
            Vector3Dd current = polygon[i].p.subtract(polygon[0].p);
            normal[0] = normal[0].add(previous.crossProduct(current));
            previous = current;
        }
        if ( MgfTessellationMath.normalizeAndGivePreviousNorm(normal, VSDK.EPSILON) == 0.0 ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        // Local frame: the polygon on z = 0, extruded along +Z
        Vector3Dd w = length > 0.0 ? normal[0].multiply(-1.0) : normal[0];
        Vector3Dd[] u = {new Vector3Dd()};
        Vector3Dd[] v = {new Vector3Dd()};
        MgfTessellationMath.mgfMakeAxes(u, v, w, VSDK.EPSILON);
        MgfBoundaryFaceEmitter emitter = new MgfBoundaryFaceEmitter(polygon[0].p, u[0], v[0], w, length < 0.0);

        ArrayList<Vector2Dd> base = new ArrayList<>();
        for ( int i = 0; i < numberOfVertices; i++ ) {
            Vector3Dd d = polygon[i].p.subtract(polygon[0].p);
            base.add(new Vector2Dd(d.dotProduct(u[0]), d.dotProduct(v[0])));
        }
        Prism prism = new Prism(base, Math.abs(length));
        if ( !prism.hasValidParameters() ) {
            MgfEntityControl.doWarning("prism base is not a simple polygon", context);
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        PolyhedralBoundedSolid solid = prism.exportToPolyhedralBoundedSolid();

        Vector3Dd[] points = new Vector3Dd[ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT];
        Vector3Dd[] normals = new Vector3Dd[ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT];
        int errorCode = ParseErrorContext.MGF_OK;
        for ( int f = 0; f < solid.getPolygonsList().size() && errorCode == ParseErrorContext.MGF_OK; f++ ) {
            int faceSize = MgfBoundaryFaceEmitter.faceVertices(solid, f, points, maximumVertices);
            if ( faceSize < 3 ) {
                errorCode = ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                break;
            }
            Vector3Dd faceNormal = MgfBoundaryFaceEmitter.polygonNormal(faceSize, points);
            boolean wall = Math.abs(faceNormal.z()) < 0.5 * faceNormal.length();
            Vector3Dd[] faceNormals = null;
            if ( hasNormals && wall ) {
                // Each wall vertex takes the normal of its polygon vertex
                for ( int k = 0; k < faceSize; k++ ) {
                    int closest = 0;
                    double closestDistance = -1.0;
                    for ( int i = 0; i < numberOfVertices; i++ ) {
                        double dx = points[k].x() - base.get(i).x;
                        double dy = points[k].y() - base.get(i).y;
                        double distance = dx * dx + dy * dy;
                        if ( closestDistance < 0.0 || distance < closestDistance ) {
                            closest = i;
                            closestDistance = distance;
                        }
                    }
                    // The classic tessellation kept them as given, also when
                    // the faces are reversed (and so their normals negated)
                    normals[k] = length < 0.0 ? polygon[closest].n.multiply(-1.0) : polygon[closest].n;
                }
                faceNormals = normals;
            }
            errorCode = emitter.emitFace(faceSize, points, faceNormals, context);
        }
        return errorCode;
    }
}
