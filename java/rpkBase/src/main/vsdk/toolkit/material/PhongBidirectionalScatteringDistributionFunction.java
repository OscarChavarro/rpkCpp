package vsdk.toolkit.material;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.material.PhongMaterial;

/**
RenderPark's view of the scattering of a material (its split BSDF: the
reflection, the transmission and the texture): the evaluation, sampling and
density functions the raytracers call, with RenderPark's argument types.
Every function converts its arguments (Vector3Dd to Vector3Dd, the shading
context to a SurfaceShadingPoint, the media to their VITRAL materials) and
answers through the VITRAL PhongMaterial; the scattering model itself lives
in VITRAL (plan step 2b). The material and the media are the ones of the
RpkMaterials that own these views, so a view identifies its material.

Read-only after construction; does not own the PhongMaterial.
*/
public class PhongBidirectionalScatteringDistributionFunction {
    private final PhongMaterial material;

    private static PhongMaterial mediumOf(PhongBidirectionalScatteringDistributionFunction bsdf) {
        return bsdf != null ? bsdf.getMaterial() : null;
    }

    public PhongBidirectionalScatteringDistributionFunction(PhongMaterial inMaterial) {
        material = inMaterial;
    }

    public PhongMaterial getMaterial() {
        return material;
    }

    /**
    No material defines its own shading frame: callers build a default one
    */
    public static boolean bsdfShadingFrame(ShadingContext context, Vector3Dd X, Vector3Dd Y, Vector3Dd Z) {
        return false;
    }

    /**
    Scattered power (diffuse/glossy/specular reflectance and/or transmittance) according to flags
    */
    public ColorRgb splitBsdfScatteredPower(ShadingContext context, int flags) {
        final Vector3Dd textureCoordinate = context.getTexCoord();
        return material.scatteredPower(
            flags, context.hasFlag(ShadingContextFlag.SHCTX_TEXTURE_COORDINATE) ? textureCoordinate : null);
    }

    public boolean splitBsdfIsTextured() {
        return material.isTextured();
    }

    public void indexOfRefraction(RefractionIndex index) {
        index.set(material.getRefractionIndex(), material.getRefractionIndexImaginary());
    }

    public Vector3Dd sample(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3Dd in,
        int doRussianRoulette,
        int flags,
        double x1,
        double x2,
        double[] probabilityDensityFunction)
    {
        return material.scatteringSample(
            context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf), in,
            doRussianRoulette != 0, flags, x1, x2, probabilityDensityFunction);
    }

    public ColorRgb evaluate(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3Dd in,
        Vector3Dd out,
        int flags)
    {
        return material.scatteringEvaluate(
            context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf),
            in, out, flags);
    }

    public void evaluateProbabilityDensityFunction(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3Dd in,
        Vector3Dd out,
        int flags,
        double[] probabilityDensityFunction,
        double[] probabilityDensityFunctionRR)
    {
        material.scatteringDensity(
            context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf),
            in, out, flags,
            probabilityDensityFunction, probabilityDensityFunctionRR);
    }

    public ColorRgb bsdfEvalComponents(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3Dd in,
        Vector3Dd out,
        int flags,
        ColorRgb[] colArray)
    {
        return material.scatteringEvaluateComponents(
            context.toShadingPoint(), mediumOf(inBsdf), mediumOf(outBsdf),
            in, out, flags, colArray);
    }
}
