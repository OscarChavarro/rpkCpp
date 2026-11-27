package vsdk.toolkit.material;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.material.PhongMaterial;
import vsdk.toolkit.environment.material.SurfaceShadingPoint;

/**
RenderPark's view of the self-emitted radiance distribution of a light
source material, with RenderPark's argument types: every function converts
its arguments and answers through the VITRAL PhongMaterial, whose emission
lobe is the model (plan step 2b).

Read-only after construction; does not own the PhongMaterial.
*/
public class PhongEmittanceDistributionFunction {
    private final PhongMaterial material;

    public PhongEmittanceDistributionFunction(PhongMaterial inMaterial) {
        material = inMaterial;
    }

    public static boolean edfIsTextured() {
        return false;
    }

    /**
    No emitter defines its own shading frame: callers build a default one
    */
    public static boolean edfShadingFrame(ShadingContext context, Vector3Dd X, Vector3Dd Y, Vector3Dd Z) {
        return false;
    }

    /** Emittance (self-emitted radiant exitance) [W / m^2] of the components in flags */
    public ColorRgb phongEmittance(ShadingContext context, int flags) {
        return material.emittance(flags);
    }

    /**
    Exitant radiance [W / m^2 sr] into the direction out; when
    probabilityDensityFunction is not null, the density of out under
    phongEdfSample goes there
    */
    public ColorRgb phongEdfEval(ShadingContext context, Vector3Dd out, int flags, double[] probabilityDensityFunction) {
        if ( context == null ) {
            return material.emissionEvaluate(null, out, flags, probabilityDensityFunction);
        }
        final SurfaceShadingPoint point = context.toShadingPoint();
        return material.emissionEvaluate(point, out, flags, probabilityDensityFunction);
    }

    /**
    Samples an emitted direction; the radiance along it (into
    selfEmittedRadiance, a one-element output holder) and its density go to
    the non-null arguments
    */
    public Vector3Dd phongEdfSample(
        ShadingContext context,
        int flags,
        double xi1,
        double xi2,
        ColorRgb[] selfEmittedRadiance,
        double[] probabilityDensityFunction)
    {
        final ColorRgb[] radiance = selfEmittedRadiance != null ? new ColorRgb[1] : null;
        final Vector3Dd direction;
        if ( context == null ) {
            direction = material.emissionSample(
                null, flags, xi1, xi2, radiance, probabilityDensityFunction);
        } else {
            final SurfaceShadingPoint point = context.toShadingPoint();
            direction = material.emissionSample(
                point, flags, xi1, xi2, radiance, probabilityDensityFunction);
        }
        if ( selfEmittedRadiance != null ) {
            selfEmittedRadiance[0] = radiance[0];
        }
        return direction;
    }
}
