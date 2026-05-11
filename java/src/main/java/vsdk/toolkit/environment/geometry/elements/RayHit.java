package vsdk.toolkit.environment.geometry.elements;

import vsdk.toolkit.skin.*;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.Material;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.material.PhongEmittanceDistributionFunction;
import vsdk.toolkit.material.ShadingContext;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;

/**
Hit record structure, returned by ray-object intersection routines and
used as a parameter for BSDF/EDF queries.
*/
public class RayHit {
    private Vector3D point; // Intersection point
    private Patch patch; // Patch that was hit
    private Vector3D texCoord; // Texture coordinate
    private Vector3D geometricNormal;
    private Material material; // Material of hit surface
    // Shading frame (Z = shading normal: hit->shadingFrame.getZ() == hit->normal)
    private CoordinateSystem shadingFrame;
    private Vector2Dd uv; // Bi-linear / barycentric parameters of hit
    // Flags indicating which of the above fields have been filled in
    private int flags;

    public RayHit() {
        point = new Vector3D();
        patch = null;
        texCoord = new Vector3D();
        geometricNormal = new Vector3D();
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
            inUv.u = uv.u;
            inUv.v = uv.v;
            return true;
        }

        if (((flags & RayHitFlag.PATCH) != 0) && ((flags & RayHitFlag.POINT) != 0) && patch != null) {
            double[] u = new double[] {0.0};
            double[] v = new double[] {0.0};
            patch.uv(point, u, v);
            uv.u = u[0];
            uv.v = v[0];
            inUv.u = uv.u;
            inUv.v = uv.v;
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
    private boolean pointShadingFrame(Vector3D inX, Vector3D inY, Vector3D inZ) {
        boolean success = false;

        if (!hitInitialised()) {
            Logger.warning("pointShadingFrame", "uninitialised hit structure");
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
            success = PhongBidirectionalScatteringDistributionFunction.bsdfShadingFrame(context, inX, inY, inZ);
        }

        if (!success && material != null && material.getEdf() != null && context != null) {
            success = PhongEmittanceDistributionFunction.edfShadingFrame(context, inX, inY, inZ);
        }

        if (!success && computeUv(uv) && patch != null) {
            // Make default shading frame
            Vector3D x = inX != null ? inX : new Vector3D();
            Vector3D y = inY != null ? inY : new Vector3D();
            Vector3D z = inZ != null ? inZ : new Vector3D();
            patch.interpolatedFrameAtUv(uv.u, uv.v, x, y, z);

            if (inX != null) {
                inX.copy(x);
            }
            if (inY != null) {
                inY.copy(y);
            }
            if (inZ != null) {
                inZ.copy(z);
            }
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
    public boolean init(Patch inPatch, Vector3D inPoint, Vector3D inGeometryNormal, Material inMaterial) {
        flags = 0;
        patch = inPatch;
        if (inPatch != null) {
            flags |= RayHitFlag.PATCH;
        }

        if (inPoint != null) {
            point.copy(inPoint);
            flags |= RayHitFlag.POINT;
        }

        if (inGeometryNormal != null) {
            geometricNormal.copy(inGeometryNormal);
            flags |= RayHitFlag.GEOMETRIC_NORMAL;
        }

        material = inMaterial;
        flags |= RayHitFlag.MATERIAL;
        flags |= RayHitFlag.DISTANCE;

        Vector3D localNormal = new Vector3D();
        localNormal.set(0.0f, 0.0f, 0.0f);
        texCoord.copy(localNormal);
        shadingFrame.setX(localNormal);
        shadingFrame.setY(localNormal);
        shadingFrame.setZ(localNormal);
        uv.u = 0.0;
        uv.v = 0.0;

        return hitInitialised();
    }

    /**
    Fills in/computes texture coordinates of hit point.
    */
    public boolean getTexCoord(Vector3D outTexCoord) {
        if ((flags & RayHitFlag.TEXTURE_COORDINATE) != 0) {
            outTexCoord.copy(texCoord);
            return true;
        }

        if (!computeUv(uv)) {
            return false;
        }

        if ((flags & RayHitFlag.PATCH) != 0 && patch != null) {
            texCoord = patch.textureCoordAtUv(uv.u, uv.v);
            outTexCoord.copy(texCoord);
            flags |= RayHitFlag.TEXTURE_COORDINATE;
            return true;
        }

        return false;
    }

    /**
    Fills in shading normal (Z axis of shading frame) only, avoiding computation
    of shading X and Y axis if possible.
    */
    public boolean shadingNormal(Vector3D inNormal) {
        if (((flags & RayHitFlag.SHADING_FRAME) != 0) || ((flags & RayHitFlag.NORMAL) != 0)) {
            inNormal.copy(shadingFrame.getZ());
            return true;
        }

        Vector3D localNormal = shadingFrame.getZ();
        if (!pointShadingFrame(null, null, localNormal)) {
            return false;
        }

        flags |= RayHitFlag.NORMAL;
        shadingFrame.setZ(localNormal);
        inNormal.copy(shadingFrame.getZ());
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

        Vector3D shadingX = shadingFrame.getX();
        Vector3D shadingY = shadingFrame.getY();
        Vector3D shadingZ = shadingFrame.getZ();

        if (!pointShadingFrame(shadingX, shadingY, shadingZ)) {
            return false;
        }

        shadingFrame.setX(shadingX);
        shadingFrame.setY(shadingY);
        shadingFrame.setZ(shadingZ);
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

    public Vector3D getPoint() {
        return point;
    }

    public void setPoint(Vector3D position) {
        point.copy(position);
    }

    public void setGeometricNormal(Vector3D inNormal) {
        geometricNormal.copy(inNormal);
    }

    public void setMaterial(Material inMaterial) {
        material = inMaterial;
    }

    public Vector2Dd getUv() {
        return uv;
    }

    public void setUv(Vector2Dd inUv) {
        uv.u = inUv.u;
        uv.v = inUv.v;
    }

    public void setUv(double inU, double inV) {
        uv.u = inU;
        uv.v = inV;
    }

    public int getFlags() {
        return flags;
    }

    public void setFlags(int inFlags) {
        flags = inFlags;
    }

    public Vector3D getNormal() {
        return shadingFrame.getZ();
    }

    public void setNormal(Vector3D n) {
        shadingFrame.setZ(n);
    }

    public CoordinateSystem getShadingFrame() {
        return shadingFrame;
    }

    public Material getMaterial() {
        return material;
    }

    public Vector3D getGeometricNormal() {
        return geometricNormal;
    }

    public void setShadingFrame(Vector3D inX, Vector3D inY, Vector3D inZ) {
        shadingFrame.setX(inX);
        shadingFrame.setY(inY);
        shadingFrame.setZ(inZ);
    }

    public void copyFrom(RayHit other) {
        if (other == null) {
            return;
        }

        setPoint(other.getPoint());
        setPatch(other.getPatch());
        setGeometricNormal(other.getGeometricNormal());
        setMaterial(other.getMaterial());
        setUv(other.getUv());
        setFlags(other.getFlags());

        CoordinateSystem frame = other.getShadingFrame();
        setShadingFrame(frame.getX(), frame.getY(), frame.getZ());
        Vector3D localTex = new Vector3D();
        if (other.getTexCoord(localTex)) {
            texCoord = localTex;
        }
    }

    public ShadingContext shadingContext() {
        Vector3D normal = new Vector3D();
        if (!shadingNormal(normal)) {
            return null;
        }

        Vector3D localTexCoord = new Vector3D();
        int localFlags = RayHitFlag.NORMAL;
        if (getTexCoord(localTexCoord)) {
            localFlags |= RayHitFlag.TEXTURE_COORDINATE;
        }
        else {
            localTexCoord.set(0.0f, 0.0f, 0.0f);
        }

        return new ShadingContext(
            getPoint(),
            getGeometricNormal(),
            normal,
            localTexCoord,
            getUv(),
            getShadingFrame(),
            getMaterial(),
            localFlags);
    }
}
