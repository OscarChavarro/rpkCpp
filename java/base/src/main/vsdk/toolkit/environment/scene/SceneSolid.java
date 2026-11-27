package vsdk.toolkit.environment.scene;

import vsdk.toolkit.common.Entity;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.numerics.FloatingInterval;
import vsdk.toolkit.environment.geometry.Geometry;
import vsdk.toolkit.environment.geometry.element.GeometryBounds;
import vsdk.toolkit.environment.geometry.element.GeometryContainment;
import vsdk.toolkit.environment.geometry.element.GeometryDistanceStatus;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.element.RayCrossingQuery;
import vsdk.toolkit.environment.geometry.element.RayCrossingSink;
import vsdk.toolkit.environment.geometry.element.RayCrossingSupport;
import vsdk.toolkit.environment.geometry.element.RayInstanceStep;
import vsdk.toolkit.environment.geometry.element.RayHit;
import vsdk.toolkit.environment.geometry.element.RaySolidClassification;
import vsdk.toolkit.environment.geometry.element.RaySolidQueryStatus;
import vsdk.toolkit.environment.geometry.volume.AffineEllipsoidDistance;
import vsdk.toolkit.environment.geometry.volume.AffineEllipsoidDistanceBounds;
import vsdk.toolkit.environment.geometry.volume.AffineEllipsoidMinimumDistance;
import vsdk.toolkit.environment.geometry.volume.Box;
import vsdk.toolkit.environment.geometry.volume.Solid;
import vsdk.toolkit.environment.geometry.volume.Sphere;
import vsdk.toolkit.environment.material.Material;

public abstract class SceneSolid extends Entity {
    protected Geometry geometry;

    /// False when the geometry is shared (see `setGeometryReference`)
    private boolean ownsGeometry;

    private boolean geometryIsSphere;

    private String name;

    private long modificationVersion;

    private static GeometryContainment classifyPlacedSphereCertified(double radius, Matrix4x4d forward, Matrix4x4d inverse, Vector3Dd point, double tolerance, int iterationLimit) {
    Vector3Dd seed=inverse.multiply(point);
    if (!Double.isFinite(seed.x()) || !Double.isFinite(seed.y()) ||
        !Double.isFinite(seed.z())) return GeometryContainment.Indeterminate;
    final boolean center=(point.x()==forward.get(0,3) && point.y()==forward.get(1,3) && point.z()==forward.get(2,3)) ||
        (seed.x()==0 && seed.y()==0 && seed.z()==0);
    final Vector3Dd[] axes = {new Vector3Dd(1,0,0),new Vector3Dd(0,1,0),new Vector3Dd(0,0,1)};
    double forwardNormSquared=0;
    for (int i=0;i<3;++i) for (int j=0;j<3;++j)
        forwardNormSquared+=forward.get(i,j)*forward.get(i,j);
    final double lipschitz=radius*radius*forwardNormSquared;
    final int seedCount=center ? 3 : 1;
    for (int candidate=0;candidate<seedCount;++candidate) {
      if (center) seed=axes[candidate];
      for (int iteration=0;iteration<iterationLimit;++iteration) {
        if (iteration==0 || iteration%4==0 || iteration+1==iterationLimit) {
            GeometryContainment[] side={GeometryContainment.Indeterminate};
            double[] lower={0},upper={0};
            final GeometryDistanceStatus status=AffineEllipsoidDistanceBounds.query(point,
                radius,forward,seed,side,lower,upper,iteration==0 && candidate==0);
            if (status==GeometryDistanceStatus.InvalidInput) return GeometryContainment.InvalidInput;
            if (status==GeometryDistanceStatus.Unsupported) return GeometryContainment.Unsupported;
            if (status!=GeometryDistanceStatus.Valid) return GeometryContainment.Indeterminate;
            if (upper[0]<=tolerance) return GeometryContainment.Boundary;
            if (lower[0]>tolerance) return side[0];
        }
        if (!(lipschitz>0) || !Double.isFinite(lipschitz)) break;
        final double seedLength=seed.length();
        if (!(seedLength>0) || !Double.isFinite(seedLength)) break;
        final Vector3Dd unit=seed.multiply(1.0/seedLength);
        final Vector3Dd delta=forward.multiply(unit.multiply(radius)).subtract(point);
        final double[] components = {delta.x(),delta.y(),delta.z()};
        double[] gradient = {0,0,0};
        for (int i=0;i<3;++i) for (int j=0;j<3;++j)
            gradient[i]+=radius*forward.get(j,i)*components[j];
        final Vector3Dd g = new Vector3Dd(gradient[0],gradient[1],gradient[2]);
        final Vector3Dd tangent=g.subtract(unit.multiply(g.dotProduct(unit)));
        if (tangent.dotProduct(tangent)==0) break;
        seed=unit.subtract(tangent.multiply(1.0/lipschitz));
        if (!Double.isFinite(seed.x()) || !Double.isFinite(seed.y()) ||
            !Double.isFinite(seed.z())) break;
      }
    }
    // If surface-directed search cannot decide, use a separately certified
    // minimum-distance enclosure. The numerical estimate only proposes its
    // witness; interval bounds, never the estimate, determine this result.
    final double requestedWidth=tolerance*1e-3;
    if (requestedWidth>0 && Double.isFinite(requestedWidth)) {
        GeometryContainment[] side={GeometryContainment.Indeterminate};
        double[] lower={0},upper={0};
        final GeometryDistanceStatus status=AffineEllipsoidMinimumDistance.query(
            point,radius,forward,inverse,requestedWidth,side,lower,upper);
        if (status==GeometryDistanceStatus.InvalidInput) return GeometryContainment.InvalidInput;
        if (status==GeometryDistanceStatus.Unsupported) return GeometryContainment.Unsupported;
        if (status==GeometryDistanceStatus.Valid) {
            if (upper[0]<=tolerance) return GeometryContainment.Boundary;
            if (lower[0]>tolerance) return side[0];
        }
    }
    return GeometryContainment.Indeterminate;
}

    private static GeometryContainment classifyPlacedSolidCertified(Geometry geometry, Matrix4x4d forward, Vector3Dd point, double tolerance) {
    boolean identity=true;
    for (int row=0;row<4;++row) for (int column=0;column<4;++column)
        if (forward.get(row,column)!=(row==column ? 1.0 : 0.0)) identity=false;
    if (identity) return geometry.classifyPointCertified(point,tolerance);
    Vector3Dd[] localHolder={new Vector3Dd()}, halfWidthsHolder={new Vector3Dd()};
    double[] outerHolder={0},innerHolder={-1};
    final AffinePlacementStatus mapped=AffineToleranceMap.map(forward,point,tolerance,localHolder,outerHolder,innerHolder,halfWidthsHolder);
    final Vector3Dd local=localHolder[0], halfWidths=halfWidthsHolder[0];
    final double outer=outerHolder[0], inner=innerHolder[0];
    if (mapped==AffinePlacementStatus.Invalid) return GeometryContainment.InvalidInput;
    if (mapped==AffinePlacementStatus.Unsupported) return GeometryContainment.Unsupported;
    if (mapped!=AffinePlacementStatus.Valid) return GeometryContainment.Indeterminate;
    // The axis-aligned local box also contains the preimage and is tighter
    // under anisotropic placement; it can only prove Inside or Outside.
    final GeometryContainment region=geometry.classifyBoxCertified(local,halfWidths);
    if (region==GeometryContainment.Inside || region==GeometryContainment.Outside ||
        region==GeometryContainment.InvalidInput) return region;
    // The outer local ball contains the preimage of the world ball, so a
    // definite side for it is a definite world side. A zero outer radius means
    // the preimage is exactly the local point.
    final GeometryContainment result=geometry.classifyPointCertified(local,outer);
    if (result==GeometryContainment.Inside || result==GeometryContainment.Outside ||
        result==GeometryContainment.InvalidInput || result==GeometryContainment.Unsupported) return result;
    if (result==GeometryContainment.Boundary && outer==0) return result;
    // A boundary inside the inner local ball maps inside the world ball.
    if (inner>0) {
        final GeometryContainment near=geometry.classifyPointCertified(local,inner);
        if (near==GeometryContainment.Boundary || near==GeometryContainment.InvalidInput ||
            near==GeometryContainment.Unsupported) return near;
    }
    if (!(tolerance>0)) return GeometryContainment.Indeterminate;
    // Under anisotropic placement the inner ball is small. Two world points of
    // the (convex) tolerance ball with certified opposite membership prove a
    // boundary point on the segment joining them.
    final GeometryContainment centre=classifyPlacedSolidCertified(geometry,forward,point,0);
    int centreSign=centre==GeometryContainment.Inside ? -1 : centre==GeometryContainment.Outside ? 1 : 0;
    boolean foundInside=centreSign<0, foundOutside=centreSign>0;
    final double step=tolerance*(1.0-1.0/1048576.0);
    final double diagonal=step/Math.sqrt(3.0)*(1.0-1.0/1048576.0);
    for (int direction=0;direction<14;++direction) {
        Vector3Dd offset = new Vector3Dd();
        if (direction<6) {
            final double sign=direction%2==0 ? 1.0 : -1.0;
            offset=new Vector3Dd(direction/2==0 ? sign*step : 0, direction/2==1 ? sign*step : 0,
                direction/2==2 ? sign*step : 0);
        } else {
            final int corner=direction-6;
            offset=new Vector3Dd((corner&1)!=0 ? -diagonal : diagonal, (corner&2)!=0 ? -diagonal : diagonal,
                (corner&4)!=0 ? -diagonal : diagonal);
        }
        final Vector3Dd sample=point.add(offset);
        FloatingInterval squared=FloatingInterval.exact(0);
        squared=squared.add(FloatingInterval.exact(sample.x()).subtract(FloatingInterval.exact(point.x())).square());
        squared=squared.add(FloatingInterval.exact(sample.y()).subtract(FloatingInterval.exact(point.y())).square());
        squared=squared.add(FloatingInterval.exact(sample.z()).subtract(FloatingInterval.exact(point.z())).square());
        final FloatingInterval distance=squared.squareRoot();
        if (!distance.isValid() || distance.getUpper()>tolerance) continue;
        final GeometryContainment side=classifyPlacedSolidCertified(geometry,forward,sample,0);
        if (side==GeometryContainment.Inside) foundInside=true;
        else if (side==GeometryContainment.Outside) foundOutside=true;
        else if (side==GeometryContainment.Boundary) return GeometryContainment.Boundary;
        if (foundInside && foundOutside) return GeometryContainment.Boundary;
    }
    return GeometryContainment.Indeterminate;
}

    private static GeometryContainment orientSide(GeometryContainment side, boolean inverted) {
    if (inverted && side==GeometryContainment.Inside) return GeometryContainment.Outside;
    if (inverted && side==GeometryContainment.Outside) return GeometryContainment.Inside;
    return side;
}

    private GeometryDistanceStatus queryDenotedSphere(Sphere[] sphere, boolean[] inverted, Matrix4x4d[] forward, Matrix4x4d[] inverse) {
    final Geometry[] intrinsic ={null};
    if (!queryDenotedSolid(intrinsic,inverted)) return GeometryDistanceStatus.Unsupported;
    sphere[0]=intrinsic[0] instanceof Sphere ? (Sphere)intrinsic[0] : null;
    if (sphere[0]==null) return GeometryDistanceStatus.Unsupported;
    final AffinePlacementStatus status=queryAffinePlacement(forward,inverse);
    if (status==AffinePlacementStatus.Invalid) return GeometryDistanceStatus.InvalidInput;
    if (status==AffinePlacementStatus.Unsupported) return GeometryDistanceStatus.Unsupported;
    if (status==AffinePlacementStatus.Indeterminate) return GeometryDistanceStatus.Indeterminate;
    return GeometryDistanceStatus.Valid;
}

    protected void markModified() { modificationVersion++; }

    protected boolean hasSphereGeometry() { return geometryIsSphere; }

    /**
    Whether the solid of this body is exactly its geometry under its
    placement, with no scene policy that changes membership. Only such bodies
    report solid bounds, certified box classification and an equivalence
    geometry. False by default, so unknown implementations do not opt in.
    */
    protected boolean denotesPlacedGeometry() { return false; }

    /**
    The intrinsic solid, and whether the body denotes its complement, when
    this body's solid is that solid under its placement with no scene policy
    changing membership. By default the geometry itself, only for bodies that
    `denotesPlacedGeometry`. Bodies that wrap or invert their geometry return
    the wrapped solid and the inversion. Box classification, solid
    equivalence and the boundary distances are computed from it.
    @return false when no such solid exists
    */
    protected boolean queryDenotedSolid(Geometry[] intrinsic, boolean[] inverted) {
    intrinsic[0] = null; inverted[0] = false;
    if (!denotesPlacedGeometry() || geometry == null) return false;
    intrinsic[0] = geometry;
    return true;
}

    // The single certified world-space point classification for an intrinsic
    // solid under a validated affine pair. Spheres use the ellipsoid distance
    // certificates; other solids map the tolerance ball to local inner and outer
    // balls. An inverted binding swaps Inside and Outside. Scene specializations
    // and CSG operands reuse this routine rather than reimplementing a certificate.
    public static GeometryContainment classifyPlacedBoxCertified(Geometry geometry, boolean inverted, Matrix4x4d inverse, GeometryBounds box) {
    if (box.getKind()==GeometryBounds.Kind.Unsupported) return GeometryContainment.Unsupported;
    if (box.getKind()!=GeometryBounds.Kind.Finite) return GeometryContainment.Indeterminate;
    // The conservative local bounds of the box's preimage, as centre and
    // outward-rounded half widths.
    final GeometryBounds local=box.transformed(inverse);
    if (local.getKind()!=GeometryBounds.Kind.Finite) return GeometryContainment.Indeterminate;
    final double[] lo = {local.getMinimum().x(),local.getMinimum().y(),local.getMinimum().z()};
    final double[] hi = {local.getMaximum().x(),local.getMaximum().y(),local.getMaximum().z()};
    double[] centre=new double[3], half=new double[3];
    for (int i=0;i<3;++i) {
        centre[i]=lo[i]*0.5+hi[i]*0.5;
        final FloatingInterval below=FloatingInterval.exact(centre[i]).subtract(FloatingInterval.exact(lo[i]));
        final FloatingInterval above=FloatingInterval.exact(hi[i]).subtract(FloatingInterval.exact(centre[i]));
        if (!Double.isFinite(centre[i]) || !below.isValid() || !above.isValid())
            return GeometryContainment.Indeterminate;
        half[i]=Math.max(Math.max(below.getUpper(),above.getUpper()),0.0);
    }
    final GeometryContainment result=geometry.classifyBoxCertified(new Vector3Dd(centre[0],centre[1],centre[2]),
        new Vector3Dd(half[0],half[1],half[2]));
    if (inverted && result==GeometryContainment.Inside) return GeometryContainment.Outside;
    if (inverted && result==GeometryContainment.Outside) return GeometryContainment.Inside;
    return result;
}

    public static GeometryContainment classifyPlacedPointCertified(Geometry geometry, boolean inverted, Matrix4x4d forward, Matrix4x4d inverse, Vector3Dd point, double tolerance) {
    if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) ||
        !Double.isFinite(point.z()) || !Double.isFinite(tolerance) || tolerance<0)
        return GeometryContainment.InvalidInput;
    final Sphere sphere =geometry instanceof Sphere ? (Sphere)geometry : null;
    final GeometryContainment result=sphere!=null ?
        classifyPlacedSphereCertified(sphere.getRadius(),forward,inverse,point,tolerance,128) :
        classifyPlacedSolidCertified(geometry,forward,point,tolerance);
    if (inverted && result==GeometryContainment.Inside) return GeometryContainment.Outside;
    if (inverted && result==GeometryContainment.Outside) return GeometryContainment.Inside;
    return result;
}

    public SceneSolid() {
        this.geometry = null;
        this.ownsGeometry = true;
        this.geometryIsSphere = false;
        this.name = "";
        this.modificationVersion = 0;
}

    public String getName() { return name; }

    public long getModificationVersion() { return modificationVersion; }

    public void setName(String n) { name = n; markModified(); }

    public Geometry getGeometry() { return geometry; }

    // Whether a CSG traversal may descend into this body's geometry as its
    // solid: exactly when the body denotes its placed geometry.
    public boolean isStructuralTraversalSafe() { return denotesPlacedGeometry(); }

    // Borrowed core solid only when scene-specific policies preserve it exactly.
    // Unknown subclasses must opt in explicitly; nullptr means not comparable.
    public Geometry getSolidEquivalenceGeometry() {
    final Geometry[] intrinsic ={null}; boolean[] inverted={false};
    return queryDenotedSolid(intrinsic,inverted) && !inverted[0] ? intrinsic[0] : null;
}

    /**
    Sets the geometry, taking its ownership (it is deleted with this body or
    when replaced).
    @param g new geometry, or null
    */
    public void setGeometry(Geometry g) {
    // cpp deletes the previously owned geometry here; Java leaves it to the GC.
    geometry = g;
    ownsGeometry = true;
    geometryIsSphere = geometry instanceof Sphere;
    markModified();
}

    /**
    Sets a geometry shared with other owners (i.e. a primitive instanced by
    several bodies): this body does not delete it.
    @param g shared geometry, or null
    */
    public void setGeometryReference(Geometry g) {
    // cpp deletes the previously owned geometry here; Java leaves it to the GC.
    geometry = g;
    ownsGeometry = false;
    geometryIsSphere = geometry instanceof Sphere;
    markModified();
}

    /// The material reported with this body's ray crossings, or null
    public Material getSurfaceMaterial() { return null; }

    // Column-vector affine geometry-to-parent placement, including all layers.
    public abstract Matrix4x4d getAffinePlacement();

    // On failure the previous placement remains unchanged.
    public abstract AffinePlacementStatus setAffinePlacement(Matrix4x4d forward, Matrix4x4d inverse);

    // Reset to identity.
    public abstract void resetAffinePlacement();

    public abstract AffinePlacementStatus queryAffinePlacement(Matrix4x4d[] forward, Matrix4x4d[] inverse);

    // Result is in the parent coordinate system of this body.
    public void getWorldBounds(GeometryBounds result) {
    Geometry shape = getGeometry();
    if (shape == null) { result.setEmpty(); return; }
    GeometryBounds local = new GeometryBounds();
    shape.getBounds(local);
    result.assign(local.transformed(getAffinePlacement()));
}

    // Conservative world bounds of the solid this body denotes. Only bodies
    // that denote their placed geometry, over Solid geometry, qualify; others
    // report Unsupported, so bounds are never mistaken for volumetric membership.
    public void getSolidWorldBounds(GeometryBounds result) {
    if (!denotesPlacedGeometry() || !(getGeometry() instanceof Solid)) {
        result.setUnsupported();
        return;
    }
    getWorldBounds(result);
}

    // Certified classification of a closed world-space axis-aligned box:
    // Inside/Outside only when the whole box lies strictly in the interior or
    // exterior of this body's solid; never Boundary.
    public GeometryContainment classifyWorldBoxCertified(GeometryBounds box) {
    final Geometry[] intrinsic ={null}; boolean[] inverted={false};
    if (!queryDenotedSolid(intrinsic,inverted)) return GeometryContainment.Unsupported;
    Matrix4x4d[] forward={new Matrix4x4d()},inverse={new Matrix4x4d()};
    final AffinePlacementStatus placement=queryAffinePlacement(forward,inverse);
    if (placement==AffinePlacementStatus.Invalid) return GeometryContainment.InvalidInput;
    if (placement==AffinePlacementStatus.Unsupported) return GeometryContainment.Unsupported;
    if (placement==AffinePlacementStatus.Indeterminate) return GeometryContainment.Indeterminate;
    return classifyPlacedBoxCertified(intrinsic[0],inverted[0],inverse[0],box);
}

    public abstract boolean doIntersectionFirstHit(Ray inOutRay, RayHit outHit);

    public abstract void doExtraInformation(Ray inRay, double inT, RayHit outData);

    public RayCrossingSupport emitRayCrossings(Ray ray, RayCrossingQuery query, RayCrossingSink sink) {
    if (geometry == null) return RayCrossingSupport.Unsupported;
    Matrix4x4d[] forward={new Matrix4x4d()}, inverse={new Matrix4x4d()};
    final AffinePlacementStatus status = queryAffinePlacement(forward, inverse);
    if (status == AffinePlacementStatus.Unsupported) return RayCrossingSupport.Unsupported;
    if (status == AffinePlacementStatus.Invalid) return RayCrossingSupport.InvalidInput;
    if (status == AffinePlacementStatus.Indeterminate) return RayCrossingSupport.Indeterminate;
    final Vector3Dd origin = inverse[0].multiply(ray.getOrigin());
    final Vector3Dd direction = inverse[0].withoutTranslation().multiply(ray.getDirection());
    if (!Double.isFinite(origin.x()) || !Double.isFinite(origin.y()) ||
        !Double.isFinite(origin.z()) || !Double.isFinite(direction.x()) ||
        !Double.isFinite(direction.y()) || !Double.isFinite(direction.z()))
        return RayCrossingSupport.Indeterminate;
    final Ray local = Ray.fromAffine(origin, direction, ray.getT());
    InstanceRayCrossingBuffer buffer = new InstanceRayCrossingBuffer(new RayInstanceStep(this,forward[0],inverse[0]), this, getSurfaceMaterial());
    final RayCrossingSupport result = geometry.emitRayCrossings(local,query,buffer);
    if (result != RayCrossingSupport.Supported) return result;
    if (!buffer.isValid()) return RayCrossingSupport.Indeterminate;
    buffer.deliver(sink);
    return RayCrossingSupport.Supported;
}

    // Zero tolerance classifies the mapped local point. Positive world-space
    // tolerance uses the certified classification below.
    public GeometryContainment classifyPoint(Vector3Dd point, double tolerance) {
    if (geometry == null) return GeometryContainment.Unsupported;
    if (!Double.isFinite(tolerance) || tolerance < 0 ||
        !Double.isFinite(point.x()) || !Double.isFinite(point.y()) ||
        !Double.isFinite(point.z())) return GeometryContainment.InvalidInput;
    if (tolerance == 0) {
        Matrix4x4d[] forward={new Matrix4x4d()}, inverse={new Matrix4x4d()};
        final AffinePlacementStatus status = queryAffinePlacement(forward, inverse);
        if (status == AffinePlacementStatus.Unsupported) return GeometryContainment.Unsupported;
        if (status == AffinePlacementStatus.Invalid) return GeometryContainment.InvalidInput;
        if (status == AffinePlacementStatus.Indeterminate) return GeometryContainment.Indeterminate;
        final Vector3Dd local = inverse[0].multiply(point);
        if (!Double.isFinite(local.x()) || !Double.isFinite(local.y()) ||
            !Double.isFinite(local.z())) return GeometryContainment.Indeterminate;
        return geometry.classifyPoint(local, 0);
    }
    return classifyPointCertified(point, tolerance);
}

    // Numerical estimate for placed spheres; estimatedError is not certified.
    // On failure both output arguments remain unchanged.
    public GeometryDistanceStatus estimateBoundaryDistance(Vector3Dd point, double[] distance, double[] estimatedError) {
    if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) ||
        !Double.isFinite(point.z())) return GeometryDistanceStatus.InvalidInput;
    final Sphere[] sphere ={null}; boolean[] inverted={false};
    Matrix4x4d[] forward={new Matrix4x4d()},inverse={new Matrix4x4d()};
    final GeometryDistanceStatus availability=queryDenotedSphere(sphere,inverted,forward,inverse);
    if (availability!=GeometryDistanceStatus.Valid) return availability;
    return AffineEllipsoidDistance.query(point,sphere[0].getRadius(),forward[0],distance,estimatedError);
}

    // Certified enclosure for a placed sphere and a nonzero local direction.
    // The lower bound may be zero; all outputs are unchanged on failure.
    public GeometryDistanceStatus encloseBoundaryDistance(Vector3Dd point, Vector3Dd localDirection, GeometryContainment[] side, double[] lower, double[] upper) {
    final Sphere[] sphere ={null}; boolean[] inverted={false};
    Matrix4x4d[] forward={new Matrix4x4d()},inverse={new Matrix4x4d()};
    final GeometryDistanceStatus availability=queryDenotedSphere(sphere,inverted,forward,inverse);
    if (availability!=GeometryDistanceStatus.Valid) return availability;
    GeometryContainment[] candidateSide={GeometryContainment.Indeterminate};
    double[] candidateLower={0},candidateUpper={0};
    final GeometryDistanceStatus status=AffineEllipsoidDistanceBounds.query(point,sphere[0].getRadius(),
        forward[0],localDirection,candidateSide,candidateLower,candidateUpper);
    if (status!=GeometryDistanceStatus.Valid) return status;
    side[0]=orientSide(candidateSide[0],inverted[0]); lower[0]=candidateLower[0]; upper[0]=candidateUpper[0];
    return GeometryDistanceStatus.Valid;
}

    // Certified minimum-distance enclosure no wider than maxWidth. Failure
    // leaves all outputs untouched; currently supported for placed spheres.
    public GeometryDistanceStatus encloseMinimumBoundaryDistance(Vector3Dd point, double maxWidth, GeometryContainment[] side, double[] lower, double[] upper) {
    final Sphere[] sphere ={null}; boolean[] inverted={false};
    Matrix4x4d[] forward={new Matrix4x4d()},inverse={new Matrix4x4d()};
    final GeometryDistanceStatus availability=queryDenotedSphere(sphere,inverted,forward,inverse);
    if (availability!=GeometryDistanceStatus.Valid) return availability;
    GeometryContainment[] candidateSide={GeometryContainment.Indeterminate};
    double[] candidateLower={0},candidateUpper={0};
    final GeometryDistanceStatus status=AffineEllipsoidMinimumDistance.query(point,sphere[0].getRadius(),
        forward[0],inverse[0],maxWidth,candidateSide,candidateLower,candidateUpper);
    if (status!=GeometryDistanceStatus.Valid) return status;
    side[0]=orientSide(candidateSide[0],inverted[0]); lower[0]=candidateLower[0]; upper[0]=candidateUpper[0];
    return GeometryDistanceStatus.Valid;
}

    // Only decides when the enclosure proves the result; otherwise Indeterminate.
    public GeometryContainment classifyPointCertified(Vector3Dd point, double tolerance) {
    if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) ||
        !Double.isFinite(point.z()) || !Double.isFinite(tolerance) || tolerance<0)
        return GeometryContainment.InvalidInput;
    if (geometry==null) return GeometryContainment.Unsupported;
    Matrix4x4d[] forward={new Matrix4x4d()},inverse={new Matrix4x4d()};
    final AffinePlacementStatus placement=queryAffinePlacement(forward,inverse);
    if (placement==AffinePlacementStatus.Invalid) return GeometryContainment.InvalidInput;
    if (placement==AffinePlacementStatus.Unsupported) return GeometryContainment.Unsupported;
    if (placement==AffinePlacementStatus.Indeterminate) return GeometryContainment.Indeterminate;
    return classifyPlacedPointCertified(geometry,false,forward[0],inverse[0],point,tolerance);
}

    public RaySolidQueryStatus classifyRay(Ray ray, RayCrossingQuery query, RaySolidClassification result) {
    result.clear();
    if (geometry == null) return RaySolidQueryStatus.Unsupported;
    Matrix4x4d[] childToParentHolder={new Matrix4x4d()}, parentToChildHolder={new Matrix4x4d()};
    final AffinePlacementStatus placement = queryAffinePlacement(childToParentHolder, parentToChildHolder);
    if (placement == AffinePlacementStatus.Unsupported) return RaySolidQueryStatus.Unsupported;
    if (placement == AffinePlacementStatus.Invalid) return RaySolidQueryStatus.InvalidInput;
    if (placement == AffinePlacementStatus.Indeterminate) return RaySolidQueryStatus.Indeterminate;
    final Matrix4x4d childToParent=childToParentHolder[0], parentToChild=parentToChildHolder[0];
    final Vector3Dd origin = ray.getOrigin();
    final Vector3Dd direction = ray.getDirection();
    if (!Double.isFinite(origin.x()) || !Double.isFinite(origin.y()) ||
        !Double.isFinite(origin.z()) || !Double.isFinite(direction.x()) ||
        !Double.isFinite(direction.y()) || !Double.isFinite(direction.z()))
        return RaySolidQueryStatus.InvalidInput;
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 4; column++) {
            if (!Double.isFinite(childToParent.get(row,column)) ||
                !Double.isFinite(parentToChild.get(row,column))) return RaySolidQueryStatus.Indeterminate;
        }
    }
    final Vector3Dd localOrigin = parentToChild.multiply(origin);
    final Vector3Dd localDirection = parentToChild.withoutTranslation().multiply(direction);
    if (!Double.isFinite(localOrigin.x()) || !Double.isFinite(localOrigin.y()) ||
        !Double.isFinite(localOrigin.z()) || !Double.isFinite(localDirection.x()) ||
        !Double.isFinite(localDirection.y()) || !Double.isFinite(localDirection.z()))
        return RaySolidQueryStatus.Indeterminate;
    final Ray localRay = Ray.fromAffine(localOrigin, localDirection, ray.getT());
    final RaySolidQueryStatus status = geometry.classifyRay(localRay, query, result);
    if (status != RaySolidQueryStatus.Supported) { result.clear(); return status; }
    final RayInstanceStep step = new RayInstanceStep(this, childToParent, parentToChild);
    for (int i = 0; i < result.boundaries.size(); i++) {
        if (!result.boundaries.get(i).appendInstance(step)) {
            result.clear();
            return RaySolidQueryStatus.Indeterminate;
        }
    }
    return status;
}
}
