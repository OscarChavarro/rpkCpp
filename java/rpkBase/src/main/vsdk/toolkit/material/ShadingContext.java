package vsdk.toolkit.material;

import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.material.SurfaceShadingPoint;

/**
Immutable shading input data used by material evaluation.
The material views convert it to VITRAL's SurfaceShadingPoint (toShadingPoint).
*/
public final class ShadingContext {
    private final Vector3Dd point;
    private final Vector3Dd geometricNormal;
    private final Vector3Dd shadingNormal;
    private final Vector3Dd texCoord;
    private final Vector2Dd uv;
    private final CoordinateSystem shadingFrame;
    private final RpkMaterial material;
    private final int flags;

    public ShadingContext(
        Vector3Dd inPoint,
        Vector3Dd inGeometricNormal,
        Vector3Dd inShadingNormal,
        Vector3Dd inTexCoord,
        Vector2Dd inUv,
        CoordinateSystem inShadingFrame,
        RpkMaterial inMaterial,
        int inFlags) {
        point = new Vector3Dd(inPoint.x(), inPoint.y(), inPoint.z());
        geometricNormal = new Vector3Dd(inGeometricNormal.x(), inGeometricNormal.y(), inGeometricNormal.z());
        shadingNormal = new Vector3Dd(inShadingNormal.x(), inShadingNormal.y(), inShadingNormal.z());
        texCoord = new Vector3Dd(inTexCoord.x(), inTexCoord.y(), inTexCoord.z());
        uv = new Vector2Dd();
        uv.x = inUv.x;
        uv.y = inUv.y;
        shadingFrame = new CoordinateSystem();
        shadingFrame.setX(inShadingFrame.getX());
        shadingFrame.setY(inShadingFrame.getY());
        shadingFrame.setZ(inShadingFrame.getZ());
        material = inMaterial;
        flags = inFlags;
    }

    public Vector3Dd getPoint() { return point; }
    public Vector3Dd getGeometricNormal() { return geometricNormal; }
    public Vector3Dd getShadingNormal() { return shadingNormal; }
    public Vector3Dd getTexCoord() { return texCoord; }
    public Vector2Dd getUv() { return uv; }
    public CoordinateSystem getShadingFrame() { return shadingFrame; }
    public RpkMaterial getMaterial() { return material; }
    public int getFlags() { return flags; }
    public boolean hasFlag(int mask) { return (flags & mask) == mask; }

    /** What the VITRAL material functions read: the shading normal and the texture coordinate */
    public SurfaceShadingPoint toShadingPoint() {
        return new SurfaceShadingPoint(
            new Vector3Dd(shadingNormal.x(), shadingNormal.y(), shadingNormal.z()), hasFlag(ShadingContextFlag.SHCTX_NORMAL),
            new Vector3Dd(texCoord.x(), texCoord.y(), texCoord.z()), hasFlag(ShadingContextFlag.SHCTX_TEXTURE_COORDINATE));
    }
}
