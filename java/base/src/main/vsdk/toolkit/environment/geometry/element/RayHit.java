package vsdk.toolkit.environment.geometry.element;

import java.io.Serial;

import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.statistics.RaytraceStatistics;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.media.Image;
import vsdk.toolkit.media.NormalMap;
import vsdk.toolkit.environment.material.SimpleMaterial;

/**
RayHit describes the result of a ray/geometry intersection.
*/
public class RayHit extends FundamentalEntity {
    @Serial
    private static final long serialVersionUID = 20260420L;

    public static final int DETAIL_NONE = SurfaceDetail.NONE;
    public static final int DETAIL_POINT = SurfaceDetail.POINT;
    public static final int DETAIL_NORMAL = SurfaceDetail.NORMAL;
    public static final int DETAIL_UV = SurfaceDetail.UV;
    public static final int DETAIL_TANGENT = SurfaceDetail.TANGENT;
    public static final int DETAIL_ALL =
        SurfaceDetail.ALL;

    private static final Vector3Dd ZERO_VECTOR = new Vector3Dd();

    public Vector3Dd point; // Intersection point coordinates
    public Vector3Dd normal; // Surface normal at intersection point
    public Vector3Dd tangent; // Surface tangent at intersection point
    // Note that surface binormal at intersection point must be calculated
    // by the application as the cross product (n x t).

    public double u; // Texture coordinate of intersection point
    public double v;

    // This can be null.
    public SimpleMaterial material; // Internal geometry selected material

    // This can be null.
    public Image texture; // Internal geometry selected texture map

    // This can be null.
    public NormalMap normalMap; // Internal geometry selected normal map

    // This can be null for miss results.
    private Ray ray; // Intersected ray with valid t at hit point

    private int requiredDetailMask;
    private int availableDetailMask;
    private boolean detailAvailabilityKnown;
    private boolean storeRay;
    private double hitDistance;
    private boolean hitDistanceKnown;

    public RayHit()
    {
        this(DETAIL_ALL, true);
    }

    public RayHit(int requiredDetailMask)
    {
        this(requiredDetailMask, true);
    }

    public RayHit(int requiredDetailMask, boolean storeRay)
    {
        this.requiredDetailMask = requiredDetailMask;
        this.storeRay = storeRay;
        clear();
        RaytraceStatistics.recordRayHitInstance();
    }

    public RayHit(RayHit other)
    {
        this(other.requiredDetailMask);
        clone(other);
    }

    public final void clear()
    {
        point = ZERO_VECTOR;
        normal = ZERO_VECTOR;
        tangent = ZERO_VECTOR;
        u = 0;
        v = 0;
        material = null;
        texture = null;
        normalMap = null;
        ray = null;
        availableDetailMask = DETAIL_NONE;
        detailAvailabilityKnown = false;
        hitDistance = 0;
        hitDistanceKnown = false;
    }

    public final void reset(int newRequiredDetailMask)
    {
        requiredDetailMask = newRequiredDetailMask;
        clear();
    }

    public void resetForDistanceOnly()
    {
        requiredDetailMask = DETAIL_NONE;
        ray = null;
        availableDetailMask = DETAIL_NONE;
        detailAvailabilityKnown = false;
        hitDistance = 0;
        hitDistanceKnown = false;
    }

    public final void clone(RayHit other)
    {
        RaytraceStatistics.recordHitInfoClone();
        this.requiredDetailMask = other.requiredDetailMask;
        this.storeRay = other.storeRay;
        this.availableDetailMask = other.availableDetailMask;
        this.detailAvailabilityKnown = other.detailAvailabilityKnown;
        this.hitDistance = other.hitDistance;
        this.hitDistanceKnown = other.hitDistanceKnown;
        this.point = other.point;
        this.normal = other.normal;
        this.tangent = other.tangent;
        this.u = other.u;
        this.v = other.v;
        this.material = other.material;
        this.texture = other.texture;
        this.normalMap = other.normalMap;
        this.ray = other.ray;
    }

    // Legacy producers may leave availability unknown; requested is not available.
    public boolean isDetailAvailabilityKnown() { return detailAvailabilityKnown; }
    public int getAvailableDetailMask() { return availableDetailMask; }
    public void beginDetailEvaluation() { availableDetailMask = DETAIL_NONE; detailAvailabilityKnown = true; }
    public void markDetailAvailable(int mask) { availableDetailMask |= mask & DETAIL_ALL; }

    public int getRequiredDetailMask()
    {
        return requiredDetailMask;
    }

    public void setRequiredDetailMask(int requiredDetailMask)
    {
        this.requiredDetailMask = requiredDetailMask;
    }

    public boolean shouldStoreRay()
    {
        return storeRay;
    }

    public void setStoreRay(boolean storeRay)
    {
        this.storeRay = storeRay;
    }

    public boolean needsPoint()
    {
        return (requiredDetailMask & DETAIL_POINT) != 0;
    }

    public boolean needsNormal()
    {
        return (requiredDetailMask & DETAIL_NORMAL) != 0;
    }

    public boolean needsTextureCoordinates()
    {
        return (requiredDetailMask & DETAIL_UV) != 0;
    }

    public boolean needsTangent()
    {
        return (requiredDetailMask & DETAIL_TANGENT) != 0;
    }

    public boolean needsAnySurfaceData()
    {
        return requiredDetailMask != DETAIL_NONE;
    }

    public Ray getRay()
    {
        return ray;
    }

    public void setRay(Ray ray)
    {
        this.ray = ray;
        if ( ray != null ) {
            hitDistance = ray.getT();
            hitDistanceKnown = true;
        }
    }

    public boolean hasHitDistance()
    {
        return hitDistanceKnown;
    }

    public double getHitDistance()
    {
        if ( hitDistanceKnown ) {
            return hitDistance;
        }
        if ( ray != null ) {
            return ray.getT();
        }
        return 0;
    }

    public void setHitDistance(double hitDistance)
    {
        this.hitDistance = hitDistance;
        this.hitDistanceKnown = true;
    }
}
