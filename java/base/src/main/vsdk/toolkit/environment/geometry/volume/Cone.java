//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidEulerConstructions;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidEulerOperators;
import java.io.Serial;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.environment.geometry.element.GeometryBounds;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.element.RayHit;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolid;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidValidationEngine;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidVertex;

public class Cone extends Solid {
    @Serial private static final long serialVersionUID = 20060502L;
    private static final double NO_HIT = Double.POSITIVE_INFINITY;

    private double bottomRadius; // Radius at the base
    private double topRadius;    // Radius at the top
    private double height;       // Height

    private static final int DEFAULT_CIRCUMFERENCE_DIVISIONS = 36;
    private static final int DEFAULT_HEIGHT_DIVISIONS = 1;
    private static final int MIN_CIRCUMFERENCE_DIVISIONS = 3;
    private static final int MIN_HEIGHT_DIVISIONS = 1;

    public Cone(double bottomRadius, double topRadius, double height) {
        this.bottomRadius = bottomRadius;
        this.topRadius = topRadius;
        this.height = height;
        getControlSpecifications().add("double;bottomRadius;(0, INFINITE)");
        getControlSpecifications().add("double;topRadius;[0, INFINITE)");
        getControlSpecifications().add("double;height;(0, INFINITE)");
    }

    public double getBottomRadius()
    {
        return bottomRadius;
    }

    public double getTopRadius()
    {
        return topRadius;
    }

    public double getHeight()
    {
        return height;
    }

    public void setBottomRadius(double value)
    {
        bottomRadius = value;
    }

    public void setTopRadius(double value)
    {
        topRadius = value;
    }

    public void setHeight(double value)
    {
        height = value;
    }

    private Ray
    doIntersectionCylinder(Ray inOutRay) {
        return null;
    }

    private Ray
    doIntersectionCylinder(Ray inOutRay, double inR, double inH,
                      RayHit outInfo) 
    {
        double A, B, C, discriminant, t0;
        double ox = inOutRay.getOrigin().x();
        double oy = inOutRay.getOrigin().y();
        double oz = inOutRay.getOrigin().z();
        double dx = inOutRay.getDirection().x();
        double dy = inOutRay.getDirection().y();
        double dz = inOutRay.getDirection().z();

        //- Calcula el termino A --------------------------------------------
        A = VSDK.square(dx) + VSDK.square(dy);
        if ( Math.abs(A) <= VSDK.EPSILON ) return null;

        //- Calcula el termino B --------------------------------------------
        B = 2 * ((dx * ox) + (dy * oy));

        //- Calcula el termino C --------------------------------------------
        C = VSDK.square(ox) + VSDK.square(oy) -
            VSDK.square(inR);

        //- Calcula el discriminant. Si el discriminant no es positivo el -
        //- rayo no intersecta el cilindro. retorna t = 0                      
        discriminant = VSDK.square(B) - 4*A*C;
        if ( discriminant <= VSDK.EPSILON ) return null;

        //- Resuelve la ecuacion cuadratica para las raices de la ecuacion. -
        //- (-B +/- sqrt(B^2 - 4*A*C)) / 2A.                                -
        discriminant = Math.sqrt(discriminant);
        t0 = (-B-discriminant) / (2 * A);

        //- Si t0 es > 0 listo. Si no debemos calcular la otra raiz t1. -----
        if ( t0 > VSDK.EPSILON ) {
            double pz = oz + dz*t0;
            if ( pz > inH || pz < 0 ) {
                return null;
            }

            if ( outInfo != null ) {
                double px = ox + dx*t0;
                double py = oy + dy*t0;
                outInfo.point = new Vector3Dd(px, py, pz);
                outInfo.normal = new Vector3Dd(px, py, 0).normalized();
            }
            return inOutRay.withT(t0);
        }

        return null;
    }

    private Ray
    doIntersectionCone(Ray inOutRay, double inR, double inH,
                      RayHit outInfo) 
    {
        double A, B, C, discriminant, t0;
        double ox = inOutRay.getOrigin().x();
        double oy = inOutRay.getOrigin().y();
        double oz = inOutRay.getOrigin().z();
        double dx = inOutRay.getDirection().x();
        double dy = inOutRay.getDirection().y();
        double dz = inOutRay.getDirection().z();
        if ( inH <= VSDK.EPSILON ) {
            return null;
        }
        double shiftedOz = oz - inH;
        double ratio = inR / inH;
        double ratioSquared = VSDK.square(ratio);

        //- Calcula el termino A --------------------------------------------
        A = VSDK.square(dx) +
            VSDK.square(dy) -
            VSDK.square(dz * ratio);
        if ( Math.abs(A) <= VSDK.EPSILON ) return null;

        //- Calcula el termino B --------------------------------------------
        B = 2 *
            ((dx * ox) +
             (dy * oy) -
             (dz * shiftedOz * ratioSquared)
             );

        //- Calcula el termino C --------------------------------------------
        C = VSDK.square(ox) +
            VSDK.square(oy) -
            VSDK.square(shiftedOz * ratio);

        //- Calcula el discriminant. Si el discriminant no es positivo el -
        //- rayo no intersecta la esfera. retorna t = 0                      
        discriminant = VSDK.square(B) - 4*A*C;
        if ( discriminant <= VSDK.EPSILON ) return null;

        //- Resuelve la ecuacion cuadratica para las raices de la ecuacion. -
        //- (-B +/- sqrt(B^2 - 4*A*C)) / 2A.                                -
        discriminant = Math.sqrt(discriminant);
        t0 = (-B-discriminant) / (2 * A);

        //- Si t0 es > 0 listo. Si no debemos calcular la otra raiz t1. -----
        if ( t0 > VSDK.EPSILON ) {
            double shiftedPz = shiftedOz + dz*t0;
            if ( shiftedPz > 0 || shiftedPz < -inH ) {
                return null;
            }

            if ( outInfo != null ) {
                double px = ox + dx*t0;
                double py = oy + dy*t0;
                outInfo.point = new Vector3Dd(px, py, shiftedPz + inH);
                outInfo.normal =
                    new Vector3Dd(px, py, -shiftedPz * ratioSquared).normalized();
            }
            return inOutRay.withT(t0);
        }

        return null;

    }

    private Ray
    doIntersectionTap(Ray inOutRay, double inR, double inH,
                      RayHit outInfo) {
        double dx = inOutRay.getDirection().x();
        double dy = inOutRay.getDirection().y();
        double dz = inOutRay.getDirection().z();
        double ox = inOutRay.getOrigin().x();
        double oy = inOutRay.getOrigin().y();
        double oz = inOutRay.getOrigin().z();

        if ( Math.abs(dz) > VSDK.EPSILON ) {
            double t = (inH - oz) / dz;
            if ( t > VSDK.EPSILON ) {
                double px = ox + dx*t;
                double py = oy + dy*t;
                if ( VSDK.square(px) + VSDK.square(py) < VSDK.square(inR) ) {
                    if ( outInfo != null ) {
                        outInfo.normal = new Vector3Dd(0, 0, 1);
                        outInfo.point = new Vector3Dd(px, py, inH);
                    }
                    return inOutRay.withT(t);
                }
            }
        }
        return null;
    }

    private static boolean hasHit(double t)
    {
        return t != NO_HIT;
    }

    private double doIntersectionCylinderDistance(Ray inOutRay, double inR, double inH)
    {
        double dx = inOutRay.getDirection().x();
        double dy = inOutRay.getDirection().y();
        double dz = inOutRay.getDirection().z();
        double ox = inOutRay.getOrigin().x();
        double oy = inOutRay.getOrigin().y();
        double oz = inOutRay.getOrigin().z();
        double A = VSDK.square(dx) + VSDK.square(dy);
        if ( Math.abs(A) <= VSDK.EPSILON ) {
            return NO_HIT;
        }

        double B = 2 * ((dx * ox) + (dy * oy));
        double C = VSDK.square(ox) + VSDK.square(oy) - VSDK.square(inR);
        double discriminant = VSDK.square(B) - 4*A*C;
        if ( discriminant <= VSDK.EPSILON ) {
            return NO_HIT;
        }

        double t0 = (-B - Math.sqrt(discriminant)) / (2 * A);
        if ( t0 <= VSDK.EPSILON ) {
            return NO_HIT;
        }
        double pz = oz + dz*t0;
        if ( pz > inH || pz < 0 ) {
            return NO_HIT;
        }
        return t0;
    }

    private double doIntersectionConeDistance(Ray inOutRay, double inR, double inH)
    {
        if ( inH <= VSDK.EPSILON ) {
            return NO_HIT;
        }

        double dx = inOutRay.getDirection().x();
        double dy = inOutRay.getDirection().y();
        double dz = inOutRay.getDirection().z();
        double ox = inOutRay.getOrigin().x();
        double oy = inOutRay.getOrigin().y();
        double shiftedOz = inOutRay.getOrigin().z() - inH;
        double ratio = inR / inH;
        double ratioSquared = VSDK.square(ratio);
        double A = VSDK.square(dx) + VSDK.square(dy) - VSDK.square(dz * ratio);
        if ( Math.abs(A) <= VSDK.EPSILON ) {
            return NO_HIT;
        }

        double B = 2 * ((dx * ox) + (dy * oy) - (dz * shiftedOz * ratioSquared));
        double C =
            VSDK.square(ox) + VSDK.square(oy) - VSDK.square(shiftedOz * ratio);
        double discriminant = VSDK.square(B) - 4*A*C;
        if ( discriminant <= VSDK.EPSILON ) {
            return NO_HIT;
        }

        double t0 = (-B - Math.sqrt(discriminant)) / (2 * A);
        if ( t0 <= VSDK.EPSILON ) {
            return NO_HIT;
        }
        double shiftedPz = shiftedOz + dz*t0;
        if ( shiftedPz > 0 || shiftedPz < -inH ) {
            return NO_HIT;
        }
        return t0;
    }

    private double doIntersectionTapDistance(Ray inOutRay, double inR, double inH)
    {
        double dz = inOutRay.getDirection().z();
        if ( Math.abs(dz) <= VSDK.EPSILON ) {
            return NO_HIT;
        }

        double t = (inH - inOutRay.getOrigin().z()) / dz;
        if ( t <= VSDK.EPSILON ) {
            return NO_HIT;
        }

        double px = inOutRay.getOrigin().x() + inOutRay.getDirection().x()*t;
        double py = inOutRay.getOrigin().y() + inOutRay.getDirection().y()*t;
        if ( VSDK.square(px) + VSDK.square(py) >= VSDK.square(inR) ) {
            return NO_HIT;
        }
        return t;
    }

    private boolean doIntersectionDistanceOnly(Ray inOutRay, RayHit outHit)
    {
        double winnerT = NO_HIT;

        if ( topRadius < VSDK.EPSILON && bottomRadius > VSDK.EPSILON ) {
            double bodyT = doIntersectionConeDistance(inOutRay, bottomRadius, height);
            double tap1T = doIntersectionTapDistance(inOutRay, bottomRadius, 0);
            if ( hasHit(tap1T) && (!hasHit(bodyT) || tap1T < bodyT) ) {
                winnerT = tap1T;
            }
            else if ( hasHit(bodyT) ) {
                winnerT = bodyT;
            }
        }
        else if ( VSDK.equals(bottomRadius, topRadius) ) {
            double bodyT = doIntersectionCylinderDistance(inOutRay, bottomRadius, height);
            double tap1T = doIntersectionTapDistance(inOutRay, bottomRadius, 0);
            double tap2T = doIntersectionTapDistance(inOutRay, bottomRadius, height);

            if ( hasHit(bodyT) &&
                 ((hasHit(tap1T) && bodyT < tap1T) || !hasHit(tap1T)) &&
                 ((hasHit(tap2T) && bodyT < tap2T) || !hasHit(tap2T)) ) {
                winnerT = bodyT;
            }
            else if ( hasHit(tap1T) &&
                      ((hasHit(bodyT) && tap1T < bodyT) || !hasHit(bodyT)) &&
                      ((hasHit(tap2T) && tap1T < tap2T) || !hasHit(tap2T)) ) {
                winnerT = tap1T;
            }
            else if ( hasHit(tap2T) ) {
                winnerT = tap2T;
            }
        }

        if ( !hasHit(winnerT) ) {
            return false;
        }
        if ( outHit != null ) {
            if ( outHit.shouldStoreRay() ) {
                outHit.setRay(inOutRay.withT(winnerT));
            }
            else {
                outHit.setHitDistance(winnerT);
            }
        }
        return true;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.
    @param inOutRay
    @return true if given ray intersects current Cone
    */
    public Ray
    doIntersectionFirstHit(Ray inOutRay) {
        RayHit hit = new RayHit();
        if ( doIntersectionFirstHit(inOutRay, hit) ) {
            return hit.getRay();
        }
        return null;
    }

    @Override
    public boolean doIntersectionFirstHit(Ray inOutRay, RayHit outHit)
    {
        if ( outHit == null || !outHit.needsAnySurfaceData() ) {
            return doIntersectionDistanceOnly(inOutRay, outHit);
        }

        RayHit infoTap1 = new RayHit();
        RayHit infoTap2 = new RayHit();
        RayHit infoBody = new RayHit();
        Ray bodyHit;
        Ray tap1Hit;
        Ray tap2Hit;
        Ray winner = null;
        RayHit winnerInfo = null;

        if ( topRadius < VSDK.EPSILON && bottomRadius > VSDK.EPSILON ) {
            bodyHit = doIntersectionCone(inOutRay, bottomRadius, height, infoBody);
            tap1Hit = doIntersectionTap(inOutRay, bottomRadius, 0, infoTap1);
            if ( (tap1Hit != null && bodyHit == null) ||
                 (tap1Hit != null && bodyHit != null && (tap1Hit.getT() < bodyHit.getT())) ) {
                infoTap1.normal = infoTap1.normal.multiply(-1);
                winner = inOutRay.withT(tap1Hit.getT());
                winnerInfo = infoTap1;
            }
            else if ( bodyHit != null ) {
                winner = inOutRay.withT(bodyHit.getT());
                winnerInfo = infoBody;
            }
        }
        else if ( VSDK.equals(bottomRadius, topRadius) ) {
            int nearest = -1;
            bodyHit = doIntersectionCylinder(inOutRay, bottomRadius, height, infoBody);
            tap1Hit = doIntersectionTap(inOutRay, bottomRadius, 0, infoTap1);
            tap2Hit = doIntersectionTap(inOutRay, bottomRadius, height, infoTap2);

            if ( bodyHit != null &&
                 ((tap1Hit != null && (bodyHit.getT() < tap1Hit.getT())) || tap1Hit == null) &&
                 ((tap2Hit != null && (bodyHit.getT() < tap2Hit.getT())) || tap2Hit == null) ) {
                nearest = 1;
            }
            else if ( tap1Hit != null &&
                      (bodyHit != null && (tap1Hit.getT() < bodyHit.getT()) || bodyHit == null) &&
                      (tap2Hit != null && (tap1Hit.getT() < tap2Hit.getT()) || tap2Hit == null) ) {
                nearest = 3;
            }
            else if ( tap2Hit != null ) {
                nearest = 2;
            }

            if ( nearest == 1 ) {
                winner = inOutRay.withT(bodyHit.getT());
                winnerInfo = infoBody;
            }
            else if ( nearest == 2 ) {
                winner = inOutRay.withT(tap2Hit.getT());
                winnerInfo = infoTap2;
            }
            else if ( nearest == 3 ) {
                winner = inOutRay.withT(tap1Hit.getT());
                winnerInfo = infoTap1;
            }
        }

        if ( winner == null ) {
            return false;
        }
        if ( outHit != null ) {
            outHit.setRay(winner);
            outHit.point = new Vector3Dd(winnerInfo.point);
            outHit.normal = new Vector3Dd(winnerInfo.normal).normalized();
            outHit.tangent = new Vector3Dd(winnerInfo.tangent);
            outHit.u = winnerInfo.u;
            outHit.v = winnerInfo.v;
            outHit.material = winnerInfo.material;
            outHit.texture = winnerInfo.texture;
            outHit.normalMap = winnerInfo.normalMap;
        }
        return true;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    @param inRay
    @param inT
    @param outData
    */
    public void
    doExtraInformation(Ray inRay, double inT, 
                                  RayHit outData)
    {
        RayHit hit = new RayHit();
        if ( doIntersectionFirstHit(inRay.withT(Double.MAX_VALUE), hit) ) {
            outData.clone(hit);
        }
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest. The cone (or truncated cone, or cylinder)
    has its axis along Z, its base of radius `bottomRadius` at z = 0 and its
    top of radius `topRadius` at z = `height`. The distance to the side is
    measured perpendicular to the side, so the tolerance is the same for the
    side and for the caps.
    @param p point to classify, in the space of the cone
    @param distanceTolerance distance to the surface of the cone under which
    the point is classified as `LIMIT`
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    @Override
    public int doContainmentTest(Vector3Dd p, double distanceTolerance)
    {
        if ( height <= 0 ) {
            return OUTSIDE;
        }
        double z = p.z();
        double radialDistance = Math.sqrt(p.x() * p.x() + p.y() * p.y());
        double radiusAtZ = bottomRadius + (topRadius - bottomRadius) * (z / height);
        double radiusChange = bottomRadius - topRadius;
        double sideCosine = height / Math.sqrt(height * height + radiusChange * radiusChange);
        double sideDistance = (radialDistance - radiusAtZ) * sideCosine;

        if ( z < -distanceTolerance || z > height + distanceTolerance ||
             sideDistance > distanceTolerance ) {
            return OUTSIDE;
        }
        if ( z > distanceTolerance && z < height - distanceTolerance &&
             sideDistance < -distanceTolerance ) {
            return INSIDE;
        }
        return LIMIT;
    }

    /**
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    /** Finite solid bounds: |x|,|y| <= max radius and 0 <= z <= height. */
    @Override
    public void getBounds(GeometryBounds result) {
        final double r = (bottomRadius > topRadius) ? bottomRadius : topRadius;
        if (!Double.isFinite(r) || !Double.isFinite(height) || height <= 0 ||
            bottomRadius < 0 || topRadius < 0) { result.setIndeterminate(); return; }
        result.setFinite(new Vector3Dd(-r,-r,0), new Vector3Dd(r,r,height));
    }

    @Override
    public double[] getMinMax()
    {
        // TODO!
        double [] minmax = new double[6];

        double r = Math.max(bottomRadius, topRadius);

        minmax[0] = -r;
        minmax[1] = -r;
        minmax[2] = 0;
        minmax[3] = r;
        minmax[4] = r;
        minmax[5] = height;

        return minmax;
    }

    @Override
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid()
    {
        return buildPolyhedralBoundedSolid(
            DEFAULT_CIRCUMFERENCE_DIVISIONS, DEFAULT_HEIGHT_DIVISIONS);
    }

    /**
    Closed boundary representation of the cone (see the three parameter
    version, with `withCaps` true).
    @param circumferenceDivisions sides of the cone base (at least 3)
    @param heightDivisions vertical subdivisions (at least 1)
    @return a new boundary representation of the cone
    */
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid(
        int circumferenceDivisions, int heightDivisions)
    {
        return exportToPolyhedralBoundedSolid(circumferenceDivisions,
            heightDivisions, true);
    }

    /**
    Boundary representation of the cone (a cylinder when both radii are
    equal), with its base on the plane z = 0 and its axis on +Z. The rings
    of vertices start at +X and go counterclockwise seen from +Z.

    With `withCaps` true it is the closed solid: the lateral surface plus
    the base disk and, when the top radius is not zero, the top disk.

    With `withCaps` false it is a valid boundary representation of the
    lateral surface alone, an open curved surface without caps: a lamina
    ([MANT1988] 12.2, generalized from the planar circular lamina) with two
    coincident sides of opposite orientation, so it encloses no volume.
      - When both radii are not zero (cylinder or frustum) the lamina is
        built from a washer (`Tube.createWasherLamina`) whose inner ring is
        moved to z = height with the top radius, and both of its faces are
        split into the planar quadrilaterals between the two rings (a
        `lmekr` joins both loops of each face and `smef` splits it). The
        lamina has the topology of a torus (genus 1). Faces 1 and
        3 to `circumferenceDivisions` + 1 are the outer side (normals away
        from the axis); faces 2 and `circumferenceDivisions` + 2 to
        2 * `circumferenceDivisions` are the inner side.
      - When the top radius is zero (cone with apex) it is a circular lamina
        whose two faces are split into triangle fans, each one towards its
        own apex vertex (both at the same point), so the lamina has the
        topology of a sphere. Face 1 and faces 3 to
        `circumferenceDivisions` + 1 are the outer side; face 2 and the
        rest are the inner side.
    The open lamina has a single band of faces along the axis:
    `heightDivisions` is not used.

    @param circumferenceDivisions sides of the cone base (at least 3)
    @param heightDivisions vertical subdivisions of the closed solid (at
    least 1)
    @param withCaps true for the closed solid, false for the open lamina of
    the lateral surface
    @return a new boundary representation of the cone
    */
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid(
        int circumferenceDivisions, int heightDivisions, boolean withCaps)
    {
        int normalizedCircumferenceDivisions = Math.max(
            MIN_CIRCUMFERENCE_DIVISIONS, circumferenceDivisions);
        int normalizedHeightDivisions = Math.max(
            MIN_HEIGHT_DIVISIONS, heightDivisions);

        if ( !withCaps ) {
            return buildOpenLamina(normalizedCircumferenceDivisions);
        }
        if ( normalizedCircumferenceDivisions ==
                 DEFAULT_CIRCUMFERENCE_DIVISIONS &&
             normalizedHeightDivisions == DEFAULT_HEIGHT_DIVISIONS ) {
            return exportToPolyhedralBoundedSolid();
        }

        return buildPolyhedralBoundedSolid(normalizedCircumferenceDivisions,
            normalizedHeightDivisions);
    }

    /**
    Current implementation of the cylinder follows the idea suggested on
    section [MANT1988].12.3.1 and program [MANT1988].12.4, where the
    cylinder is built upon a circular lamina base and an extrusion 
    (translational sweep) operation. The cone case is done manually,
    */
    private static void closeTopFaceToApex(PolyhedralBoundedSolid solid,
        double apexZ)
    {
        closeFaceToApex(solid, 1, apexZ);
    }

    /**
    Splits face `faceId`, a single loop polygon, into a fan of triangles
    towards a new vertex at (0, 0, `apexZ`); the face keeps one of the
    triangles.
    */
    private static void closeFaceToApex(PolyhedralBoundedSolid solid,
        int faceId, double apexZ)
    {
        Tube.splitFaceIntoFan(solid, faceId, new Vector3Dd(0.0, 0.0, apexZ));
    }

    private PolyhedralBoundedSolid buildPolyhedralBoundedSolid(int nsides,
        int heightDivisions)
    {
        PolyhedralBoundedSolid solid;
        Matrix4x4d T, S, M;

        solid = PolyhedralBoundedSolidEulerConstructions.createCircularLamina(
            0.0, 0.0, bottomRadius, 0.0, nsides
        );

        if ( topRadius > VSDK.EPSILON && bottomRadius > VSDK.EPSILON ) {
            double prevRadius = bottomRadius;
            double zStep = height / ((double)heightDivisions);
            int i;
            for ( i = 1; i <= heightDivisions; i++ ) {
                double nextRadius = bottomRadius + (topRadius - bottomRadius) *
                    (((double)i) / ((double)heightDivisions));
                double f = nextRadius / prevRadius;
                T = new Matrix4x4d();
                T = T.translation(0.0, 0.0, zStep);
                S = new Matrix4x4d();
                S = S.scale(f, f, 1.0);
                M = T.multiply(S);
                PolyhedralBoundedSolidEulerConstructions.translationalSweepExtrudeFacePlanar(
                    solid, solid.findFace(1), M);
                prevRadius = nextRadius;
            }
        }
        else if ( topRadius <= VSDK.EPSILON && bottomRadius > VSDK.EPSILON ) {
            // Cone case, with optional vertical subdivisions.
            double prevRadius = bottomRadius;
            double zStep = height / ((double)heightDivisions);
            int i;
            for ( i = 1; i < heightDivisions; i++ ) {
                double nextRadius = bottomRadius *
                    (1.0 - (((double)i) / ((double)heightDivisions)));
                double f = nextRadius / prevRadius;
                T = new Matrix4x4d();
                T = T.translation(0.0, 0.0, zStep);
                S = new Matrix4x4d();
                S = S.scale(f, f, 1.0);
                M = T.multiply(S);
                PolyhedralBoundedSolidEulerConstructions.translationalSweepExtrudeFacePlanar(
                    solid, solid.findFace(1), M);
                prevRadius = nextRadius;
            }
            closeTopFaceToApex(solid, height);
        }
        return solid;
    }

    /**
    Open lamina of the lateral surface (see `exportToPolyhedralBoundedSolid`
    with `withCaps` false).
    */
    private PolyhedralBoundedSolid buildOpenLamina(int nsides)
    {
        PolyhedralBoundedSolid solid;
        int k;

        if ( topRadius <= VSDK.EPSILON ) {
            //- Cone with apex: both faces of a disk as fans, center lifted ---
            // The fans of faces 1 and 2 end at the vertices n+1 and n+2
            solid = Tube.createWasherLamina(0.0, bottomRadius, 0.0, nsides,
                true);
            solid.findVertex(nsides + 1).position = new Vector3Dd(0.0, 0.0, height);
            solid.findVertex(nsides + 2).position = new Vector3Dd(0.0, 0.0, height);
            PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
            return solid;
        }

        //- Washer split in quadrilaterals, internal circle lifted ----------
        // The external circle has the vertices 1..n at z = 0; the internal
        // vertex at the angle of the external vertex k+1 is
        // n+1+((n-k) mod n). Each one is placed above its external pair,
        // scaled to the top radius, so both have exactly the same longitude
        // and the quadrilaterals between them are planar.
        solid = Tube.createWasherLamina(0.5 * bottomRadius, bottomRadius, 0.0,
            nsides, true);
        double scale = topRadius / bottomRadius;
        for ( k = 0; k < nsides; k++ ) {
            _PolyhedralBoundedSolidVertex bottom = solid.findVertex(k + 1);
            _PolyhedralBoundedSolidVertex top =
                solid.findVertex(nsides + 1 + ((nsides - k) % nsides));
            top.position = new Vector3Dd(bottom.position.x() * scale,
                bottom.position.y() * scale, height);
        }
        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
        return solid;
    }

}
