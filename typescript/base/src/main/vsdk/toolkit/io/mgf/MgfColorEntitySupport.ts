import { LookUpEntity } from "../../common/dataStructures/LookUpEntity";
import { ColorContext } from "../context/ColorContext";
import { EntityTypeContext } from "../context/EntityTypeContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { TokenValidationContext } from "../context/TokenValidationContext";
import { MgfEntityControl } from "./MgfEntityControl";

export class MgfColorEntitySupport {
  private constructor() {
  }

  /**
  Handle color entity
  */
  public static handleColorEntity(ac: number, av: string[], context: ParseRuntimeContext): number {
    let i: number;
    let wSum: number;
    let lp: LookUpEntity<ColorContext> | null;

    switch (MgfEntityControl.mgfEntity(av[0], context)) {
      case EntityTypeContext.COLOR:
        // Get/set color context
        if (ac > 4) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (ac === 1) {
          // Set unnamed color context
          (context.unNamedColorContext as ColorContext).copy(ColorContext.DEFAULT_COLOR_CONTEXT);
          context.currentColor = context.unNamedColorContext;
          context.colorRepository.currentColor = context.currentColor;
          return ParseErrorContext.MGF_OK;
        }
        if (!TokenValidationContext.isName(av[1])) {
          return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        lp = (context.colorRepository.colorLookUpTable as any).lookUpFind(av[1]); // Lookup context
        if (lp === null) {
          return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
        }
        context.currentColor = lp.data;
        context.colorRepository.currentColor = context.currentColor;
        if (ac === 2) {
          // Re-establish previous context
          if (context.currentColor === null) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
          }
          return ParseErrorContext.MGF_OK;
        }
        if (av[2].length !== 1 || av[2].charAt(0) !== "=") {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        if (context.currentColor === null) {
          // Create new color context
          lp.key = av[1];
          lp.data = new ColorContext();
          context.currentColor = lp.data;
          context.colorRepository.currentColor = context.currentColor;
          context.currentColor.clock = 0;
        }
        i = context.currentColor.clock;
        if (ac === 3) {
          // Use default template
          context.currentColor.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
          context.currentColor.clock = i + 1;
          return ParseErrorContext.MGF_OK;
        }
        lp = (context.colorRepository.colorLookUpTable as any).lookUpFind(av[3]);
        // Lookup template
        if (lp === null) {
          return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
        }
        if (lp.data === null) {
          return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        context.currentColor.copy(lp.data);
        context.currentColor.clock = i + 1;
        return ParseErrorContext.MGF_OK;
      case EntityTypeContext.CXY:
        // Assign CIE XY value
        if (ac !== 3) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isFloat(av[1]) || !TokenValidationContext.isFloat(av[2])) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        if (context.currentColor === null) {
          return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        context.currentColor.cx = Number.parseFloat(av[1]);
        context.currentColor.cy = Number.parseFloat(av[2]);
        context.currentColor.flags = (ColorContext.COLOR_DEFINED_WITH_XY_FLAG | ColorContext.COLOR_XY_IS_SET_FLAG);
        if (
          context.currentColor.cx < 0.0
          || context.currentColor.cy < 0.0
          || context.currentColor.cx + context.currentColor.cy > 1.0
        ) {
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
        if (context.currentColor === null) {
          return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        return context.currentColor.setSpectrum(
          Number.parseFloat(av[1]),
          Number.parseFloat(av[2]),
          ac - 3,
          av.slice(3, ac),
        );
      case EntityTypeContext.CCT:
        // Assign black body spectrum
        if (ac !== 2) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isFloat(av[1])) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        if (context.currentColor === null) {
          return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        return context.currentColor.setBlackBodyTemperature(Number.parseFloat(av[1]));
      case EntityTypeContext.C_MIX:
        // Mix colors
        if (ac < 5 || ((ac - 1) % 2) !== 0) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isFloat(av[1])) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        wSum = Number.parseFloat(av[1]);
        lp = (context.colorRepository.colorLookUpTable as any).lookUpFind(av[2]);
        if (lp === null) {
          return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
        }
        if (lp.data === null || context.currentColor === null) {
          return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        context.currentColor.copy(lp.data);
        for (i = 3; i < ac; i += 2) {
          if (!TokenValidationContext.isFloat(av[i])) {
            return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
          }
          const w = Number.parseFloat(av[i]);
          lp = (context.colorRepository.colorLookUpTable as any).lookUpFind(av[i + 1]);
          if (lp === null) {
            return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
          }
          if (lp.data === null) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
          }
          context.currentColor.mixColors(
            wSum,
            context.currentColor,
            w,
            lp.data,
          );
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
  public static initColorContextTables(context: ParseRuntimeContext): void {
    context.colorRepository.reset();
    context.currentColor = context.colorRepository.currentColor;
    context.unNamedColorContext = context.colorRepository.unNamedColorContext;
  }
}
