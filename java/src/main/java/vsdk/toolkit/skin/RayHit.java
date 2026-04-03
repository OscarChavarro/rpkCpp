package vsdk.toolkit.skin;

import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.Material;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.material.PhongEmittanceDistributionFunction;
import vsdk.toolkit.material.RayHitFlag;

public class RayHit {
    private Vector3D point;
    private Patch patch;
    private Vector3D texCoord;
    private Vector3D geometricNormal;
    private Material material;
    private CoordinateSystem shadingFrame;
    private Vector2Dd uv;
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

    private boolean hitInitialised() {
        return (((flags & RayHitFlag.PATCH) != 0) || ((flags & RayHitFlag.GEOMETRY) != 0))
            && ((flags & RayHitFlag.POINT) != 0)
            && ((flags & RayHitFlag.GEOMETRIC_NORMAL) != 0)
            && ((flags & RayHitFlag.MATERIAL) != 0)
            && ((flags & RayHitFlag.DISTANCE) != 0);
    }

    private boolean pointShadingFrame(Vector3D inX, Vector3D inY, Vector3D inZ) {
        boolean success = false;

        if (!hitInitialised()) {
            Error.warning("pointShadingFrame", "uninitialised hit structure");
            return false;
        }

        if (material != null && material.getBsdf() != null) {
            success = PhongBidirectionalScatteringDistributionFunction.bsdfShadingFrame(this, inX, inY, inZ);
        }

        if (!success && material != null && material.getEdf() != null) {
            success = PhongEmittanceDistributionFunction.edfShadingFrame(this, inX, inY, inZ);
        }

        if (!success && computeUv(uv) && patch != null) {
            Vector3D x = inX != null ? inX : new Vector3D();
            Vector3D y = inY != null ? inY : new Vector3D();
            Vector3D z = inZ != null ? inZ : new Vector3D();
            patch.interpolatedFrameAtUv(uv.u, uv.v, x, y, z);

            if (inZ != null) {
                inZ.copy(z);
            }
            if (inX != null) {
                inX.copy(x);
            }
            if (inY != null) {
                inY.copy(y);
            }
            success = true;
        }

        return success;
    }

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
        point = new Vector3D(position.x, position.y, position.z);
    }

    public void setGeometricNormal(Vector3D inNormal) {
        geometricNormal = new Vector3D(inNormal.x, inNormal.y, inNormal.z);
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
}
