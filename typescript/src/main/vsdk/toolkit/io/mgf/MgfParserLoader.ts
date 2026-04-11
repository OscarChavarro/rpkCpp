import { InputStream } from "../../../../java/io/InputStream";
import { Error } from "../../common/Error";
import { ColorContext } from "../context/ColorContext";
import { EntityDispatchContext } from "../context/EntityDispatchContext";
import { EntityTypeContext } from "../context/EntityTypeContext";
import { HandlerRoleContext } from "../context/HandlerRoleContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { ParseSnapshotContext } from "../context/ParseSnapshotContext";
import { ReaderContext } from "../context/ReaderContext";
import { TransformStackContext } from "../context/TransformStackContext";
import { Material } from "../../material/Material";
import { Compound } from "../../skin/Compound";
import { Geometry } from "../../skin/Geometry";
import { GeometryClassId } from "../../skin/GeometryClassId";
import { MgfColorEntitySupport } from "./MgfColorEntitySupport";
import { MgfConeEntityTessellator } from "./MgfConeEntityTessellator";
import { MgfCylinderEntityExpander } from "./MgfCylinderEntityExpander";
import { MgfEntityControl } from "./MgfEntityControl";
import { MgfEntityHandlerAdapter } from "./MgfEntityHandlerAdapter";
import { MgfFaceWithHolesEntityExpander } from "./MgfFaceWithHolesEntityExpander";
import { MgfMaterialEntitySupport } from "./MgfMaterialEntitySupport";
import { MgfObjectNameSupport } from "./MgfObjectNameSupport";
import { MgfPrismEntityTessellator } from "./MgfPrismEntityTessellator";
import { MgfRingEntityTessellator } from "./MgfRingEntityTessellator";
import { MgfSphereEntityExpander } from "./MgfSphereEntityExpander";
import { MgfTorusEntityExpander } from "./MgfTorusEntityExpander";
import { MgfTransformationSupport } from "./MgfTransformationSupport";
import { MgfVertexFaceEntitySupport } from "./MgfVertexFaceEntitySupport";

export class MgfParserLoader {
  private constructor() {
  }

  /**
  Read next line from file.
  */
  private static readInputLine(inputStream: InputStream | null, maxLength: number): string {
    if (inputStream === null || maxLength <= 0) {
      return "";
    }
    let buffer = "";
    try {
      while (buffer.length < maxLength - 1) {
        const readChar = inputStream.read();
        if (readChar < 0) {
          break;
        }
        buffer += String.fromCharCode(readChar);
        if (readChar === "\n".charCodeAt(0)) {
          break;
        }
      }
    }
    catch (_ignored) {
      return "";
    }
    return buffer;
  }

  private static mgfReadNextLine(context: ParseRuntimeContext): number {
    if (context.readerContext === null || context.readerContext.inputStream === null) {
      return 0;
    }

    let len = 0;
    let lineBuilder = "";

    do {
      const maxLength = ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH - len;
      if (maxLength <= 0) {
        return len;
      }
      const readBuffer = MgfParserLoader.readInputLine(context.readerContext.inputStream, maxLength);
      const readLength = readBuffer.length;
      if (readLength <= 0) {
        context.readerContext.inputLine = lineBuilder;
        return len;
      }

      lineBuilder += readBuffer;
      len = lineBuilder.length;
      if (len >= ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH - 1) {
        context.readerContext.inputLine = lineBuilder;
        return len;
      }
      context.readerContext.lineNumber++;
    } while (len > 1 && lineBuilder.charAt(len - 2) === "\\");

    context.readerContext.inputLine = lineBuilder;
    return len;
  }

  /**
  Parse current input line.
  */
  private static mgfParseCurrentLine(context: ParseRuntimeContext): number {
    const argv = new Array<string>(ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT);
    let argc = 0;

    // Copy line, removing escape chars.
    let buffer = "";
    const inputLine = context.readerContext?.inputLine ?? "";
    for (let i = 0; i < inputLine.length; i++) {
      const current = inputLine.charAt(i);
      const next = (i + 1 < inputLine.length) ? inputLine.charAt(i + 1) : "\0";
      if (current === "\\" && next === "\n") {
        continue;
      }
      buffer += current;
    }

    const tokens = buffer.trim().length > 0 ? buffer.trim().split(/[ \t\r\n\f\v]+/) : [];
    for (let i = 0; i < tokens.length; i++) {
      if (argc >= ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT - 1) {
        return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
      }
      argv[argc] = tokens[i];
      argc++;
    }
    if (argc === 0) {
      return ParseErrorContext.MGF_OK; // No words in line
    }

    return MgfEntityControl.mgfHandle(-1, argc, argv.slice(0, argc), context);
  }

  /**
  Clear parser history.
  */
  private static mgfClear(context: ParseRuntimeContext): void {
    MgfColorEntitySupport.initColorContextTables(context);
    MgfVertexFaceEntitySupport.initGeometryContextTables(context);
    MgfMaterialEntitySupport.initMaterialContextTables(context);
    while (context.readerContext !== null) {
      // Reset file contexts
      MgfEntityControl.mgfClose(context);
    }
  }

  /**
  Sets number of quarter circle divisions for discrete approximations.
  */
  private static mgfSetNrQuartCircDivs(divs: number): void {
    if (divs <= 0) {
      Error.error(null, "Number of quarter circle divisions (%d) should be positive", divs);
    }
  }

  /**
  If yesno is true, all materials will be converted to monochrome.
  */
  private static mgfSetMonochrome(yesno: boolean, context: ParseRuntimeContext): void {
    context.monochrome = yesno;
    context.parserConfig.monochrome = yesno;
  }

  /**
  Discard unneeded/unwanted entity.
  */
  private static mgfDiscardUnNeededEntity(ac: number, av: string[], context: ParseRuntimeContext): number {
    void ac;
    void av;
    void context;
    return ParseErrorContext.MGF_OK;
  }

  /**
  Put out current color spectrum.
  */
  private static mgfPutCSpec(context: ParseRuntimeContext): number {
    const wl = ["", ""];
    const buffer = new Array<string>(ColorContext.NUMBER_OF_SPECTRAL_SAMPLES);
    const newAv = new Array<string>(ColorContext.NUMBER_OF_SPECTRAL_SAMPLES + 4);

    if (MgfParserLoader.mgfHandlerMatches(
      context.readerStackState.handleCallbacks[EntityTypeContext.C_SPEC],
      HandlerRoleContext.HANDLE_COLOR,
    ) === false) {
      wl[0] = `${ColorContext.COLOR_MINIMUM_WAVE_LENGTH}`;
      wl[1] = `${ColorContext.COLOR_MAXIMUM_WAVE_LENGTH}`;
      newAv[0] = context.entityNames[EntityTypeContext.C_SPEC];
      newAv[1] = wl[0];
      newAv[2] = wl[1];
      const sf = ColorContext.NUMBER_OF_SPECTRAL_SAMPLES / (context.currentColor as ColorContext).spectralStraightSum;
      for (let i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
        buffer[i] = `${(sf * (context.currentColor as ColorContext).straightSamples[i]).toFixed(4)}`;
        newAv[i + 3] = buffer[i];
      }
      const status = MgfEntityControl.mgfHandle(
        EntityTypeContext.C_SPEC,
        ColorContext.NUMBER_OF_SPECTRAL_SAMPLES + 3,
        newAv.slice(0, ColorContext.NUMBER_OF_SPECTRAL_SAMPLES + 3),
        context,
      );
      if (status !== ParseErrorContext.MGF_OK) {
        return status;
      }
    }
    return ParseErrorContext.MGF_OK;
  }

  /**
  Put out current xy chromatic values.
  */
  private static mgfPutCxy(context: ParseRuntimeContext): number {
    const cCom = [
      context.entityNames[EntityTypeContext.CXY],
      `${(context.currentColor as ColorContext).cx.toFixed(4)}`,
      `${(context.currentColor as ColorContext).cy.toFixed(4)}`,
    ];
    return MgfEntityControl.mgfHandle(EntityTypeContext.CXY, 3, cCom, context);
  }

  /**
  Handle spectral color.
  */
  private static mgfECSpec(ac: number, av: string[], context: ParseRuntimeContext): number {
    void ac;
    void av;
    // Convert to xy chromaticity
    (context.currentColor as ColorContext).fixColorRepresentation(ColorContext.COLOR_XY_IS_SET_FLAG);
    if (MgfParserLoader.mgfHandlerMatches(
      context.readerStackState.handleCallbacks[EntityTypeContext.CXY],
      HandlerRoleContext.HANDLE_COLOR,
    ) === false) {
      return MgfParserLoader.mgfPutCxy(context);
    }
    return ParseErrorContext.MGF_OK;
  }

  /**
  Handle mixing of colors.
  */
  private static mgfECMix(ac: number, av: string[], context: ParseRuntimeContext): number {
    void ac;
    void av;
    if (MgfParserLoader.mgfHandlerMatches(
      context.readerStackState.handleCallbacks[EntityTypeContext.C_SPEC],
      HandlerRoleContext.COLOR_SPEC_HELPER,
    )) {
      (context.currentColor as ColorContext).fixColorRepresentation(ColorContext.COLOR_XY_IS_SET_FLAG);
    }
    else if (((context.currentColor as ColorContext).flags & ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG) !== 0) {
      return MgfParserLoader.mgfPutCSpec(context);
    }
    if (MgfParserLoader.mgfHandlerMatches(
      context.readerStackState.handleCallbacks[EntityTypeContext.CXY],
      HandlerRoleContext.HANDLE_COLOR,
    ) === false) {
      return MgfParserLoader.mgfPutCxy(context);
    }
    return ParseErrorContext.MGF_OK;
  }

  /**
  Handle color temperature.
  */
  private static mgfColorTemperature(ac: number, av: string[], context: ParseRuntimeContext): number {
    void ac;
    void av;
    if (MgfParserLoader.mgfHandlerMatches(
      context.readerStackState.handleCallbacks[EntityTypeContext.C_SPEC],
      HandlerRoleContext.COLOR_SPEC_HELPER,
    ) === false) {
      return MgfParserLoader.mgfPutCSpec(context);
    }
    (context.currentColor as ColorContext).fixColorRepresentation(ColorContext.COLOR_XY_IS_SET_FLAG);
    if (MgfParserLoader.mgfHandlerMatches(
      context.readerStackState.handleCallbacks[EntityTypeContext.CXY],
      HandlerRoleContext.HANDLE_COLOR,
    ) === false) {
      return MgfParserLoader.mgfPutCxy(context);
    }
    return ParseErrorContext.MGF_OK;
  }

  private static handleIncludedFile(ac: number, av: string[], context: ParseRuntimeContext): number {
    const transformArgument = new Array<string>(ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT);
    const readerContext = new ReaderContext();
    const originTransform = context.transformContext;

    if (ac < 2) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    let rv = MgfEntityControl.mgfOpen(readerContext, av[1], context);
    if (rv !== ParseErrorContext.MGF_OK) {
      return rv;
    }
    if (ac > 2) {
      transformArgument[0] = context.entityNames[EntityTypeContext.TRANSFORM];
      for (let i = 1; i < ac - 1; i++) {
        transformArgument[i] = av[i + 1];
      }
      rv = MgfEntityControl.mgfHandle(
        EntityTypeContext.TRANSFORM,
        ac - 1,
        transformArgument.slice(0, ac - 1),
        context,
      );
      if (rv !== ParseErrorContext.MGF_OK) {
        MgfEntityControl.mgfClose(context);
        return rv;
      }
    }
    do {
      while ((rv = MgfParserLoader.mgfReadNextLine(context)) > 0) {
        if (rv >= ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH - 1) {
          process.stderr.write(
            `${readerContext.fileName}: ${readerContext.lineNumber}: `
            + `${context.errorCodeMessages[ParseErrorContext.MGF_ERROR_LINE_TOO_LONG]}\n`,
          );
          MgfEntityControl.mgfClose(context);
          return ParseErrorContext.MGF_ERROR_IN_INCLUDED_FILE;
        }
        rv = MgfParserLoader.mgfParseCurrentLine(context);
        if (rv !== ParseErrorContext.MGF_OK) {
          process.stderr.write(
            `${readerContext.fileName}: ${readerContext.lineNumber}: `
            + `${context.errorCodeMessages[rv]}:\n${readerContext.inputLine}`,
          );
          MgfEntityControl.mgfClose(context);
          return ParseErrorContext.MGF_ERROR_IN_INCLUDED_FILE;
        }
      }
      if (ac > 2) {
        rv = MgfEntityControl.mgfHandle(
          EntityTypeContext.TRANSFORM,
          1,
          transformArgument.slice(0, 1),
          context,
        );
        if (rv !== ParseErrorContext.MGF_OK) {
          MgfEntityControl.mgfClose(context);
          return rv;
        }
      }
    } while (context.transformContext !== originTransform);
    MgfEntityControl.mgfClose(context);
    return ParseErrorContext.MGF_OK;
  }

  private static ensureSessionHandlerRegistry(context: ParseRuntimeContext): void {
    if (context === null) {
      return;
    }

    const handlers = context.readerStackState.handlerByType;
    if (handlers[0] !== null) {
      return;
    }

    handlers[HandlerRoleContext.DISCARD_UNNEEDED] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.DISCARD_UNNEEDED, MgfParserLoader.mgfDiscardUnNeededEntity);
    handlers[HandlerRoleContext.INCLUDE_FILE] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.INCLUDE_FILE, MgfParserLoader.handleIncludedFile);
    handlers[HandlerRoleContext.ENTITY_SPHERE] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_SPHERE, MgfSphereEntityExpander.handleEntity);
    handlers[HandlerRoleContext.ENTITY_TORUS] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_TORUS, MgfTorusEntityExpander.handleEntity);
    handlers[HandlerRoleContext.ENTITY_CYLINDER] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_CYLINDER, MgfCylinderEntityExpander.handleEntity);
    handlers[HandlerRoleContext.ENTITY_RING] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_RING, MgfRingEntityTessellator.handleEntity);
    handlers[HandlerRoleContext.ENTITY_CONE] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_CONE, MgfConeEntityTessellator.handleEntity);
    handlers[HandlerRoleContext.ENTITY_PRISM] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_PRISM, MgfPrismEntityTessellator.handleEntity);
    handlers[HandlerRoleContext.ENTITY_FACE_WITH_HOLES] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_FACE_WITH_HOLES, MgfFaceWithHolesEntityExpander.handleEntity);
    handlers[HandlerRoleContext.COLOR_SPEC_HELPER] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.COLOR_SPEC_HELPER, MgfParserLoader.mgfECSpec);
    handlers[HandlerRoleContext.COLOR_MIX_HELPER] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.COLOR_MIX_HELPER, MgfParserLoader.mgfECMix);
    handlers[HandlerRoleContext.COLOR_TEMPERATURE_HELPER] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.COLOR_TEMPERATURE_HELPER, MgfParserLoader.mgfColorTemperature);
    handlers[HandlerRoleContext.HANDLE_VERTEX] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_VERTEX, MgfVertexFaceEntitySupport.handleVertexEntity);
    handlers[HandlerRoleContext.HANDLE_FACE] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_FACE, MgfVertexFaceEntitySupport.handleFaceEntity);
    handlers[HandlerRoleContext.HANDLE_FACE_WITH_HOLES] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_FACE_WITH_HOLES, MgfVertexFaceEntitySupport.handleFaceWithHolesEntity);
    handlers[HandlerRoleContext.HANDLE_SURFACE] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_SURFACE, MgfVertexFaceEntitySupport.handleSurfaceEntity);
    handlers[HandlerRoleContext.HANDLE_COLOR] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_COLOR, MgfColorEntitySupport.handleColorEntity);
    handlers[HandlerRoleContext.HANDLE_MATERIAL] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_MATERIAL, MgfMaterialEntitySupport.handleMaterialEntity);
    handlers[HandlerRoleContext.HANDLE_TRANSFORM] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_TRANSFORM, MgfTransformationSupport.handleTransformationEntity);
    handlers[HandlerRoleContext.HANDLE_OBJECT] =
      new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_OBJECT, MgfObjectNameSupport.handleObjectEntity);
  }

  private static mgfHandlerFromType(context: ParseRuntimeContext, handlerType: HandlerRoleContext): EntityDispatchContext {
    MgfParserLoader.ensureSessionHandlerRegistry(context);

    const handlerIndex = handlerType as number;
    const handler = context.readerStackState.handlerByType[handlerIndex];
    if (handler === null) {
      Error.fatal(-1, "mgfHandlerFromType", "Missing MGF handler for type %d", handlerIndex);
    }
    return handler as EntityDispatchContext;
  }

  private static mgfHandlerMatches(handler: EntityDispatchContext | null, handlerType: HandlerRoleContext): boolean {
    return handler !== null && handler.type() === handlerType;
  }

  /**
  Initialize alternate entity handlers.
  */
  private static mgfAlternativeInit(handleCallbacks: Array<EntityDispatchContext | null>, context: ParseRuntimeContext): void {
    let iNeed = 0;
    let uNeed = 0;

    // Pick up slack
    if (handleCallbacks[EntityTypeContext.IES] === null) {
      handleCallbacks[EntityTypeContext.IES] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.DISCARD_UNNEEDED);
    }
    if (handleCallbacks[EntityTypeContext.INCLUDE] === null) {
      handleCallbacks[EntityTypeContext.INCLUDE] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.INCLUDE_FILE);
    }
    if (handleCallbacks[EntityTypeContext.SPHERE] === null) {
      handleCallbacks[EntityTypeContext.SPHERE] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.ENTITY_SPHERE);
      iNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.VERTEX);
    }
    else {
      uNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.VERTEX) | (1 << EntityTypeContext.TRANSFORM);
    }
    if (handleCallbacks[EntityTypeContext.CYLINDER] === null) {
      handleCallbacks[EntityTypeContext.CYLINDER] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.ENTITY_CYLINDER);
      iNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.VERTEX);
    }
    else {
      uNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.VERTEX) | (1 << EntityTypeContext.TRANSFORM);
    }
    if (handleCallbacks[EntityTypeContext.CONE] === null) {
      handleCallbacks[EntityTypeContext.CONE] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.ENTITY_CONE);
      iNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.VERTEX);
    }
    else {
      uNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.VERTEX) | (1 << EntityTypeContext.TRANSFORM);
    }
    if (handleCallbacks[EntityTypeContext.RING] === null) {
      handleCallbacks[EntityTypeContext.RING] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.ENTITY_RING);
      iNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.MGF_NORMAL) | (1 << EntityTypeContext.VERTEX);
    }
    else {
      uNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.MGF_NORMAL) | (1 << EntityTypeContext.VERTEX)
        | (1 << EntityTypeContext.TRANSFORM);
    }
    if (handleCallbacks[EntityTypeContext.PRISM] === null) {
      handleCallbacks[EntityTypeContext.PRISM] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.ENTITY_PRISM);
      iNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.VERTEX);
    }
    else {
      uNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.VERTEX) | (1 << EntityTypeContext.TRANSFORM);
    }
    if (handleCallbacks[EntityTypeContext.TORUS] === null) {
      handleCallbacks[EntityTypeContext.TORUS] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.ENTITY_TORUS);
      iNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.MGF_NORMAL) | (1 << EntityTypeContext.VERTEX);
    }
    else {
      uNeed |= (1 << EntityTypeContext.MGF_POINT) | (1 << EntityTypeContext.MGF_NORMAL) | (1 << EntityTypeContext.VERTEX)
        | (1 << EntityTypeContext.TRANSFORM);
    }
    if (handleCallbacks[EntityTypeContext.FACE] === null) {
      handleCallbacks[EntityTypeContext.FACE] = handleCallbacks[EntityTypeContext.FACE_WITH_HOLES];
    }
    else if (handleCallbacks[EntityTypeContext.FACE_WITH_HOLES] === null) {
      handleCallbacks[EntityTypeContext.FACE_WITH_HOLES] =
        MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.ENTITY_FACE_WITH_HOLES);
    }
    if (handleCallbacks[EntityTypeContext.COLOR] !== null) {
      if (handleCallbacks[EntityTypeContext.C_MIX] === null) {
        handleCallbacks[EntityTypeContext.C_MIX] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.COLOR_MIX_HELPER);
        iNeed |= (1 << EntityTypeContext.COLOR) | (1 << EntityTypeContext.CXY) | (1 << EntityTypeContext.C_SPEC)
          | (1 << EntityTypeContext.C_MIX) | (1 << EntityTypeContext.CCT);
      }
      if (handleCallbacks[EntityTypeContext.C_SPEC] === null) {
        handleCallbacks[EntityTypeContext.C_SPEC] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.COLOR_SPEC_HELPER);
        iNeed |= (1 << EntityTypeContext.COLOR) | (1 << EntityTypeContext.CXY) | (1 << EntityTypeContext.C_SPEC)
          | (1 << EntityTypeContext.C_MIX) | (1 << EntityTypeContext.CCT);
      }
      if (handleCallbacks[EntityTypeContext.CCT] === null) {
        handleCallbacks[EntityTypeContext.CCT] =
          MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.COLOR_TEMPERATURE_HELPER);
        iNeed |= (1 << EntityTypeContext.COLOR) | (1 << EntityTypeContext.CXY) | (1 << EntityTypeContext.C_SPEC)
          | (1 << EntityTypeContext.C_MIX) | (1 << EntityTypeContext.CCT);
      }
    }

    // Add support as needed
    if ((iNeed & (1 << EntityTypeContext.VERTEX)) !== 0
      && MgfParserLoader.mgfHandlerMatches(handleCallbacks[EntityTypeContext.VERTEX], HandlerRoleContext.HANDLE_VERTEX) === false) {
      context.readerStackState.supportCallbacks[EntityTypeContext.VERTEX] =
        MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
    }
    if ((iNeed & (1 << EntityTypeContext.MGF_POINT)) !== 0
      && MgfParserLoader.mgfHandlerMatches(handleCallbacks[EntityTypeContext.MGF_POINT], HandlerRoleContext.HANDLE_VERTEX) === false) {
      context.readerStackState.supportCallbacks[EntityTypeContext.MGF_POINT] =
        MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
    }
    if ((iNeed & (1 << EntityTypeContext.MGF_NORMAL)) !== 0
      && MgfParserLoader.mgfHandlerMatches(handleCallbacks[EntityTypeContext.MGF_NORMAL], HandlerRoleContext.HANDLE_VERTEX) === false) {
      context.readerStackState.supportCallbacks[EntityTypeContext.MGF_NORMAL] =
        MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
    }
    if ((iNeed & (1 << EntityTypeContext.COLOR)) !== 0
      && MgfParserLoader.mgfHandlerMatches(handleCallbacks[EntityTypeContext.COLOR], HandlerRoleContext.HANDLE_COLOR) === false) {
      context.readerStackState.supportCallbacks[EntityTypeContext.COLOR] =
        MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
    }
    if ((iNeed & (1 << EntityTypeContext.CXY)) !== 0
      && MgfParserLoader.mgfHandlerMatches(handleCallbacks[EntityTypeContext.CXY], HandlerRoleContext.HANDLE_COLOR) === false) {
      context.readerStackState.supportCallbacks[EntityTypeContext.CXY] =
        MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
    }
    if ((iNeed & (1 << EntityTypeContext.C_SPEC)) !== 0
      && MgfParserLoader.mgfHandlerMatches(handleCallbacks[EntityTypeContext.C_SPEC], HandlerRoleContext.HANDLE_COLOR) === false) {
      context.readerStackState.supportCallbacks[EntityTypeContext.C_SPEC] =
        MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
    }
    if ((iNeed & (1 << EntityTypeContext.C_MIX)) !== 0
      && MgfParserLoader.mgfHandlerMatches(handleCallbacks[EntityTypeContext.C_MIX], HandlerRoleContext.HANDLE_COLOR) === false) {
      context.readerStackState.supportCallbacks[EntityTypeContext.C_MIX] =
        MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
    }
    if ((iNeed & (1 << EntityTypeContext.CCT)) !== 0
      && MgfParserLoader.mgfHandlerMatches(handleCallbacks[EntityTypeContext.CCT], HandlerRoleContext.HANDLE_COLOR) === false) {
      context.readerStackState.supportCallbacks[EntityTypeContext.CCT] =
        MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
    }

    // Discard remaining entities
    for (let i = 0; i < EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES; i++) {
      if (handleCallbacks[i] === null) {
        handleCallbacks[i] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.DISCARD_UNNEEDED);
      }
    }

    void uNeed;
  }

  private static initMgf(context: ParseRuntimeContext): void {
    // Related to ColorContext
    context.readerStackState.handleCallbacks[EntityTypeContext.COLOR] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
    context.readerStackState.handleCallbacks[EntityTypeContext.CXY] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
    context.readerStackState.handleCallbacks[EntityTypeContext.C_MIX] = MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);

    // Related to MaterialContext
    context.readerStackState.handleCallbacks[EntityTypeContext.MGF_MATERIAL] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
    context.readerStackState.handleCallbacks[EntityTypeContext.ED] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
    context.readerStackState.handleCallbacks[EntityTypeContext.IR] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
    context.readerStackState.handleCallbacks[EntityTypeContext.RD] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
    context.readerStackState.handleCallbacks[EntityTypeContext.RS] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
    context.readerStackState.handleCallbacks[EntityTypeContext.SIDES] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
    context.readerStackState.handleCallbacks[EntityTypeContext.TD] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
    context.readerStackState.handleCallbacks[EntityTypeContext.TS] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);

    // Related to TransformStackContext
    context.readerStackState.handleCallbacks[EntityTypeContext.TRANSFORM] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_TRANSFORM);

    // Related to object and geometry
    context.readerStackState.handleCallbacks[EntityTypeContext.OBJECT] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_OBJECT);
    context.readerStackState.handleCallbacks[EntityTypeContext.FACE] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_FACE);
    context.readerStackState.handleCallbacks[EntityTypeContext.FACE_WITH_HOLES] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_FACE_WITH_HOLES);
    context.readerStackState.handleCallbacks[EntityTypeContext.VERTEX] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
    context.readerStackState.handleCallbacks[EntityTypeContext.MGF_POINT] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
    context.readerStackState.handleCallbacks[EntityTypeContext.MGF_NORMAL] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
    context.readerStackState.handleCallbacks[EntityTypeContext.SPHERE] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);
    context.readerStackState.handleCallbacks[EntityTypeContext.TORUS] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);
    context.readerStackState.handleCallbacks[EntityTypeContext.RING] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);
    context.readerStackState.handleCallbacks[EntityTypeContext.CYLINDER] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);
    context.readerStackState.handleCallbacks[EntityTypeContext.CONE] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);
    context.readerStackState.handleCallbacks[EntityTypeContext.PRISM] =
      MgfParserLoader.mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);

    MgfParserLoader.mgfAlternativeInit(context.readerStackState.handleCallbacks, context);
  }

  private static mgfBuildModel(context: ParseRuntimeContext): ParseSnapshotContext | null {
    if (context === null) {
      return null;
    }

    if (context.model === null) {
      context.model = new ParseSnapshotContext();
    }

    const model = context.model;
    model.currentColor = context.currentColor;
    model.currentFaceList = context.currentFaceList;
    model.currentGeometryList = context.currentGeometryList;
    model.currentMaterialName = context.currentMaterialName;
    model.currentNormalList = context.currentNormalList;
    model.currentObjectName = context.currentObjectName;
    model.currentPointList = context.currentPointList;
    model.currentVertexList = context.currentVertexList;
    model.currentVertexName = context.currentVertexName;
    model.geometries = context.geometries;
    model.geometryStackHeadIndex = context.geometryStackHeadIndex;
    model.inComplex = context.inComplex;
    model.inSurface = context.inSurface;
    model.materials = context.materials;
    model.monochrome = context.monochrome;
    model.singleSided = context.singleSided;
    model.warpConeEnds = context.warpConeEnds;
    model.numberOfQuarterCircleDivisions = context.numberOfQuarterCircleDivisions;
    model.readerContext = context.readerContext;
    model.transformContext = context.transformContext;

    return model;
  }

  /**
  Reads an mgf file and returns a parser snapshot.
  */
  public static readMgf(filename: string, context: ParseRuntimeContext): ParseSnapshotContext | null {
    MgfParserLoader.mgfSetNrQuartCircDivs(context.numberOfQuarterCircleDivisions);
    MgfParserLoader.mgfSetMonochrome(context.monochrome, context);

    MgfParserLoader.initMgf(context);

    context.currentGeometryList = [];
    context.geometryBuildState.currentGeometryList = context.currentGeometryList;

    if (context.materials === null) {
      context.materials = [];
      context.materialState.materials = context.materials;
    }

    context.geometryStackHeadIndex = 0;
    context.geometryBuildState.geometryStackHeadIndex = 0;

    context.inComplex = false;
    context.inSurface = false;
    context.geometryBuildState.inComplex = false;
    context.geometryBuildState.inSurface = false;

    MgfObjectNameSupport.mgfObjectNewSurface(context);

    const mgfReaderContext = new ReaderContext();
    let status: number;
    if (filename !== null && filename.length > 0 && filename.charAt(0) === "#") {
      status = MgfEntityControl.mgfOpen(mgfReaderContext, null, context);
    }
    else {
      status = MgfEntityControl.mgfOpen(mgfReaderContext, filename, context);
    }
    if (status !== 0) {
      MgfEntityControl.doError(context.errorCodeMessages[status], context);
    }
    else {
      while (MgfParserLoader.mgfReadNextLine(context) > 0 && status === 0) {
        status = MgfParserLoader.mgfParseCurrentLine(context);
        if (status !== 0) {
          MgfEntityControl.doError(context.errorCodeMessages[status], context);
        }
      }
      MgfEntityControl.mgfClose(context);
    }
    MgfParserLoader.mgfClear(context);

    if (context.inSurface) {
      MgfObjectNameSupport.mgfObjectSurfaceDone(context);
    }
    context.geometries = context.currentGeometryList;
    context.geometryBuildState.geometries = context.geometries;

    return MgfParserLoader.mgfBuildModel(context);
  }

  public static mgfFreeMemory(context: ParseRuntimeContext): void {
    if (context.currentGeometryList !== null) {
      let surfaces = 0;
      let patchSets = 0;
      let compounds = 0;
      let compoundChildren = 0;
      let innerCompoundChildren = 0;
      let unknowns = 0;
      for (let i = 0; i < context.currentGeometryList.length; i++) {
        const geometry = context.currentGeometryList[i];
        if (geometry.className === GeometryClassId.SURFACE_MESH) {
          surfaces++;
        }
        else if (geometry.className === GeometryClassId.PATCH_SET) {
          patchSets++;
        }
        else if (geometry.className === GeometryClassId.COMPOUND) {
          const compound = geometry as Compound;
          if (compound.children !== null) {
            compoundChildren += compound.children.length;
          }
          compounds++;
        }
        else {
          unknowns++;
        }
      }
      process.stdout.write(`  - MeshSurfaces: ${surfaces}\n`);
      process.stdout.write(`  - Patch sets: ${patchSets}\n`);
      process.stdout.write(`  - Compounds: ${compounds}\n`);
      process.stdout.write(`    . Children: ${compoundChildren}\n`);
      process.stdout.write(`    . Inner children: ${innerCompoundChildren}\n`);
      process.stdout.write(`  - Unknowns: ${unknowns}\n`);
    }

    if (context.allGeometries !== null) {
      context.allGeometries.length = 0;
    }

    if (context.currentGeometryList !== null) {
      context.currentGeometryList.length = 0;
      context.currentGeometryList = null;
      context.geometries = null;
    }

    if (context.materials !== null) {
      context.materials.length = 0;
      context.materials = null;
    }

    context.currentObjectName = null;

    if (context.model !== null) {
      context.model = null;
    }

    MgfObjectNameSupport.mgfObjectFreeMemory(context);
    MgfTransformationSupport.mgfTransformFreeMemory(context);
    MgfEntityControl.mgfLookUpFreeMemory(context);
  }
}
