package vsdk.toolkit.material;

import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

/**
Immutable shading input data used by material evaluation.
*/
public final class ShadingContext {
    private final Vector3D point;
    private final Vector3D geometricNormal;
    private final Vector3D shadingNormal;
    private final Vector3D texCoord;
    private final Vector2Dd uv;
    private final CoordinateSystem shadingFrame;
    private final Material material;
    private final int flags;

    public ShadingContext(
        Vector3D inPoint,
        Vector3D inGeometricNormal,
        Vector3D inShadingNormal,
        Vector3D inTexCoord,
        Vector2Dd inUv,
        CoordinateSystem inShadingFrame,
        Material inMaterial,
        int inFlags) {
        point = new Vector3D(inPoint.x, inPoint.y, inPoint.z);
        geometricNormal = new Vector3D(inGeometricNormal.x, inGeometricNormal.y, inGeometricNormal.z);
        shadingNormal = new Vector3D(inShadingNormal.x, inShadingNormal.y, inShadingNormal.z);
        texCoord = new Vector3D(inTexCoord.x, inTexCoord.y, inTexCoord.z);
        uv = new Vector2Dd();
        uv.u = inUv.u;
        uv.v = inUv.v;
        shadingFrame = new CoordinateSystem();
        shadingFrame.setX(inShadingFrame.getX());
        shadingFrame.setY(inShadingFrame.getY());
        shadingFrame.setZ(inShadingFrame.getZ());
        material = inMaterial;
        flags = inFlags;
    }

    public Vector3D getPoint() { return point; }
    public Vector3D getGeometricNormal() { return geometricNormal; }
    public Vector3D getShadingNormal() { return shadingNormal; }
    public Vector3D getTexCoord() { return texCoord; }
    public Vector2Dd getUv() { return uv; }
    public CoordinateSystem getShadingFrame() { return shadingFrame; }
    public Material getMaterial() { return material; }
    public int getFlags() { return flags; }
    public boolean hasFlag(int mask) { return (flags & mask) == mask; }
}
