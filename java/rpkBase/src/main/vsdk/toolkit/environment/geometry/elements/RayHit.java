package vsdk.toolkit.environment.geometry.elements;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.material.PhongEmittanceDistributionFunction;
import vsdk.toolkit.material.ShadingContext;
import vsdk.toolkit.material.ShadingContextFlag;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;

/**
Hit record structure, returned by ray-object intersection routines and
used as a parameter for BSDF/EDF queries.
*/
public class RayHit {
    private Vector3Df point; // Intersection point
    private Patch patch; // Patch that was hit
    private Vector3Df texCoord; // Texture coordinate
    private Vector3Df geometricNormal;
    private RpkMaterial material; // RpkMaterial of hit surface
    // Shading frame (Z = shading normal: hit->shadingFrame.getZ() == hit->normal)
    private CoordinateSystem shadingFrame;
    private Vector2Dd uv; // Bi-linear / barycentric parameters of hit
    // Flags indicating which of the above fields have been filled in
    private int flags;

    public RayHit() {
        point = new Vector3Df();
        patch = null;
        texCoord = new Vector3Df();
        geometricNormal = new Vector3Df();
        material = null;
        shadingFrame = new CoordinateSystem();
        uv = new Vector2Dd();
        flags = 0;
    }

    /**
    Checks whether or not the hit record is properly initialised, that
    means that at least patch or geometry plus point, geometricNormal, material
    and distance are initialised. Returns TRUE if the structure is properly
    initialised and FALSE if not.
    */
    private boolean hitInitialised() {
        return (((flags & RayHitFlag.PATCH) != 0) || ((flags & RayHitFlag.GEOMETRY) != 0))
            && ((flags & RayHitFlag.POINT) != 0)
            && ((flags & RayHitFlag.GEOMETRIC_NORMAL) != 0)
            && ((flags & RayHitFlag.MATERIAL) != 0)
            && ((flags & RayHitFlag.DISTANCE) != 0);
    }

    /**
    Fills in (u,v) parameters of hit point on the hit patch, computing it if not
    computed before. Returns FALSE if the (u,v) parameters could not be determined.
    */
    private boolean computeUv(Vector2Dd inUv) {
        if ((flags & RayHitFlag.UV) != 0) {
            inUv.x = uv.x;
            inUv.y = uv.y;
            return true;
        }

        if (((flags & RayHitFlag.PATCH) != 0) && ((flags & RayHitFlag.POINT) != 0) && patch != null) {
            double[] u = new double[] {0.0};
            double[] v = new double[] {0.0};
            patch.uv(point, u, v);
            uv.x = u[0];
            uv.y = v[0];
            inUv.x = uv.x;
            inUv.y = uv.y;
            flags |= RayHitFlag.UV;
            return true;
        }

        return false;
    }

    /**
    Computes shading frame at hit point. Z is the shading normal. Returns FALSE
    if the shading frame could not be determined.
    If X and Y are null pointers, only the shading normal is returned in Z
    possibly avoiding computations of the X and Y axis.
    */
    private boolean pointShadingFrame(Vector3Df[] inX, Vector3Df[] inY, Vector3Df[] inZ) {
        boolean success = false;

        if (!hitInitialised()) {
            Logger.reportMessage(this, Logger.WARNING, "pointShadingFrame", "uninitialised hit structure");
            return false;
        }

        ShadingContext context = new ShadingContext(
            point,
            geometricNormal,
            shadingFrame.getZ(),
            texCoord,
            uv,
            shadingFrame,
            material,
            0);

        if (material != null && material.getBsdf() != null && context != null) {
            success = PhongBidirectionalScatteringDistributionFunction.bsdfShadingFrame(
                context, inX != null ? inX[0] : null, inY != null ? inY[0] : null, inZ != null ? inZ[0] : null);
        }

        if (!success && material != null && material.getEdf() != null && context != null) {
            success = PhongEmittanceDistributionFunction.edfShadingFrame(
                context, inX != null ? inX[0] : null, inY != null ? inY[0] : null, inZ != null ? inZ[0] : null);
        }

        if (!success && computeUv(uv) && patch != null) {
            // Make default shading frame
            Vector3Df[] x = inX != null ? inX : new Vector3Df[1];
            Vector3Df[] y = inY != null ? inY : new Vector3Df[1];
            Vector3Df[] z = inZ != null ? inZ : new Vector3Df[1];
            patch.interpolatedFrameAtUv(uv.x, uv.y, x, y, z);
            success = true;
        }

        return success;
    }

    /**
    Initialises a hit record. Either patch or geometry shall be non-null. Returns
    TRUE if the structure is properly initialised and FALSE if not.
    This routine can be used in order to construct BSDF queries at other positions
    than hit positions returned by ray intersection routines.
    */
    public boolean init(Patch inPatch, Vector3Df inPoint, Vector3Df inGeometryNormal, RpkMaterial inMaterial) {
        flags = 0;
        patch = inPatch;
        if (inPatch != null) {
            flags |= RayHitFlag.PATCH;
        }

        if (inPoint != null) {
            point = inPoint;
            flags |= RayHitFlag.POINT;
        }

        if (inGeometryNormal != null) {
            geometricNormal = inGeometryNormal;
            flags |= RayHitFlag.GEOMETRIC_NORMAL;
        }

        material = inMaterial;
        flags |= RayHitFlag.MATERIAL;
        flags |= RayHitFlag.DISTANCE;

        Vector3Df localNormal = new Vector3Df(0.0f, 0.0f, 0.0f);
        texCoord = localNormal;
        shadingFrame.setX(localNormal);
        shadingFrame.setY(localNormal);
        shadingFrame.setZ(localNormal);
        uv.x = 0.0;
        uv.y = 0.0;

        return hitInitialised();
    }

    /**
    Fills in/computes texture coordinates of hit point
    (outTexCoord is a one-element output holder).
    */
    public boolean getTexCoord(Vector3Df[] outTexCoord) {
        if ((flags & RayHitFlag.TEXTURE_COORDINATE) != 0) {
            outTexCoord[0] = texCoord;
            return true;
        }

        if (!computeUv(uv)) {
            return false;
        }

        if ((flags & RayHitFlag.PATCH) != 0 && patch != null) {
            texCoord = patch.textureCoordAtUv(uv.x, uv.y);
            outTexCoord[0] = texCoord;
            flags |= RayHitFlag.TEXTURE_COORDINATE;
            return true;
        }

        return false;
    }

    /**
    Fills in shading normal (Z axis of shading frame) only, avoiding computation
    of shading X and Y axis if possible (inNormal is a one-element output
    holder).
    */
    public boolean shadingNormal(Vector3Df[] inNormal) {
        if (((flags & RayHitFlag.SHADING_FRAME) != 0) || ((flags & RayHitFlag.NORMAL) != 0)) {
            inNormal[0] = shadingFrame.getZ();
            return true;
        }

        Vector3Df[] localNormal = {shadingFrame.getZ()};
        if (!pointShadingFrame(null, null, localNormal)) {
            return false;
        }

        flags |= RayHitFlag.NORMAL;
        shadingFrame.setZ(localNormal[0]);
        inNormal[0] = shadingFrame.getZ();
        return true;
    }

    /**
    Fills in shading frame: Z is the shading normal.
    */
    public boolean setShadingFrame(CoordinateSystem frame) {
        if ((flags & RayHitFlag.SHADING_FRAME) != 0) {
            frame.setX(shadingFrame.getX());
            frame.setY(shadingFrame.getY());
            frame.setZ(shadingFrame.getZ());
            return true;
        }

        Vector3Df[] shadingX = {shadingFrame.getX()};
        Vector3Df[] shadingY = {shadingFrame.getY()};
        Vector3Df[] shadingZ = {shadingFrame.getZ()};

        if (!pointShadingFrame(shadingX, shadingY, shadingZ)) {
            return false;
        }

        shadingFrame.setX(shadingX[0]);
        shadingFrame.setY(shadingY[0]);
        shadingFrame.setZ(shadingZ[0]);
        flags |= RayHitFlag.SHADING_FRAME | RayHitFlag.NORMAL;

        frame.setX(shadingFrame.getX());
        frame.setY(shadingFrame.getY());
        frame.setZ(shadingFrame.getZ());
        return true;
    }

    public Patch getPatch() {
        return patch;
    }

    public void setPatch(Patch inPatch) {
        patch = inPatch;
    }

    public Vector3Df getPoint() {
        return point;
    }

    public void setPoint(Vector3Df position) {
        point = position;
    }

    public void setGeometricNormal(Vector3Df inNormal) {
        geometricNormal = inNormal;
    }

    public void setMaterial(RpkMaterial inMaterial) {
        material = inMaterial;
    }

    public Vector2Dd getUv() {
        return uv;
    }

    /**
    Sets the (u,v) parameters of the hit and marks them as known. Everything
    derived from (u,v) and cached in the record, the texture coordinate and the
    interpolated shading normal and frame, is invalidated, so that a record
    reused for several samples on one patch evaluates each sample at its own
    (u,v) and not at the first one.
    */
    public void setUv(double inU, double inV) {
        uv.x = inU;
        uv.y = inV;
        flags = (flags | RayHitFlag.UV)
            & ~(RayHitFlag.TEXTURE_COORDINATE | RayHitFlag.NORMAL | RayHitFlag.SHADING_FRAME);
    }

    public void setUv(Vector2Dd inUv) {
        setUv(inUv.x, inUv.y);
    }

    public int getFlags() {
        return flags;
    }

    public void setFlags(int inFlags) {
        flags = inFlags;
    }

    public Vector3Df getNormal() {
        return shadingFrame.getZ();
    }

    public void setNormal(Vector3Df n) {
        shadingFrame.setZ(n);
    }

    public CoordinateSystem getShadingFrame() {
        return shadingFrame;
    }

    public RpkMaterial getMaterial() {
        return material;
    }

    public Vector3Df getGeometricNormal() {
        return geometricNormal;
    }

    public void setShadingFrame(Vector3Df inX, Vector3Df inY, Vector3Df inZ) {
        shadingFrame.setX(inX);
        shadingFrame.setY(inY);
        shadingFrame.setZ(inZ);
    }

    /**
    Plain member-wise copy, equivalent to the C++ port's structure assignment
    (*hitStore = hit). Fields not marked as filled in by the flags are copied
    as-is and remain meaningless, just as in the C++ version; readers must
    check the flags (or use the getTexCoord/shadingNormal accessors, which
    compute missing data on demand).
    */
    public void copyFrom(RayHit other) {
        if (other == null) {
            return;
        }

        point = other.point;
        patch = other.patch;
        texCoord = other.texCoord;
        geometricNormal = other.geometricNormal;
        material = other.material;
        shadingFrame.setX(other.shadingFrame.getX());
        shadingFrame.setY(other.shadingFrame.getY());
        shadingFrame.setZ(other.shadingFrame.getZ());
        uv.x = other.uv.x;
        uv.y = other.uv.y;
        flags = other.flags;
    }

    public ShadingContext shadingContext() {
        Vector3Df[] normal = {new Vector3Df()};
        if (!shadingNormal(normal)) {
            return null;
        }

        Vector3Df[] localTexCoord = {new Vector3Df()};
        int localFlags = 0;
        boolean ok = true;
        if (normal[0] != null) {
            localFlags |= ShadingContextFlag.SHCTX_NORMAL;
        }
        else {
            ok = false;
        }
        if (getTexCoord(localTexCoord)) {
            localFlags |= ShadingContextFlag.SHCTX_TEXTURE_COORDINATE;
        }
        else {
            localTexCoord[0] = new Vector3Df(0.0f, 0.0f, 0.0f);
        }
        if (!ok) {
            normal[0] = new Vector3Df(0.0f, 0.0f, 1.0f);
        }
        return new ShadingContext(
            getPoint(),
            getGeometricNormal(),
            normal[0],
            localTexCoord[0],
            getUv(),
            getShadingFrame(),
            getMaterial(),
            localFlags);
    }
}
