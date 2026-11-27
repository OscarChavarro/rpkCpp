package vsdk.toolkit.environment.material;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

/**
What the directional functions of PhongMaterial read about the surface point
being shaded: the shading normal and the texture coordinate, each with a flag
telling whether the caller could determine it. The caller finds the point,
its normal and its texture coordinate; the material only reads them.

Immutable value.
*/
public final class SurfaceShadingPoint {
    private final Vector3Dd shadingNormal;
    private final Vector3Dd textureCoordinate;
    private final boolean shadingNormalKnown;
    private final boolean textureCoordinateKnown;

    public SurfaceShadingPoint(
        Vector3Dd inShadingNormal,
        boolean inShadingNormalKnown,
        Vector3Dd inTextureCoordinate,
        boolean inTextureCoordinateKnown)
    {
        shadingNormal = new Vector3Dd(inShadingNormal);
        textureCoordinate = new Vector3Dd(inTextureCoordinate);
        shadingNormalKnown = inShadingNormalKnown;
        textureCoordinateKnown = inTextureCoordinateKnown;
    }

    /** Unit normal leaving the surface on the side the scattering is evaluated from */
    public Vector3Dd getShadingNormal() { return shadingNormal; }
    public boolean hasShadingNormal() { return shadingNormalKnown; }

    /** Texture coordinate (x and y used) */
    public Vector3Dd getTextureCoordinate() { return textureCoordinate; }
    public boolean hasTextureCoordinate() { return textureCoordinateKnown; }
}
