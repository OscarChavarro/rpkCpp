package vsdk.toolkit.material;

import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.environment.material.SurfaceShadingPoint;

/**
Immutable shading input data used by material evaluation.
The material views convert it to VITRAL's SurfaceShadingPoint (toShadingPoint).
*/
public final class ShadingContext {
    private final Vector3D point;
    private final Vector3D geometricNormal;
    private final Vector3D shadingNormal;
    private final Vector3D texCoord;
    private final Vector2Dd uv;
    private final CoordinateSystem shadingFrame;
    private final RpkMaterial material;
    private final int flags;

    public ShadingContext(
        Vector3D inPoint,
        Vector3D inGeometricNormal,
        Vector3D inShadingNormal,
        Vector3D inTexCoord,
        Vector2Dd inUv,
        CoordinateSystem inShadingFrame,
        RpkMaterial inMaterial,
        int inFlags) {
        point = new Vector3D(inPoint.x, inPoint.y, inPoint.z);
        geometricNormal = new Vector3D(inGeometricNormal.x, inGeometricNormal.y, inGeometricNormal.z);
        shadingNormal = new Vector3D(inShadingNormal.x, inShadingNormal.y, inShadingNormal.z);
        texCoord = new Vector3D(inTexCoord.x, inTexCoord.y, inTexCoord.z);
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

    public Vector3D getPoint() { return point; }
    public Vector3D getGeometricNormal() { return geometricNormal; }
    public Vector3D getShadingNormal() { return shadingNormal; }
    public Vector3D getTexCoord() { return texCoord; }
    public Vector2Dd getUv() { return uv; }
    public CoordinateSystem getShadingFrame() { return shadingFrame; }
    public RpkMaterial getMaterial() { return material; }
    public int getFlags() { return flags; }
    public boolean hasFlag(int mask) { return (flags & mask) == mask; }

    /** What the VITRAL material functions read: the shading normal and the texture coordinate */
    public SurfaceShadingPoint toShadingPoint() {
        return new SurfaceShadingPoint(
            new Vector3Df(shadingNormal.x, shadingNormal.y, shadingNormal.z), hasFlag(ShadingContextFlag.SHCTX_NORMAL),
            new Vector3Df(texCoord.x, texCoord.y, texCoord.z), hasFlag(ShadingContextFlag.SHCTX_TEXTURE_COORDINATE));
    }
}
