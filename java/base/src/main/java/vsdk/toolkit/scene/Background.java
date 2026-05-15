package vsdk.toolkit.scene;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.environment.geometry.elements.Patch;

public class Background {
    public Patch bkgPatch; // Virtual patch for background

    public Background() {
        bkgPatch = null;
    }

    /*
    Evaluate background radiance coming in from direction (direction
    positions towards the background). If probabilityDensityFunction is non-null, also fills
    in the probability of sampling this direction with sample()
    */
    public ColorRgb radiance(
        Vector3D position,
        Vector3D direction,
        float[] probabilityDensityFunction) {
        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = 0.0f;
        }
        ColorRgb black = new ColorRgb();
        black.setMonochrome(0.0f);
        return black;
    }

    /*
    Samples a direction to the background, taking into account the
    radiance coming in from the background. The returned direction
    is unique for given xi1, xi2 (in the range [0,1), including 0 but
    excluding 1). Directions on a full sphere may be returned. If a
    direction is inappropriate, a new direction (with new numbers xi1, xi2)
    needs to be sampled. If value or pdf is non-null, the radiance coming
    in from the sampled direction or the probability of sampling the
    direction are computed on the fly.
    */
    public Vector3D sample(
        Vector3D position,
        float xi1,
        float xi2,
        ColorRgb radianceValue,
        float[] probabilityDensityFunction) {
        if (radianceValue != null) {
            radianceValue.setMonochrome(0.0f);
        }
        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = 0.0f;
        }
        return new Vector3D();
    }

    /*
    Computes total power emitted by the background (= integral over
    the full sphere of the background radiance.
    */
    public ColorRgb power(Vector3D position) {
        ColorRgb black = new ColorRgb();
        black.setMonochrome(0.0f);
        return black;
    }

    public static ColorRgb backgroundRadiance(
        Background bkg,
        Vector3D position,
        Vector3D direction,
        float[] probabilityDensityFunction) {
        if (bkg == null) {
            ColorRgb black = new ColorRgb();
            black.setMonochrome(0.0f);
            return black;
        }
        else {
            return bkg.radiance(position, direction, probabilityDensityFunction);
        }
    }

    public static ColorRgb backgroundPower(Background bkg, Vector3D position) {
        if (bkg == null) {
            ColorRgb black = new ColorRgb();
            black.setMonochrome(0.0f);
            return black;
        }
        else {
            return bkg.power(position);
        }
    }
}
