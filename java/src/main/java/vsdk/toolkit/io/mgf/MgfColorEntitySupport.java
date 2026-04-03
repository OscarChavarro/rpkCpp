package vsdk.toolkit.io.mgf;

import vsdk.toolkit.common.dataStructures.LookUpEntity;
import vsdk.toolkit.io.context.ColorContext;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.TokenValidationContext;

public class MgfColorEntitySupport {
    /**
    Handle color entity
    */
    public static int handleColorEntity(int ac, String[] av, ParseRuntimeContext context) {
        int i;
        double wSum;
        LookUpEntity<ColorContext> lp;

        switch (MgfEntityControl.mgfEntity(av[0], context)) {
            case EntityTypeContext.COLOR:
                // Get/set color context
                if (ac > 4) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (ac == 1) {
                    // Set unnamed color context
                    context.unNamedColorContext.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
                    context.currentColor = context.unNamedColorContext;
                    context.colorRepository.currentColor = context.currentColor;
                    return ParseErrorContext.MGF_OK;
                }
                if (!TokenValidationContext.isName(av[1])) {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                lp = context.colorRepository.colorLookUpTable.lookUpFind(av[1]); // Lookup context
                if (lp == null) {
                    return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
                }
                context.currentColor = lp.data;
                context.colorRepository.currentColor = context.currentColor;
                if (ac == 2) {
                    // Re-establish previous context
                    if (context.currentColor == null) {
                        return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
                    }
                    return ParseErrorContext.MGF_OK;
                }
                if (av[2].length() != 1 || av[2].charAt(0) != '=') {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                if (context.currentColor == null) {    /* create new color context */
                    lp.key = av[1];
                    lp.data = new ColorContext();
                    context.currentColor = lp.data;
                    context.colorRepository.currentColor = context.currentColor;
                    context.currentColor.clock = 0;
                }
                i = context.currentColor.clock;
                if (ac == 3) {
                    // Use default template
                    context.currentColor.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
                    context.currentColor.clock = i + 1;
                    return ParseErrorContext.MGF_OK;
                }
                lp = context.colorRepository.colorLookUpTable.lookUpFind(av[3]);
                // Lookup template
                if (lp == null) {
                    return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
                }
                if (lp.data == null) {
                    return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
                }
                context.currentColor.copy(lp.data);
                context.currentColor.clock = i + 1;
                return ParseErrorContext.MGF_OK;
            case EntityTypeContext.CXY:
                // Assign CIE XY value
                if (ac != 3) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (!TokenValidationContext.isFloat(av[1]) || !TokenValidationContext.isFloat(av[2])) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                context.currentColor.cx = Float.parseFloat(av[1]);
                context.currentColor.cy = Float.parseFloat(av[2]);
                context.currentColor.flags = (short)(ColorContext.COLOR_DEFINED_WITH_XY_FLAG | ColorContext.COLOR_XY_IS_SET_FLAG);
                if (context.currentColor.cx < 0.0 || context.currentColor.cy < 0.0 ||
                     context.currentColor.cx + context.currentColor.cy > 1.0) {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                context.currentColor.clock++;
                return ParseErrorContext.MGF_OK;
            case EntityTypeContext.C_SPEC:
                // Assign spectral values
                if (ac < 5) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (!TokenValidationContext.isFloat(av[1]) || !TokenValidationContext.isFloat(av[2])) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                String[] spectrumArguments = new String[ac - 3];
                System.arraycopy(av, 3, spectrumArguments, 0, ac - 3);
                return context.currentColor.setSpectrum(
                    Double.parseDouble(av[1]),
                    Double.parseDouble(av[2]),
                    ac - 3,
                    spectrumArguments);
            case EntityTypeContext.CCT:
                // Assign black body spectrum
                if (ac != 2) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (!TokenValidationContext.isFloat(av[1])) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                return context.currentColor.setBlackBodyTemperature(Double.parseDouble(av[1]));
            case EntityTypeContext.C_MIX:
                // Mix colors
                if (ac < 5 || ((ac - 1) % 2) != 0) {
                    return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
                }
                if (!TokenValidationContext.isFloat(av[1])) {
                    return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                }
                wSum = Double.parseDouble(av[1]);
                lp = context.colorRepository.colorLookUpTable.lookUpFind(av[2]);
                if (lp == null) {
                    return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
                }
                if (lp.data == null) {
                    return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
                }
                context.currentColor.copy(lp.data);
                for (i = 3; i < ac; i += 2) {
                    if (!TokenValidationContext.isFloat(av[i])) {
                        return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
                    }
                    final double w = Double.parseDouble(av[i]);
                    lp = context.colorRepository.colorLookUpTable.lookUpFind(av[i + 1]);
                    if (lp == null) {
                        return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
                    }
                    if (lp.data == null) {
                        return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
                    }
                    context.currentColor.mixColors(
                        wSum,
                        context.currentColor,
                        w,
                        lp.data);
                    wSum += w;
                }
                if (wSum <= 0.0) {
                    return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
                }
                context.currentColor.clock++;
                return ParseErrorContext.MGF_OK;
            default:
                break;
        }
        return ParseErrorContext.MGF_ERROR_UNKNOWN_ENTITY;
    }

    /**
    Empty context tables
    */
    public static void initColorContextTables(ParseRuntimeContext context) {
        context.colorRepository.reset();
        context.currentColor = context.colorRepository.currentColor;
        context.unNamedColorContext = context.colorRepository.unNamedColorContext;
    }
}
