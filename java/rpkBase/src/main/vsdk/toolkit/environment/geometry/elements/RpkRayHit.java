package vsdk.toolkit.environment.geometry.elements;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
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
public class RpkRayHit {
    private Vector3Dd point; // Intersection point
    private Patch patch; // Patch that was hit
    private Vector3Dd texCoord; // Texture coordinate
    private Vector3Dd geometricNormal;
    private RpkMaterial material; // RpkMaterial of hit surface
    // Shading frame (Z = shading normal: hit->shadingFrame.getZAxis() == hit->normal)
    private CoordinateSystem shadingFrame;
    private Vector2Dd uv; // Bi-linear / barycentric parameters of hit
    // Flags indicating which of the above fields have been filled in
    private int flags;

    public RpkRayHit() {
        point = new Vector3Dd();
        patch = null;
        texCoord = new Vector3Dd();
        geometricNormal = new Vector3Dd();
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
    private boolean pointShadingFrame(Vector3Dd[] inX, Vector3Dd[] inY, Vector3Dd[] inZ) {
        boolean success = false;

        if (!hitInitialised()) {
            Logger.reportMessage(this, Logger.WARNING, "pointShadingFrame", "uninitialised hit structure");
            return false;
        }

        ShadingContext context = new ShadingContext(
            point,
            geometricNormal,
            shadingFrame.getZAxis(),
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
            Vector3Dd[] x = inX != null ? inX : new Vector3Dd[1];
            Vector3Dd[] y = inY != null ? inY : new Vector3Dd[1];
            Vector3Dd[] z = inZ != null ? inZ : new Vector3Dd[1];
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
    public boolean init(Patch inPatch, Vector3Dd inPoint, Vector3Dd inGeometryNormal, RpkMaterial inMaterial) {
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

        Vector3Dd localNormal = new Vector3Dd(0.0, 0.0, 0.0);
        texCoord = localNormal;
        shadingFrame.setXAxis(localNormal);
        shadingFrame.setYAxis(localNormal);
        shadingFrame.setZAxis(localNormal);
        uv.x = 0.0;
        uv.y = 0.0;

        return hitInitialised();
    }

    /**
    Fills in/computes texture coordinates of hit point
    (outTexCoord is a one-element output holder).
    */
    public boolean getTexCoord(Vector3Dd[] outTexCoord) {
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
    public boolean shadingNormal(Vector3Dd[] inNormal) {
        if (((flags & RayHitFlag.SHADING_FRAME) != 0) || ((flags & RayHitFlag.NORMAL) != 0)) {
            inNormal[0] = shadingFrame.getZAxis();
            return true;
        }

        Vector3Dd[] localNormal = {shadingFrame.getZAxis()};
        if (!pointShadingFrame(null, null, localNormal)) {
            return false;
        }

        flags |= RayHitFlag.NORMAL;
        shadingFrame.setZAxis(localNormal[0]);
        inNormal[0] = shadingFrame.getZAxis();
        return true;
    }

    /**
    Fills in shading frame: Z is the shading normal.
    */
    public boolean setShadingFrame(CoordinateSystem frame) {
        if ((flags & RayHitFlag.SHADING_FRAME) != 0) {
            frame.setXAxis(shadingFrame.getXAxis());
            frame.setYAxis(shadingFrame.getYAxis());
            frame.setZAxis(shadingFrame.getZAxis());
            return true;
        }

        Vector3Dd[] shadingX = {shadingFrame.getXAxis()};
        Vector3Dd[] shadingY = {shadingFrame.getYAxis()};
        Vector3Dd[] shadingZ = {shadingFrame.getZAxis()};

        if (!pointShadingFrame(shadingX, shadingY, shadingZ)) {
            return false;
        }

        shadingFrame.setXAxis(shadingX[0]);
        shadingFrame.setYAxis(shadingY[0]);
        shadingFrame.setZAxis(shadingZ[0]);
        flags |= RayHitFlag.SHADING_FRAME | RayHitFlag.NORMAL;

        frame.setXAxis(shadingFrame.getXAxis());
        frame.setYAxis(shadingFrame.getYAxis());
        frame.setZAxis(shadingFrame.getZAxis());
        return true;
    }

    public Patch getPatch() {
        return patch;
    }

    public void setPatch(Patch inPatch) {
        patch = inPatch;
    }

    public Vector3Dd getPoint() {
        return point;
    }

    public void setPoint(Vector3Dd position) {
        point = position;
    }

    public void setGeometricNormal(Vector3Dd inNormal) {
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

    public Vector3Dd getNormal() {
        return shadingFrame.getZAxis();
    }

    public void setNormal(Vector3Dd n) {
        shadingFrame.setZAxis(n);
    }

    public CoordinateSystem getShadingFrame() {
        return shadingFrame;
    }

    public RpkMaterial getMaterial() {
        return material;
    }

    public Vector3Dd getGeometricNormal() {
        return geometricNormal;
    }

    public void setShadingFrame(Vector3Dd inX, Vector3Dd inY, Vector3Dd inZ) {
        shadingFrame.setXAxis(inX);
        shadingFrame.setYAxis(inY);
        shadingFrame.setZAxis(inZ);
    }

    /**
    Plain member-wise copy, equivalent to the C++ port's structure assignment
    (*hitStore = hit). Fields not marked as filled in by the flags are copied
    as-is and remain meaningless, just as in the C++ version; readers must
    check the flags (or use the getTexCoord/shadingNormal accessors, which
    compute missing data on demand).
    */
    public void copyFrom(RpkRayHit other) {
        if (other == null) {
            return;
        }

        point = other.point;
        patch = other.patch;
        texCoord = other.texCoord;
        geometricNormal = other.geometricNormal;
        material = other.material;
        shadingFrame.setXAxis(other.shadingFrame.getXAxis());
        shadingFrame.setYAxis(other.shadingFrame.getYAxis());
        shadingFrame.setZAxis(other.shadingFrame.getZAxis());
        uv.x = other.uv.x;
        uv.y = other.uv.y;
        flags = other.flags;
    }

    public ShadingContext shadingContext() {
        Vector3Dd[] normal = {new Vector3Dd()};
        if (!shadingNormal(normal)) {
            return null;
        }

        Vector3Dd[] localTexCoord = {new Vector3Dd()};
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
            localTexCoord[0] = new Vector3Dd(0.0, 0.0, 0.0);
        }
        if (!ok) {
            normal[0] = new Vector3Dd(0.0, 0.0, 1.0);
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
