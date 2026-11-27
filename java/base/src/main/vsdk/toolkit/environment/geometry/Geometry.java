//= References:                                                             =
//= [APPE1967] Appel, Arthur. "The notion of quantitative invisivility and  =
//=          the machine rendering of solids". Proceedings, ACM National    =
//=          meeting 1967.                                                  =

package vsdk.toolkit.environment.geometry;
import java.io.Serial;

import vsdk.toolkit.common.Entity;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.environment.geometry.element.GeometryBounds;
import vsdk.toolkit.environment.geometry.element.GeometryContainment;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.element.RayBoundary;
import vsdk.toolkit.environment.geometry.element.RayFirstHitStatus;
import vsdk.toolkit.environment.geometry.element.RaySolidClassification;
import vsdk.toolkit.environment.geometry.element.RaySolidQueryStatus;
import vsdk.toolkit.environment.geometry.element.RayCrossingQuery;
import vsdk.toolkit.environment.geometry.element.RayCrossingSink;
import vsdk.toolkit.environment.geometry.element.RayCrossingSupport;

import vsdk.toolkit.environment.geometry.element.RayHit;
import vsdk.toolkit.processing.Containment;

/**
Every geometric entity (prefer not to call it "Object", for not confusing
it with the Object Oriented Programming concept or the base Java superclass)
need to describe its form, its geometric transformation, material and other
properties. The sub-classes of Geometry are responsible of representing
the form only, given that this form is located in its own origin, 
without rotation or scaling.<P>

This abstract class defines the required basic operations that must be
supplied by all geometries in order to be managed uniformly by every
geometric algorithm, including but not limited to rendering operations.<P>
*/
public abstract class Geometry extends Entity {
    /** An independent mathematical copy, or null when copying is unsupported. */
    public Geometry copyGeometry() { return null; }

    /** Unsupported is distinct from an empty sequence of crossings. */
    public RayCrossingSupport emitRayCrossings(Ray ray, RayCrossingQuery query, RayCrossingSink sink) { return RayCrossingSupport.Unsupported; }

    /**
    Full solid membership over the query interval; never infer interior from
    surface hits alone. Unsupported geometries leave result empty.
    */
    public RaySolidQueryStatus classifyRay(Ray ray, RayCrossingQuery query,
        RaySolidClassification result)
    {
        result.clear();
        return RaySolidQueryStatus.Unsupported;
    }

    /** Inclusive explicit interval; unsupported implementations clear output. */
    public final RayFirstHitStatus queryFirstHit(Ray ray, RayCrossingQuery query, RayHit result)
    {
        return queryFirstHit(ray, query, result, null);
    }

    /** Inclusive explicit interval; unsupported implementations clear output. */
    public RayFirstHitStatus queryFirstHit(Ray ray, RayCrossingQuery query, RayHit result,
        RayBoundary boundary)
    {
        if (result != null) result.clear();
        if (boundary != null) boundary.assign(new RayBoundary());
        return RayFirstHitStatus.Unsupported;
    }

    /**
    Resolve detail for a boundary in this geometry's local coordinates.
    Hit means evaluated; unsupported and numerical failures remain explicit.
    */
    public RayFirstHitStatus querySurfaceDetail(Ray ray, RayBoundary boundary, RayHit result)
    {
        if (result != null) result.clear();
        return RayFirstHitStatus.Unsupported;
    }

    /** Unsupported is explicit; legacy getMinMax remains available. */
    public void getBounds(GeometryBounds result) { result.setUnsupported(); }

    /** Typed containment; unsupported is explicit here. */
    public GeometryContainment classifyPoint(Vector3Dd p, double distanceTolerance)
    { return GeometryContainment.Unsupported; }

    /**
    Certified local-space classification of the closed ball of radius
    distanceTolerance around the point. Inside/Outside: the whole ball lies
    strictly in the interior/exterior. Boundary: a boundary point lies within
    the ball. Anything not proven, including at zero tolerance, is Indeterminate.
    */
    public GeometryContainment classifyPointCertified(Vector3Dd p, double distanceTolerance)
    {
        return GeometryContainment.Unsupported;
    }

    /**
    Certified classification of the closed axis-aligned local box
    center +/- halfWidths: Inside/Outside only when the whole box lies strictly
    in the interior/exterior; never Boundary. Other results are Indeterminate.
    */
    public GeometryContainment classifyBoxCertified(Vector3Dd center, Vector3Dd halfWidths)
    {
        return GeometryContainment.Unsupported;
    }

    @SuppressWarnings("FieldNameHidesFieldInSuperclass")
    @Serial private static final long serialVersionUID = 20150218L;
    public static final int INSIDE = Containment.INSIDE.value();
    public static final int LIMIT = Containment.LIMIT.value();
    public static final int OUTSIDE = Containment.OUTSIDE.value();

    /**
    Takes in a Ray and checks wheter or not that Ray intersects the current 
    Geometry.  The coordinates of the Ray are interpreted as global 
    coordinates, and the current Geometry is supposed to be located in 
    coordinates around the origin, without  rotation or scaling.<P>

    USES/APPLICATIONS:
    This method is one of the most fundamental and important of the whole
    VSDK toolkit. All Surface or Solid Geometry must implement it in order
    to support a great variety of applications, which include but are not
    limited to:
    <UL>
        <LI> Rendering scenes using Raytracing or Raycasting like methods.
        Currently, the class SimpleRaytracer makes use of this operation for 
        visualizing scenes.
        <LI> Selecting objects interactively from a mouse click specified
        by a user, in cooperation with the Camera.generateRay method.
        <LI> Implementing simple forms of collision detection in the case
        of particle / Geometry interaction, and posibly as an aid in other
        more complexes cases of collision detection. 
        <LI> Collision detection of a Ray generated from the position of an 
        object and pointing in the direction of a gravity field, to measure
        altitude over a terrain.
        <LI> Partial implementation of the more complex operation of
        doRayClassification which is defined only in the Solid subclass,
        and could be useful for volumetric rendering and constructive
        solid geometry (CSG).
        <LI> Ray path following in an scene. Beside the Raycasting case for
        visualization, this could be useful in the modelling of interactions
        between sound or electromagnetic waves and its environment. This
        could be used to model problem like radar/sonar or auralization and
        sound spacialization.
        <LI>As part of skinning operations, where a "skin" Geometry is
        modified to fit an underlying base "skeleton" or "muscle" Geometry, 
        this operation could be used to calculate the nearest base point.
    </UL><P>

    ILLUSTRATION: As a path following example take into account the red Sphere
    and the gray Box shown in the figure. From the point indicated by the green
    Sphere emanates a Ray in the direction shown by the cyan Arrow. The 
    Geometry.doIntersectionFirstHit method returns true the first time is called,
    and the Ray is modified to have the distance between the Ray origin and
    the red Sphere surface in its Ray.t() attribute. From this operation,
    the Geometry.doExtraInformation method could be used to recall the
    normal Vector3Dd of the red Sphere, which is shown in yellow. Applying a
    reflection rule (equal incident and reflection angles respective to
    the normal) a second ray can be generated and the process repeated.<P>
        <IMG SRC="../images/Geometry.doIntersection_1.jpg"></IMG><P>


    @param inOut_ray It is used both ways: as an input value and as an output
    returned value. As input parameter, it must specify the ray origin and
    ray direction in the two internal Vector3Dd fields, it's scalar
    distance value Ray.t() is not relevant.  As output value, the origin and 
    direction of the Ray are not altered, but the scalar distance value Ray.t() 
    could change if the input Ray intersects current geometry.
    @return If the specified input Ray intersects current Geometry, true
    value is returned, otherwise false is returned.
    */
    public abstract boolean doIntersectionFirstHit(Ray inRay, RayHit outHit);

    /**
    Rebuilds additional surface information for a ray/parameter pair already
    known to intersect this geometry.

    <p>Subclasses should override this when they can compute hit attributes
    such as point, normal, texture coordinates or tangent without repeating
    the intersection search.
    */
    public void doExtraInformation(Ray inRay, double inT, RayHit outHit)
    {
    }

    /**
    This method returns the number of front facing surface elements (with
    respect to `origin`) between the `origin` point and the `p` point. The
    internal working of this method is usually related with the
    `doIntersectionFirstHit` method.

    This operation is used in queries as the specified on [APPE1967]. Note,
    that as stated on [APPE1967]: "...A bounded surface hides a point when the
    line of sight to that point pierces the surface within the surface
    boundaries, and the piercing point is closer to the observer than the
    point being tested for visibility...". Acording to that definition:
    @param origin contains the "observer" position.
    @param p is the "point being tested for visibility".

    @return The number of surfaces within current Geometry hiding giving point.
    Default (empty) implementation here reports any point as non-obscured,
    that is "phamtom" or "transparent" geometry with respect to visibility.
    This method should be overloaded for each Geometry's subclass.
    */
    public int computeQuantitativeInvisibility(Vector3Dd origin, Vector3Dd p)
    {
        return 0;
    }

    /**
    This operation returns a simple bounding volume specification in the
    form of a "min-max box", which is a paralelogram aligned with the
    axes, when the geometry is centered in the origin, without scaling or
    rotation.

    @return A 6 position vector of doubles, with the following 
    positions interpretation:
    [0]: minimum value in x coordinate
    [1]: minimum value in y coordinate
    [2]: minimum value in z coordinate
    [3]: maximum value in x coordinate
    [4]: maximum value in y coordinate
    [5]: maximum value in z coordinate
    */
    public abstract double[] getMinMax();

    /**
    Legacy integer predicate kept for parity with the Java port. It is not
    the typed contract: its tolerance band is family specific, the default
    reports OUTSIDE for unsupported geometries, and surfaces report LIMIT only
    on the surface. Typed scene and CSG queries never consult it. Adapters for
    solids introduced with the typed contract raise when a typed result has
    no integer representation.<P>

    Given a point `p`, this method classifies if point lies `INSIDE` current
    geometry, `OUTSIDE` current geometry or `LIMIT` if it is near the geometry
    at some specified `distanceTolerance`.

    Note that the `INSIDE` and `OUTSIDE` cases are only feasible in `Solid`s,
    while `LIMIT` cases can be defined in 0D, 1D and 2D geometries.
    @param p
    @param distanceTolerance
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    public int doContainmentTest(Vector3Dd p, double distanceTolerance)
    {
        return OUTSIDE;
    }

}
