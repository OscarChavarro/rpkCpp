import { Cie } from "../../common/color/Cie";
import { ColorRgb } from "../../common/color/ColorRgb";
import { LookUpEntity } from "../../common/dataStructures/LookUpEntity";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { ColorContext } from "../context/ColorContext";
import { EntityTypeContext } from "../context/EntityTypeContext";
import { MaterialContext } from "../context/MaterialContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { TokenValidationContext } from "../context/TokenValidationContext";
import { Material } from "../../material/Material";
import { PhongBidirectionalReflectanceDistributionFunction } from "../../material/PhongBidirectionalReflectanceDistributionFunction";
import { PhongBidirectionalScatteringDistributionFunction } from "../../material/PhongBidirectionalScatteringDistributionFunction";
import { PhongBidirectionalTransmittanceDistributionFunction } from "../../material/PhongBidirectionalTransmittanceDistributionFunction";
import { PhongEmittanceDistributionFunction } from "../../material/PhongEmittanceDistributionFunction";
import { MgfEntityControl } from "./MgfEntityControl";

export class MgfMaterialEntitySupport {
  private static readonly NUMBER_OF_SAMPLES = 3;

  private constructor() {
  }

  /**
  Looks up a material with given name in the current material list.
  */
  private static materialLookup(name: string, context: ParseRuntimeContext): Material | null {
    for (let i = 0; context.materials !== null && i < context.materials.length; i++) {
      const material = context.materials[i]!;
      if (material !== null && material.getName() !== null && material.getName() === name) {
        return material;
      }
    }
    return null;
  }

  /**
  Translates mgf color into output color representation.
  */
  private static mgfGetColor(cin: ColorContext, intensity: number, colorOut: ColorRgb, context: ParseRuntimeContext): void {
    const xyz = [0.0, 0.0, 0.0];
    const rgb = [0.0, 0.0, 0.0];

    cin.fixColorRepresentation(ColorContext.COLOR_XY_IS_SET_FLAG);
    if (cin.cy > Numeric.EPSILON) {
      xyz[0] = cin.cx / cin.cy * intensity;
      xyz[1] = 1.0 * intensity;
      xyz[2] = (1.0 - cin.cx - cin.cy) / cin.cy * intensity;
    }
    else {
      MgfEntityControl.doWarning("invalid color specification (Y<=0) ... setting to black", context);
      xyz[0] = 0.0;
      xyz[1] = 0.0;
      xyz[2] = 0.0;
    }

    if (xyz[0] < 0.0 || xyz[1] < 0.0 || xyz[2] < 0.0) {
      MgfEntityControl.doWarning("invalid color specification (negative CIE XYZ components) ... clipping to zero", context);
      if (xyz[0] < 0.0) {
        xyz[0] = 0.0;
      }
      if (xyz[1] < 1.0) {
        xyz[1] = 0.0;
      }
      if (xyz[2] < 2.0) {
        xyz[2] = 0.0;
      }
    }

    Cie.transformColorFromXYZ2RGB(xyz, rgb);
    if (Cie.clipGamut(rgb)) {
      MgfEntityControl.doWarning("color desaturated during gamut clipping", context);
    }
    colorOut.set(rgb[0]!, rgb[1]!, rgb[2]!);
  }

  private static specSamples(col: ColorRgb, rgb: number[]): void {
    rgb[0] = col.r;
    rgb[1] = col.g;
    rgb[2] = col.b;
  }

  private static colorMax(col: ColorRgb): number {
    // First approximation: evaluate only RGB primaries.
    const samples = [0.0, 0.0, 0.0];
    MgfMaterialEntitySupport.specSamples(col, samples);

    let mx = -Numeric.HUGE_FLOAT_VALUE;
    for (let i = 0; i < MgfMaterialEntitySupport.NUMBER_OF_SAMPLES; i++) {
      if (samples[i]! > mx) {
        mx = samples[i]!;
      }
    }
    return mx;
  }

  /**
  Checks whether current MGF material changed and converts it to toolkit material.
  Returns true if material in use changed.
  */
  public static mgfGetCurrentMaterial(material: Array<Material | null>, allSurfacesSided: boolean, context: ParseRuntimeContext): boolean {
    const Ed = new ColorRgb();
    const Es = new ColorRgb();
    const Rd = new ColorRgb();
    const Td = new ColorRgb();
    const Rs = new ColorRgb();
    const Ts = new ColorRgb();
    const A = new ColorRgb();
    const currentMaterialContext = context.materialRepository.currentMaterialContext as MaterialContext;
    let materialName = context.currentMaterialName;
    if (materialName === null || materialName.length <= 0) {
      materialName = "unnamed";
    }

    // If same material and unchanged context, keep it.
    if (material[0] !== null && materialName === material[0]!.getName() && currentMaterialContext.clock === 0) {
      return false;
    }

    const storedMaterial = MgfMaterialEntitySupport.materialLookup(materialName, context);
    if (storedMaterial !== null && currentMaterialContext.clock === 0) {
      material[0] = storedMaterial;
      return true;
    }

    // Convert intensities and chromaticities.
    MgfMaterialEntitySupport.mgfGetColor(currentMaterialContext.ed_c, currentMaterialContext.ed, Ed, context);
    MgfMaterialEntitySupport.mgfGetColor(currentMaterialContext.rd_c, currentMaterialContext.rd, Rd, context);
    MgfMaterialEntitySupport.mgfGetColor(currentMaterialContext.td_c, currentMaterialContext.td, Td, context);
    MgfMaterialEntitySupport.mgfGetColor(currentMaterialContext.rs_c, currentMaterialContext.rs, Rs, context);
    MgfMaterialEntitySupport.mgfGetColor(currentMaterialContext.ts_c, currentMaterialContext.ts, Ts, context);

    // Check/correct reflectances/transmittances.
    A.add(Rd, Rs);
    let a = MgfMaterialEntitySupport.colorMax(A);
    if (a > 1.0 - Numeric.EPSILON_FLOAT) {
      MgfEntityControl.doWarning("invalid material specification: total reflectance shall be < 1", context);
      a = (1.0 - Numeric.EPSILON_FLOAT) / a;
      Rd.scale(a);
      Rs.scale(a);
    }

    A.add(Td, Ts);
    a = MgfMaterialEntitySupport.colorMax(A);
    if (a > 1.0 - Numeric.EPSILON_FLOAT) {
      MgfEntityControl.doWarning("invalid material specification: total transmittance shall be < 1", context);
      a = (1.0 - Numeric.EPSILON_FLOAT) / a;
      Td.scale(a);
      Ts.scale(a);
    }

    // Convert lumen/m^2 to W/m^2.
    Ed.scale(1.0 / Cie.WHITE_EFFICACY);
    Es.clear();

    let Nr: number;
    let Nt: number;

    // Specular power = (0.6 / roughness)^2.
    if (currentMaterialContext.rs_a !== 0.0) {
      Nr = 0.6 / currentMaterialContext.rs_a;
      Nr *= Nr;
    }
    else {
      Nr = 0.0;
    }

    if (currentMaterialContext.ts_a !== 0.0) {
      Nt = 0.6 / currentMaterialContext.ts_a;
      Nt *= Nt;
    }
    else {
      Nt = 0.0;
    }

    if (context.monochrome) {
      Ed.setMonochrome(Cie.spectrumGray(Ed.r, Ed.g, Ed.b));
      Es.setMonochrome(Cie.spectrumGray(Es.r, Es.g, Es.b));
      Rd.setMonochrome(Cie.spectrumGray(Rd.r, Rd.g, Rd.b));
      Rs.setMonochrome(Cie.spectrumGray(Rs.r, Rs.g, Rs.b));
      Td.setMonochrome(Cie.spectrumGray(Td.r, Td.g, Td.b));
      Ts.setMonochrome(Cie.spectrumGray(Ts.r, Ts.g, Ts.b));
    }

    let edf: PhongEmittanceDistributionFunction | null = null;
    if (!Ed.isBlack() || !Es.isBlack()) {
      const Ne = 0.0;
      edf = new PhongEmittanceDistributionFunction(Ed, Es, Ne);
    }

    let brdf: PhongBidirectionalReflectanceDistributionFunction | null = null;
    if (!Rd.isBlack() || !Rs.isBlack()) {
      brdf = new PhongBidirectionalReflectanceDistributionFunction(Rd, Rs, Nr);
    }

    let btdf: PhongBidirectionalTransmittanceDistributionFunction | null = null;
    if (!Td.isBlack() || !Ts.isBlack()) {
      btdf = new PhongBidirectionalTransmittanceDistributionFunction(
        Td,
        Ts,
        Nt,
        currentMaterialContext.nr,
        currentMaterialContext.ni,
      );
    }

    const bsdf = new PhongBidirectionalScatteringDistributionFunction(brdf, btdf, null);
    material[0] = new Material(
      materialName,
      edf as unknown as PhongEmittanceDistributionFunction,
      bsdf as unknown as PhongBidirectionalScatteringDistributionFunction,
      allSurfacesSided || currentMaterialContext.sided,
    );

    if (context.materials === null) {
      context.materials = [];
      context.materialState.materials = context.materials;
    }
    context.materials.push(material[0]);

    // Reset clock to track future changes.
    currentMaterialContext.clock = 0;
    return true;
  }

  public static initMaterialContextTables(context: ParseRuntimeContext): void {
    context.materialRepository.reset();
    context.currentMaterialName = null;
    context.materialState.currentMaterialName = null;
  }

  /**
  Returns true if current material has changed.
  */
  public static mgfMaterialChanged(material: Material | null, context: ParseRuntimeContext): boolean {
    let materialName = context.currentMaterialName;
    if (materialName === null || materialName.length <= 0) {
      materialName = "unnamed";
    }

    if (
      material !== null
      && materialName === material.getName()
      && (context.materialRepository.currentMaterialContext as MaterialContext).clock === 0
    ) {
      return false;
    }

    return true;
  }

  /**
  Handle material entity.
  */
  public static handleMaterialEntity(ac: number, av: string[], context: ParseRuntimeContext): number {
    let i: number;
    let lp: LookUpEntity<MaterialContext> | null;
    let currentMaterialContext = context.materialRepository.currentMaterialContext as MaterialContext;

    switch (MgfEntityControl.mgfEntity(av[0]!, context)) {
      case EntityTypeContext.MGF_MATERIAL:
        // Get / set material context
        if (ac > 4) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (ac === 1) {
          // Set unnamed material context
          context.materialRepository.unNamedMaterialContext.copy(context.materialRepository.defaultMaterialContext);
          currentMaterialContext = context.materialRepository.unNamedMaterialContext;
          context.materialRepository.currentMaterialContext = currentMaterialContext;
          context.currentMaterialName = null;
          context.materialState.currentMaterialName = null;
          return ParseErrorContext.MGF_OK;
        }
        if (!TokenValidationContext.isName(av[1]!)) {
          return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        lp = (context.materialRepository.materialLookUpTable as any).lookUpFind(av[1]!);
        // Lookup context
        if (lp === null) {
          return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
        }
        context.currentMaterialName = lp.key;
        context.materialState.currentMaterialName = context.currentMaterialName;
        currentMaterialContext = lp.data as MaterialContext;
        context.materialRepository.currentMaterialContext = currentMaterialContext;
        if (ac === 2) {
          // Re-establish previous context
          if (currentMaterialContext === null) {
            return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
          }
          return ParseErrorContext.MGF_OK;
        }
        if (av[2]!.length !== 1 || av[2]!.charAt(0) !== "=") {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        if (currentMaterialContext === null) {
          // Create new material
          lp.key = av[1]!;
          lp.data = new MaterialContext();
          context.currentMaterialName = lp.key;
          context.materialState.currentMaterialName = context.currentMaterialName;
          currentMaterialContext = lp.data;
          context.materialRepository.currentMaterialContext = currentMaterialContext;
          currentMaterialContext.clock = 0;
        }
        i = currentMaterialContext.clock;
        if (ac === 3) {
          // Use default template
          currentMaterialContext.copy(context.materialRepository.defaultMaterialContext);
          currentMaterialContext.clock = i + 1;
          return ParseErrorContext.MGF_OK;
        }
        lp = (context.materialRepository.materialLookUpTable as any).lookUpFind(av[3]);
        // Lookup template
        if (lp === null) {
          return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
        }
        if (lp.data === null) {
          return ParseErrorContext.MGF_ERROR_UNDEFINED_REFERENCE;
        }
        currentMaterialContext.copy(lp.data);
        currentMaterialContext.clock = i + 1;
        return ParseErrorContext.MGF_OK;

      case EntityTypeContext.IR:
        // Set index of refraction
        if (ac !== 3) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isFloat(av[1]!) || !TokenValidationContext.isFloat(av[2]!)) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        currentMaterialContext.nr = Number.parseFloat(av[1]!);
        currentMaterialContext.ni = Number.parseFloat(av[2]!);
        if (currentMaterialContext.nr <= Numeric.EPSILON) {
          return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        currentMaterialContext.clock++;
        return ParseErrorContext.MGF_OK;

      case EntityTypeContext.RD:
        // Set diffuse reflectance
        if (ac !== 2) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isFloat(av[1]!)) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        currentMaterialContext.rd = Number.parseFloat(av[1]!);
        if (currentMaterialContext.rd < 0.0 || currentMaterialContext.rd > 1.0) {
          return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        currentMaterialContext.rd_c.copy(context.currentColor as ColorContext);
        currentMaterialContext.clock++;
        return ParseErrorContext.MGF_OK;

      case EntityTypeContext.ED:
        // Set diffuse emittance
        if (ac !== 2) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isFloat(av[1]!)) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        currentMaterialContext.ed = Number.parseFloat(av[1]!);
        if (currentMaterialContext.ed < 0.0) {
          return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        currentMaterialContext.ed_c.copy(context.currentColor as ColorContext);
        currentMaterialContext.clock++;
        return ParseErrorContext.MGF_OK;

      case EntityTypeContext.TD:
        // Set diffuse transmittance
        if (ac !== 2) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isFloat(av[1]!)) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        currentMaterialContext.td = Number.parseFloat(av[1]!);
        if (currentMaterialContext.td < 0.0 || currentMaterialContext.td > 1.0) {
          return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        currentMaterialContext.td_c.copy(context.currentColor as ColorContext);
        currentMaterialContext.clock++;
        return ParseErrorContext.MGF_OK;

      case EntityTypeContext.RS:
        // Set specular reflectance
        if (ac !== 3) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isFloat(av[1]!) || !TokenValidationContext.isFloat(av[2]!)) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        currentMaterialContext.rs = Number.parseFloat(av[1]!);
        currentMaterialContext.rs_a = Number.parseFloat(av[2]!);
        if (currentMaterialContext.rs < 0.0 || currentMaterialContext.rs > 1.0 || currentMaterialContext.rs_a < 0.0) {
          return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        currentMaterialContext.rs_c.copy(context.currentColor as ColorContext);
        currentMaterialContext.clock++;
        return ParseErrorContext.MGF_OK;

      case EntityTypeContext.TS:
        // Set specular transmittance
        if (ac !== 3) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isFloat(av[1]!) || !TokenValidationContext.isFloat(av[2]!)) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        currentMaterialContext.ts = Number.parseFloat(av[1]!);
        currentMaterialContext.ts_a = Number.parseFloat(av[2]!);
        if (currentMaterialContext.ts < 0.0 || currentMaterialContext.ts > 1.0 || currentMaterialContext.ts_a < 0.0) {
          return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
        currentMaterialContext.ts_c.copy(context.currentColor as ColorContext);
        currentMaterialContext.clock++;
        return ParseErrorContext.MGF_OK;

      case EntityTypeContext.SIDES:
        // Set number of sides
        if (ac !== 2) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (!TokenValidationContext.isInt(av[1]!)) {
          return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }
        i = Number.parseInt(av[1]!, 10);
        if (i === 1) {
          currentMaterialContext.sided = true;
        }
        else if (i === 2) {
          currentMaterialContext.sided = false;
        }
        else {
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
