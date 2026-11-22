package vsdk.toolkit.io.mgf;

import java.util.ArrayList;
import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.dataStructures.LookUpEntity;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.io.context.ColorContext;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.MaterialContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.TokenValidationContext;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.environment.material.PhongLobe;

public class MgfMaterialEntitySupport {
    private static final int NUMBER_OF_SAMPLES = 3;

    /**
    Looks up a material with given name in the given material list. Returns
a pointer to the material if found, or nullptr if not found
    */
    private static RpkMaterial materialLookup(String name, ParseRuntimeContext context) {
        for (int i = 0; context.materials != null && i < context.materials.size(); i++) {
            RpkMaterial m = context.materials.get(i);
            if (m != null && m.getName() != null && m.getName().equals(name)) {
                return m;
            }
        }
        return null;
    }

    private static ColorRgb addColor(ColorRgb a, ColorRgb b) {
        return new ColorRgb(a.getR() + b.getR(), a.getG() + b.getG(), a.getB() + b.getB());
    }

    private static ColorRgb scaleColor(ColorRgb c, double s) {
        return new ColorRgb(c.getR() * s, c.getG() * s, c.getB() * s);
    }

    private static boolean isBlack(ColorRgb c) {
        return Math.abs(c.getR()) < Numeric.EPSILON
            && Math.abs(c.getG()) < Numeric.EPSILON
            && Math.abs(c.getB()) < Numeric.EPSILON;
    }

    private static ColorRgb gray(ColorRgb c) {
        return new ColorRgb(
            Cie.spectrumGray(c.getR(), c.getG(), c.getB()),
            Cie.spectrumGray(c.getR(), c.getG(), c.getB()),
            Cie.spectrumGray(c.getR(), c.getG(), c.getB()));
    }

    /**
    Translates mgf color into out color representation
    */
    private static ColorRgb mgfGetColor(ColorContext cin, double intensity, ParseRuntimeContext context) {
        double[] xyz = new double[3];
        double[] rgb = new double[3];

        cin.fixColorRepresentation(ColorContext.COLOR_XY_IS_SET_FLAG);
        if ( cin.cy > Numeric.EPSILON ) {
            xyz[0] = cin.cx / cin.cy * intensity;
            xyz[1] = 1.0 * intensity;
            xyz[2] = (1.0 - cin.cx - cin.cy) / cin.cy * intensity;
        } else {
            MgfEntityControl.doWarning("invalid color specification (Y<=0) ... setting to black", context);
            xyz[0] = 0.0;
            xyz[1] = 0.0;
            xyz[2] = 0.0;
        }

        if ( xyz[0] < 0.0 || xyz[1] < 0.0 || xyz[2] < 0.0 ) {
            MgfEntityControl.doWarning("invalid color specification (negative CIE XYZ components) ... clipping to zero", context);
            if ( xyz[0] < 0.0 ) {
                xyz[0] = 0.0;
            }
            if ( xyz[1] < 1.0 ) {
                xyz[1] = 0.0;
            }
            if ( xyz[2] < 2.0 ) {
                xyz[2] = 0.0;
            }
        }

        Cie.transformColorFromXYZ2RGB(xyz, rgb);
        if ( Cie.clipGamut(rgb) ) {
            MgfEntityControl.doWarning("color desaturated during gamut clipping", context);
        }
        return new ColorRgb(rgb[0], rgb[1], rgb[2]);
    }

    private static void specSamples(ColorRgb col, double[] rgb) {
        rgb[0] = col.getR();
        rgb[1] = col.getG();
        rgb[2] = col.getB();
    }

    private static double colorMax(ColorRgb col) {
        // We should check every wavelength in the visible spectrum, but
        // as a first approximation, only the three RGB primary colors
        // are checked
        double[] samples = new double[NUMBER_OF_SAMPLES];

        MgfMaterialEntitySupport.specSamples(col, samples);

        double mx = -Numeric.HUGE_FLOAT_VALUE;
        for ( int i = 0; i < NUMBER_OF_SAMPLES; i++ ) {
            if ( samples[i] > mx ) {
                mx = samples[i];
            }
        }

        return mx;
    }

    /**
    This routine checks whether the mgf material being used has changed. If it
    changed, this routine converts to our representation of materials and
    creates a new MATERIAL, which is added to the session material library.
    The routine returns true if the material being used has changed
    */
    public static boolean mgfGetCurrentMaterial(RpkMaterial[] material, boolean allSurfacesSided, ParseRuntimeContext context) {
        MaterialContext currentMaterialContext = context.materialRepository.currentMaterialContext;
        String materialName = context.currentMaterialName;
        if ( materialName == null || materialName.isEmpty() ) {
            // This might cause strcmp to crash!
            materialName = "unnamed";
        }

        // Is it another material than the one used for the previous face ?? If not, the
        // material remains the same
        if ( material[0] != null && materialName.equals(material[0].getName()) && currentMaterialContext.clock == 0 ) {
            return false;
        }

        RpkMaterial storedMaterial = MgfMaterialEntitySupport.materialLookup(materialName, context);
        if ( storedMaterial != null && currentMaterialContext.clock == 0 ) {
            material[0] = storedMaterial;
            return true;
        }

        // New material, or a material that changed. Convert intensities and chromaticities
        // to our color model
        ColorRgb Ed = MgfMaterialEntitySupport.mgfGetColor(currentMaterialContext.ed_c, currentMaterialContext.ed, context);
        ColorRgb Rd = MgfMaterialEntitySupport.mgfGetColor(currentMaterialContext.rd_c, currentMaterialContext.rd, context);
        ColorRgb Td = MgfMaterialEntitySupport.mgfGetColor(currentMaterialContext.td_c, currentMaterialContext.td, context);
        ColorRgb Rs = MgfMaterialEntitySupport.mgfGetColor(currentMaterialContext.rs_c, currentMaterialContext.rs, context);
        ColorRgb Ts = MgfMaterialEntitySupport.mgfGetColor(currentMaterialContext.ts_c, currentMaterialContext.ts, context);
        ColorRgb A;

        // Check/correct range of reflectances and transmittances
        A = addColor(Rd, Rs);
        double a = MgfMaterialEntitySupport.colorMax(A);
        if ( a > 1.0 - Numeric.EPSILON_FLOAT ) {
            MgfEntityControl.doWarning("invalid material specification: total reflectance shall be < 1", context);
            a = (1.0 - Numeric.EPSILON_FLOAT) / a;
            Rd = scaleColor(Rd, a);
            Rs = scaleColor(Rs, a);
        }

        A = addColor(Td, Ts);
        a = MgfMaterialEntitySupport.colorMax(A);
        if ( a > 1.0 - Numeric.EPSILON_FLOAT ) {
            MgfEntityControl.doWarning("invalid material specification: total transmittance shall be < 1", context);
            a = (1.0 - Numeric.EPSILON_FLOAT) / a;
            Td = scaleColor(Td, a);
            Ts = scaleColor(Ts, a);
        }

        // Convert lumen / m^2 to W / m^2
        Ed = scaleColor(Ed, 1.0 / Cie.WHITE_EFFICACY);

        ColorRgb Es = new ColorRgb(0.0, 0.0, 0.0);

        double Nr;
        double Nt;

        // Specular power = (0.6/roughness)^2 (see mgf docs)
        if ( currentMaterialContext.rs_a != 0.0 ) {
            Nr = 0.6 / currentMaterialContext.rs_a;
            Nr *= Nr;
        } else {
            Nr = 0.0;
        }

        if ( currentMaterialContext.ts_a != 0.0 ) {
            Nt = 0.6 / currentMaterialContext.ts_a;
            Nt *= Nt;
        } else {
            Nt = 0.0;
        }

        if ( context.monochrome ) {
            Ed = gray(Ed);
            Es = gray(Es);
            Rd = gray(Rd);
            Rs = gray(Rs);
            Td = gray(Td);
            Ts = gray(Ts);
        }

        // The lobes keep their exponents in single precision
        PhongLobe emission = null;
        if ( !isBlack(Ed) || !isBlack(Es) ) {
            final double Ne = 0.0;
            emission = new PhongLobe(Ed, Es, (float) Ne);
        }

        PhongLobe reflection = null;
        if ( !isBlack(Rd) || !isBlack(Rs) ) {
            reflection = new PhongLobe(Rd, Rs, (float) Nr);
        }

        PhongLobe transmission = null;
        float refractionIndexReal = 1.0F;
        float refractionIndexImaginary = 0.0F;
        if ( !isBlack(Td) || !isBlack(Ts) ) {
            transmission = new PhongLobe(Td, Ts, (float) Nt);
            refractionIndexReal = currentMaterialContext.nr;
            refractionIndexImaginary = currentMaterialContext.ni;
        }

        material[0] = new RpkMaterial(
            materialName,
            emission,
            true,
            reflection,
            transmission,
            null,
            refractionIndexReal,
            refractionIndexImaginary,
            allSurfacesSided || currentMaterialContext.sided);

        if ( context.materials == null ) {
            context.materials = new ArrayList<>();
            context.materialState.materials = context.materials;
        }
        context.materials.add(material[0]);

        // Reset the clock value to be aware of possible changes in future
        currentMaterialContext.clock = 0;

        return true;
    }

    public static void initMaterialContextTables(ParseRuntimeContext context) {
        context.materialRepository.reset();
        context.currentMaterialName = null;
        context.materialState.currentMaterialName = null;
    }

    /**
This routine returns true if the current material has changed
    */
    public static boolean mgfMaterialChanged(RpkMaterial material, ParseRuntimeContext context) {
        String materialName = context.currentMaterialName;
        if (materialName == null || materialName.isEmpty()) {
            materialName = "unnamed";
        }

        // Is it another material than the one used for the previous face? If not, the
        // current material context remains the same
        if (material != null && materialName.equals(material.getName()) &&
             context.materialRepository.currentMaterialContext.clock == 0) {
            return false;
        }

        return true;
    }

    /**
    Handle material entity
    */
    public static int handleMaterialEntity(int ac, String[] av, ParseRuntimeContext context) {
        int i;
        LookUpEntity<MaterialContext> lp;
        MaterialContext currentMaterialContext = context.materialRepository.currentMaterialContext;

        switch (MgfEntityControl.mgfEntity(av[0], context)) {

            case EntityTypeContext.MGF_MATERIAL:
                // Get / set material context
                if (ac > 4) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (ac == 1) {
                    // Set unnamed material context
                    context.materialRepository.unNamedMaterialContext.copy(context.materialRepository.defaultMaterialContext);
                    currentMaterialContext = context.materialRepository.unNamedMaterialContext;
                    context.materialRepository.currentMaterialContext = currentMaterialContext;
                    context.currentMaterialName = null;
                    context.materialState.currentMaterialName = null;
                    return ParseErrorContext.MGF_OK;
                }
                if (!TokenValidationContext.isName(av[1])) {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                lp = context.materialRepository.materialLookUpTable.lookUpFind(av[1]);
                // Lookup context
                if (lp == null) {
                    return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
                }
                context.currentMaterialName = lp.key;
                context.materialState.currentMaterialName = context.currentMaterialName;
                currentMaterialContext = lp.data;
                context.materialRepository.currentMaterialContext = currentMaterialContext;
                if (ac == 2) {
                    // Re-establish previous context
                    if (currentMaterialContext == null) {
                        return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
                    }
                    return ParseErrorContext.MGF_OK;
                }
                if (av[2].length() != 1 || av[2].charAt(0) != '=') {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                if (currentMaterialContext == null) {
                    // Create new material
                    lp.key = av[1];
                    lp.data = new MaterialContext();
                    context.currentMaterialName = lp.key;
                    context.materialState.currentMaterialName = context.currentMaterialName;
                    currentMaterialContext = lp.data;
                    context.materialRepository.currentMaterialContext = currentMaterialContext;
                    currentMaterialContext.clock = 0;
                }
                i = currentMaterialContext.clock;
                if (ac == 3) {
                    // Use default template
                    currentMaterialContext.copy(context.materialRepository.defaultMaterialContext);
                    currentMaterialContext.clock = i + 1;
                    return ParseErrorContext.MGF_OK;
                }
                lp = context.materialRepository.materialLookUpTable.lookUpFind(av[3]);
                // Lookup template
                if (lp == null) {
                    return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
                }
                if (lp.data == null) {
                    return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
                }
                currentMaterialContext.copy(lp.data);
                currentMaterialContext.clock = i + 1;
                return ParseErrorContext.MGF_OK;

            case EntityTypeContext.IR:
                // Set index of refraction
                if (ac != 3) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (!TokenValidationContext.isFloat(av[1]) || !TokenValidationContext.isFloat(av[2])) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                currentMaterialContext.nr = Float.parseFloat(av[1]);
                currentMaterialContext.ni = Float.parseFloat(av[2]);
                if (currentMaterialContext.nr <= Numeric.EPSILON) {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                currentMaterialContext.clock++;
                return ParseErrorContext.MGF_OK;

            case EntityTypeContext.RD:
                // Set diffuse reflectance
                if (ac != 2) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (!TokenValidationContext.isFloat(av[1])) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                currentMaterialContext.rd = Float.parseFloat(av[1]);
                if (currentMaterialContext.rd < 0.0 || currentMaterialContext.rd > 1.0) {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                currentMaterialContext.rd_c.copy(context.currentColor);
                currentMaterialContext.clock++;
                return ParseErrorContext.MGF_OK;

            case EntityTypeContext.ED:
                // Set diffuse emittance
                if (ac != 2) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (!TokenValidationContext.isFloat(av[1])) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                currentMaterialContext.ed = Float.parseFloat(av[1]);
                if (currentMaterialContext.ed < 0.0) {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                currentMaterialContext.ed_c.copy(context.currentColor);
                currentMaterialContext.clock++;
                return ParseErrorContext.MGF_OK;

            case EntityTypeContext.TD:
                // Set diffuse transmittance
                if (ac != 2) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (!TokenValidationContext.isFloat(av[1])) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                currentMaterialContext.td = Float.parseFloat(av[1]);
                if (currentMaterialContext.td < 0.0 || currentMaterialContext.td > 1.0) {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                currentMaterialContext.td_c.copy(context.currentColor);
                currentMaterialContext.clock++;
                return ParseErrorContext.MGF_OK;

            case EntityTypeContext.RS:
                // Set specular reflectance
                if (ac != 3) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (!TokenValidationContext.isFloat(av[1]) || !TokenValidationContext.isFloat(av[2])) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                currentMaterialContext.rs = Float.parseFloat(av[1]);
                currentMaterialContext.rs_a = Float.parseFloat(av[2]);
                if (currentMaterialContext.rs < 0.0 || currentMaterialContext.rs > 1.0 ||
                     currentMaterialContext.rs_a < 0.0) {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                currentMaterialContext.rs_c.copy(context.currentColor);
                currentMaterialContext.clock++;
                return ParseErrorContext.MGF_OK;

            case EntityTypeContext.TS:
                // Set specular transmittance
                if (ac != 3) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (!TokenValidationContext.isFloat(av[1]) || !TokenValidationContext.isFloat(av[2])) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                currentMaterialContext.ts = Float.parseFloat(av[1]);
                currentMaterialContext.ts_a = Float.parseFloat(av[2]);
                if (currentMaterialContext.ts < 0.0 || currentMaterialContext.ts > 1.0 ||
                     currentMaterialContext.ts_a < 0.0) {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                currentMaterialContext.ts_c.copy(context.currentColor);
                currentMaterialContext.clock++;
                return ParseErrorContext.MGF_OK;

            case EntityTypeContext.SIDES:
                // Set number of sides
                if (ac != 2) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (!TokenValidationContext.isInt(av[1])) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                i = Integer.parseInt(av[1]);
                if (i == 1) {
                    currentMaterialContext.sided = true;
                } else if (i == 2) {
                    currentMaterialContext.sided = false;
                } else {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                currentMaterialContext.clock++;
                return ParseErrorContext.MGF_OK;

            default:
                break;
        }
        return ParseErrorContext.MGF_ERROR_UNKNOWN_ENTITY;
    }
}
