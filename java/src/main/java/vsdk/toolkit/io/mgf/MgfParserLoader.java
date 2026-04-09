package vsdk.toolkit.io.mgf;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.StringTokenizer;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.io.context.ColorContext;
import vsdk.toolkit.io.context.EntityDispatchContext;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.HandlerRoleContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.ParseSnapshotContext;
import vsdk.toolkit.io.context.ReaderContext;
import vsdk.toolkit.io.context.TransformStackContext;
import vsdk.toolkit.material.Material;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.GeometryClassId;

public class MgfParserLoader {
    /**
    Read next line from file
    */
    private static int readInputLine(InputStream inputStream, StringBuilder readBuffer, int maxLength) {
        if (inputStream == null || readBuffer == null || maxLength <= 0) {
            return 0;
        }
        readBuffer.setLength(0);
        int length = 0;
        try {
            while (length < maxLength - 1) {
                final int readChar = inputStream.read();
                if (readChar < 0) {
                    break;
                }
                readBuffer.append((char)readChar);
                length++;
                if (readChar == '\n') {
                    break;
                }
            }
        }
        catch (Exception ignored) {
            return 0;
        }
        return length;
    }

    private static int mgfReadNextLine(ParseRuntimeContext context) {
        if (context.readerContext.inputStream == null) {
            return 0;
        }

        int len = 0;
        StringBuilder lineBuilder = new StringBuilder();
        StringBuilder readBuffer = new StringBuilder();

        do {
            final int maxLength = ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH - len;
            if (maxLength <= 0) {
                return len;
            }
            final int readLength = readInputLine(context.readerContext.inputStream, readBuffer, maxLength);
            if (readLength <= 0) {
                context.readerContext.inputLine = lineBuilder.toString();
                return len;
            }

            lineBuilder.append(readBuffer);
            len = lineBuilder.length();
            if (len >= ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH - 1) {
                context.readerContext.inputLine = lineBuilder.toString();
                return len;
            }
            context.readerContext.lineNumber++;
        } while (len > 1 && lineBuilder.charAt(len - 2) == '\\');

        context.readerContext.inputLine = lineBuilder.toString();
        return len;
    }

    /**
    Parse current input line
    */
    private static int mgfParseCurrentLine(ParseRuntimeContext context) {
        String[] argv = new String[ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT];
        int argc = 0;

        // Copy line, removing escape chars
        StringBuilder buffer = new StringBuilder();
        String inputLine = context.readerContext.inputLine == null ? "" : context.readerContext.inputLine;
        for (int i = 0; i < inputLine.length(); i++) {
            final char current = inputLine.charAt(i);
            final char next = (i + 1 < inputLine.length()) ? inputLine.charAt(i + 1) : '\0';
            if (current == '\\' && next == '\n') {
                continue;
            }
            buffer.append(current);
        }

        StringTokenizer tokenizer = new StringTokenizer(buffer.toString(), " \t\r\n\f\u000b");
        while (tokenizer.hasMoreTokens()) {
            if (argc >= ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT - 1) {
                return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            argv[argc] = tokenizer.nextToken();
            argc++;
        }
        if (argc == 0) {
            // No words in line
            return ParseErrorContext.MGF_OK;
        }
        argv[argc] = null;

        // Else handle it
        return MgfEntityControl.mgfHandle(-1, argc, argv, context);
    }

    /**
    Clear parser history
    */
    private static void mgfClear(ParseRuntimeContext context) {
        MgfColorEntitySupport.initColorContextTables(context);
        MgfVertexFaceEntitySupport.initGeometryContextTables(context);
        MgfMaterialEntitySupport.initMaterialContextTables(context);
        while (context.readerContext != null) {
            // Reset our file context
            MgfEntityControl.mgfClose(context);
        }
    }

    /**
    Sets the number of quarter circle divisions for discrete approximation of cylinders, spheres, cones, etc.
    */
    private static void mgfSetNrQuartCircDivs(int divs) {
        if (divs <= 0) {
            Error.error(null, "Number of quarter circle divisions (%d) should be positive", divs);
            return;
        }
    }

    /**
    If yesno is true, all materials will be converted to be monochrome
    */
    private static void mgfSetMonochrome(boolean yesno, ParseRuntimeContext context) {
        context.monochrome = yesno;
        context.parserConfig.monochrome = yesno;
    }

    /**
    Discard unneeded/unwanted entity
    */
    private static int mgfDiscardUnNeededEntity(int ac, String[] av, ParseRuntimeContext context) {
        return ParseErrorContext.MGF_OK;
    }

    /**
    Put out current color spectrum
    */
    private static int mgfPutCSpec(ParseRuntimeContext context) {
        String[] wl = new String[2];
        String[] buffer = new String[ColorContext.NUMBER_OF_SPECTRAL_SAMPLES];
        String[] newAv = new String[ColorContext.NUMBER_OF_SPECTRAL_SAMPLES + 4];

        if (mgfHandlerMatches(context.readerStackState.handleCallbacks[EntityTypeContext.C_SPEC], HandlerRoleContext.HANDLE_COLOR) == false) {
            wl[0] = Integer.toString(ColorContext.COLOR_MINIMUM_WAVE_LENGTH);
            wl[1] = Integer.toString(ColorContext.COLOR_MAXIMUM_WAVE_LENGTH);
            newAv[0] = context.entityNames[EntityTypeContext.C_SPEC];
            newAv[1] = wl[0];
            newAv[2] = wl[1];
            final double sf = (double)ColorContext.NUMBER_OF_SPECTRAL_SAMPLES / (double)context.currentColor.spectralStraightSum;
            for (int i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
                buffer[i] = String.format(java.util.Locale.US, "%.4f", sf * context.currentColor.straightSamples[i]);
                newAv[i + 3] = buffer[i];
            }
            newAv[ColorContext.NUMBER_OF_SPECTRAL_SAMPLES + 3] = null;
            int status = MgfEntityControl.mgfHandle(EntityTypeContext.C_SPEC, ColorContext.NUMBER_OF_SPECTRAL_SAMPLES + 3, newAv, context);
            if (status != ParseErrorContext.MGF_OK) {
                return status;
            }
        }
        return ParseErrorContext.MGF_OK;
    }

    /**
    Put out current xy chromatic values
    */
    private static int mgfPutCxy(ParseRuntimeContext context) {
        String[] cCom = new String[] {
            context.entityNames[EntityTypeContext.CXY],
            String.format(java.util.Locale.US, "%.4f", context.currentColor.cx),
            String.format(java.util.Locale.US, "%.4f", context.currentColor.cy)
        };
        return MgfEntityControl.mgfHandle(EntityTypeContext.CXY, 3, cCom, context);
    }

    /**
    Handle spectral color
    */
    private static int mgfECSpec(int ac, String[] av, ParseRuntimeContext context) {
        // Convert to xy chromaticity
        context.currentColor.fixColorRepresentation(ColorContext.COLOR_XY_IS_SET_FLAG);
        // If it's really their handler, use it
        if (mgfHandlerMatches(context.readerStackState.handleCallbacks[EntityTypeContext.CXY], HandlerRoleContext.HANDLE_COLOR) == false) {
            return mgfPutCxy(context);
        }
        return ParseErrorContext.MGF_OK;
    }

    /**
    Handle mixing of colors
Contorted logic works as follows:
1. the colors are already mixed in c_h_color() support function
2. if we would handle a spectral result, make sure it's not
3. if MgfColorEntitySupport::handleColorEntity() would handle a spectral result, don't bother
4. otherwise, make c_spec entity and pass it to their handler
5. if we have only xy results, handle it as c_spec() would
    */
    private static int mgfECMix(int ac, String[] av, ParseRuntimeContext context) {
        if (mgfHandlerMatches(context.readerStackState.handleCallbacks[EntityTypeContext.C_SPEC], HandlerRoleContext.COLOR_SPEC_HELPER)) {
            context.currentColor.fixColorRepresentation(ColorContext.COLOR_XY_IS_SET_FLAG);
        } else if ((context.currentColor.flags & ColorContext.COLOR_DEFINED_WITH_SPECTRUM_FLAG) != 0) {
            return mgfPutCSpec(context);
        }
        if (mgfHandlerMatches(context.readerStackState.handleCallbacks[EntityTypeContext.CXY], HandlerRoleContext.HANDLE_COLOR) == false) {
            return mgfPutCxy(context);
        }
        return ParseErrorContext.MGF_OK;
    }

    /**
    Handle color temperature
    */
    private static int mgfColorTemperature(int ac, String[] av, ParseRuntimeContext context) {
        // Logic is similar to mgfECMix here.  Support handler has already
        // converted temperature to spectral color.  Put it out as such
        // if they support it, otherwise convert to xy chromaticity and
        // put it out if they handle it
        if (mgfHandlerMatches(context.readerStackState.handleCallbacks[EntityTypeContext.C_SPEC], HandlerRoleContext.COLOR_SPEC_HELPER) == false) {
            return mgfPutCSpec(context);
        }
        context.currentColor.fixColorRepresentation(ColorContext.COLOR_XY_IS_SET_FLAG);
        if (mgfHandlerMatches(context.readerStackState.handleCallbacks[EntityTypeContext.CXY], HandlerRoleContext.HANDLE_COLOR) == false) {
            return mgfPutCxy(context);
        }
        return ParseErrorContext.MGF_OK;
    }

    private static int handleIncludedFile(int ac, String[] av, ParseRuntimeContext context) {
        String[] transformArgument = new String[ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT];
        ReaderContext readerContext = new ReaderContext();
        TransformStackContext originTransform = context.transformContext;

        if (ac < 2) {
            return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }

        int rv = MgfEntityControl.mgfOpen(readerContext, av[1], context);
        if (rv != ParseErrorContext.MGF_OK) {
            return rv;
        }
        if (ac > 2) {
            transformArgument[0] = context.entityNames[EntityTypeContext.TRANSFORM];
            for (int i = 1; i < ac - 1; i++) {
                transformArgument[i] = av[i + 1];
            }
            transformArgument[ac - 1] = null;
            rv = MgfEntityControl.mgfHandle(EntityTypeContext.TRANSFORM, ac - 1, transformArgument, context);
            if (rv != ParseErrorContext.MGF_OK) {
                MgfEntityControl.mgfClose(context);
                return rv;
            }
        }
        do {
            while ((rv = mgfReadNextLine(context)) > 0) {
                if (rv >= ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH - 1) {
                    System.err.printf("%s: %d: %s\n", readerContext.fileName,
                        readerContext.lineNumber, context.errorCodeMessages[ParseErrorContext.MGF_ERROR_LINE_TOO_LONG]);
                    MgfEntityControl.mgfClose(context);
                    return ParseErrorContext.MGF_ERROR_IN_INCLUDED_FILE;
                }
                rv = mgfParseCurrentLine(context);
                if (rv != ParseErrorContext.MGF_OK) {
                    System.err.printf("%s: %d: %s:\n%s", readerContext.fileName,
                            readerContext.lineNumber, context.errorCodeMessages[rv],
                            readerContext.inputLine);
                    MgfEntityControl.mgfClose(context);
                    return ParseErrorContext.MGF_ERROR_IN_INCLUDED_FILE;
                }
            }
            if (ac > 2) {
                rv = MgfEntityControl.mgfHandle(EntityTypeContext.TRANSFORM, 1, transformArgument, context);
                if (rv != ParseErrorContext.MGF_OK) {
                    MgfEntityControl.mgfClose(context);
                    return rv;
                }
            }
        } while (context.transformContext != originTransform);
        MgfEntityControl.mgfClose(context);
        return ParseErrorContext.MGF_OK;
    }

    private static void ensureSessionHandlerRegistry(ParseRuntimeContext context) {
        if (context == null) {
            return;
        }

        EntityDispatchContext[] handlers = context.readerStackState.handlerByType;
        if (handlers[0] != null) {
            return;
        }

        handlers[HandlerRoleContext.DISCARD_UNNEEDED.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.DISCARD_UNNEEDED, MgfParserLoader::mgfDiscardUnNeededEntity);
        handlers[HandlerRoleContext.INCLUDE_FILE.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.INCLUDE_FILE, MgfParserLoader::handleIncludedFile);
        handlers[HandlerRoleContext.ENTITY_SPHERE.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_SPHERE, MgfSphereEntityExpander::handleEntity);
        handlers[HandlerRoleContext.ENTITY_TORUS.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_TORUS, MgfTorusEntityExpander::handleEntity);
        handlers[HandlerRoleContext.ENTITY_CYLINDER.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_CYLINDER, MgfCylinderEntityExpander::handleEntity);
        handlers[HandlerRoleContext.ENTITY_RING.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_RING, MgfRingEntityTessellator::handleEntity);
        handlers[HandlerRoleContext.ENTITY_CONE.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_CONE, MgfConeEntityTessellator::handleEntity);
        handlers[HandlerRoleContext.ENTITY_PRISM.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_PRISM, MgfPrismEntityTessellator::handleEntity);
        handlers[HandlerRoleContext.ENTITY_FACE_WITH_HOLES.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.ENTITY_FACE_WITH_HOLES, MgfFaceWithHolesEntityExpander::handleEntity);
        handlers[HandlerRoleContext.COLOR_SPEC_HELPER.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.COLOR_SPEC_HELPER, MgfParserLoader::mgfECSpec);
        handlers[HandlerRoleContext.COLOR_MIX_HELPER.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.COLOR_MIX_HELPER, MgfParserLoader::mgfECMix);
        handlers[HandlerRoleContext.COLOR_TEMPERATURE_HELPER.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.COLOR_TEMPERATURE_HELPER, MgfParserLoader::mgfColorTemperature);
        handlers[HandlerRoleContext.HANDLE_VERTEX.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_VERTEX, MgfVertexFaceEntitySupport::handleVertexEntity);
        handlers[HandlerRoleContext.HANDLE_FACE.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_FACE, MgfVertexFaceEntitySupport::handleFaceEntity);
        handlers[HandlerRoleContext.HANDLE_FACE_WITH_HOLES.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_FACE_WITH_HOLES, MgfVertexFaceEntitySupport::handleFaceWithHolesEntity);
        handlers[HandlerRoleContext.HANDLE_SURFACE.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_SURFACE, MgfVertexFaceEntitySupport::handleSurfaceEntity);
        handlers[HandlerRoleContext.HANDLE_COLOR.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_COLOR, MgfColorEntitySupport::handleColorEntity);
        handlers[HandlerRoleContext.HANDLE_MATERIAL.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_MATERIAL, MgfMaterialEntitySupport::handleMaterialEntity);
        handlers[HandlerRoleContext.HANDLE_TRANSFORM.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_TRANSFORM, MgfTransformationSupport::handleTransformationEntity);
        handlers[HandlerRoleContext.HANDLE_OBJECT.ordinal()] = new MgfEntityHandlerAdapter(HandlerRoleContext.HANDLE_OBJECT, MgfObjectNameSupport::handleObjectEntity);
    }

    private static EntityDispatchContext mgfHandlerFromType(ParseRuntimeContext context, HandlerRoleContext handlerType) {
        ensureSessionHandlerRegistry(context);

        final int handlerIndex = handlerType.ordinal();
        EntityDispatchContext handler = context.readerStackState.handlerByType[handlerIndex];
        if (handler == null) {
            Error.fatal(-1, "mgfHandlerFromType", "Missing MGF handler for type %d", handlerIndex);
        }
        return handler;
    }

    private static boolean mgfHandlerMatches(EntityDispatchContext handler, HandlerRoleContext handlerType) {
        return handler != null && handler.type() == handlerType;
    }

    /**
    rayCasterInitialize alternate entity handlers
    */
    private static void mgfAlternativeInit(EntityDispatchContext[] handleCallbacks, ParseRuntimeContext context) {
        long iNeed = 0;
        long uNeed = 0;

        // Pick up slack
        if (handleCallbacks[EntityTypeContext.IES] == null) {
            handleCallbacks[EntityTypeContext.IES] = mgfHandlerFromType(context, HandlerRoleContext.DISCARD_UNNEEDED);
        }
        if (handleCallbacks[EntityTypeContext.INCLUDE] == null) {
            handleCallbacks[EntityTypeContext.INCLUDE] = mgfHandlerFromType(context, HandlerRoleContext.INCLUDE_FILE);
        }
        if (handleCallbacks[EntityTypeContext.SPHERE] == null) {
            handleCallbacks[EntityTypeContext.SPHERE] = mgfHandlerFromType(context, HandlerRoleContext.ENTITY_SPHERE);
            iNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.VERTEX);
        } else {
            uNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.VERTEX) | (1L << EntityTypeContext.TRANSFORM);
        }
        if (handleCallbacks[EntityTypeContext.CYLINDER] == null) {
            handleCallbacks[EntityTypeContext.CYLINDER] = mgfHandlerFromType(context, HandlerRoleContext.ENTITY_CYLINDER);
            iNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.VERTEX);
        } else {
            uNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.VERTEX) | (1L << EntityTypeContext.TRANSFORM);
        }
        if (handleCallbacks[EntityTypeContext.CONE] == null) {
            handleCallbacks[EntityTypeContext.CONE] = mgfHandlerFromType(context, HandlerRoleContext.ENTITY_CONE);
            iNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.VERTEX);
        } else {
            uNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.VERTEX) | (1L << EntityTypeContext.TRANSFORM);
        }
        if (handleCallbacks[EntityTypeContext.RING] == null) {
            handleCallbacks[EntityTypeContext.RING] = mgfHandlerFromType(context, HandlerRoleContext.ENTITY_RING);
            iNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.MGF_NORMAL) | (1L << EntityTypeContext.VERTEX);
        } else {
            uNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.MGF_NORMAL) | (1L << EntityTypeContext.VERTEX) | (1L << EntityTypeContext.TRANSFORM);
        }
        if (handleCallbacks[EntityTypeContext.PRISM] == null) {
            handleCallbacks[EntityTypeContext.PRISM] = mgfHandlerFromType(context, HandlerRoleContext.ENTITY_PRISM);
            iNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.VERTEX);
        } else {
            uNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.VERTEX) | (1L << EntityTypeContext.TRANSFORM);
        }
        if (handleCallbacks[EntityTypeContext.TORUS] == null) {
            handleCallbacks[EntityTypeContext.TORUS] = mgfHandlerFromType(context, HandlerRoleContext.ENTITY_TORUS);
            iNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.MGF_NORMAL) | (1L << EntityTypeContext.VERTEX);
        } else {
            uNeed |= (1L << EntityTypeContext.MGF_POINT) | (1L << EntityTypeContext.MGF_NORMAL) | (1L << EntityTypeContext.VERTEX) | (1L << EntityTypeContext.TRANSFORM);
        }
        if (handleCallbacks[EntityTypeContext.FACE] == null) {
            handleCallbacks[EntityTypeContext.FACE] = handleCallbacks[EntityTypeContext.FACE_WITH_HOLES];
        } else if (handleCallbacks[EntityTypeContext.FACE_WITH_HOLES] == null) {
            handleCallbacks[EntityTypeContext.FACE_WITH_HOLES] = mgfHandlerFromType(context, HandlerRoleContext.ENTITY_FACE_WITH_HOLES);
        }
        if (handleCallbacks[EntityTypeContext.COLOR] != null) {
            if (handleCallbacks[EntityTypeContext.C_MIX] == null) {
                handleCallbacks[EntityTypeContext.C_MIX] = mgfHandlerFromType(context, HandlerRoleContext.COLOR_MIX_HELPER);
                iNeed |= (1L << EntityTypeContext.COLOR) | (1L << EntityTypeContext.CXY) | (1L << EntityTypeContext.C_SPEC) | (1L << EntityTypeContext.C_MIX) | (1L << EntityTypeContext.CCT);
            }
            if (handleCallbacks[EntityTypeContext.C_SPEC] == null) {
                handleCallbacks[EntityTypeContext.C_SPEC] = mgfHandlerFromType(context, HandlerRoleContext.COLOR_SPEC_HELPER);
                iNeed |= (1L << EntityTypeContext.COLOR) | (1L << EntityTypeContext.CXY) | (1L << EntityTypeContext.C_SPEC) | (1L << EntityTypeContext.C_MIX) | (1L << EntityTypeContext.CCT);
            }
            if (handleCallbacks[EntityTypeContext.CCT] == null) {
                handleCallbacks[EntityTypeContext.CCT] = mgfHandlerFromType(context, HandlerRoleContext.COLOR_TEMPERATURE_HELPER);
                iNeed |= (1L << EntityTypeContext.COLOR) | (1L << EntityTypeContext.CXY) | (1L << EntityTypeContext.C_SPEC) | (1L << EntityTypeContext.C_MIX) | (1L << EntityTypeContext.CCT);
            }
        }

        // Add support as needed
        if ((iNeed & (1L << EntityTypeContext.VERTEX)) != 0 && mgfHandlerMatches(handleCallbacks[EntityTypeContext.VERTEX], HandlerRoleContext.HANDLE_VERTEX) == false) {
            context.readerStackState.supportCallbacks[EntityTypeContext.VERTEX] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
        }
        if ((iNeed & (1L << EntityTypeContext.MGF_POINT)) != 0 && mgfHandlerMatches(handleCallbacks[EntityTypeContext.MGF_POINT], HandlerRoleContext.HANDLE_VERTEX) == false) {
            context.readerStackState.supportCallbacks[EntityTypeContext.MGF_POINT] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
        }
        if ((iNeed & (1L << EntityTypeContext.MGF_NORMAL)) != 0 && mgfHandlerMatches(handleCallbacks[EntityTypeContext.MGF_NORMAL], HandlerRoleContext.HANDLE_VERTEX) == false) {
            context.readerStackState.supportCallbacks[EntityTypeContext.MGF_NORMAL] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
        }
        if ((iNeed & (1L << EntityTypeContext.COLOR)) != 0 && mgfHandlerMatches(handleCallbacks[EntityTypeContext.COLOR], HandlerRoleContext.HANDLE_COLOR) == false) {
            context.readerStackState.supportCallbacks[EntityTypeContext.COLOR] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
        }
        if ((iNeed & (1L << EntityTypeContext.CXY)) != 0 && mgfHandlerMatches(handleCallbacks[EntityTypeContext.CXY], HandlerRoleContext.HANDLE_COLOR) == false) {
            context.readerStackState.supportCallbacks[EntityTypeContext.CXY] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
        }
        if ((iNeed & (1L << EntityTypeContext.C_SPEC)) != 0 && mgfHandlerMatches(handleCallbacks[EntityTypeContext.C_SPEC], HandlerRoleContext.HANDLE_COLOR) == false) {
            context.readerStackState.supportCallbacks[EntityTypeContext.C_SPEC] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
        }
        if ((iNeed & (1L << EntityTypeContext.C_MIX)) != 0 && mgfHandlerMatches(handleCallbacks[EntityTypeContext.C_MIX], HandlerRoleContext.HANDLE_COLOR) == false) {
            context.readerStackState.supportCallbacks[EntityTypeContext.C_MIX] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
        }
        if ((iNeed & (1L << EntityTypeContext.CCT)) != 0 && mgfHandlerMatches(handleCallbacks[EntityTypeContext.CCT], HandlerRoleContext.HANDLE_COLOR) == false) {
            context.readerStackState.supportCallbacks[EntityTypeContext.CCT] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
        }

        // Discard remaining entities
        for (int i = 0; i < EntityTypeContext.TOTAL_NUMBER_OF_ENTITIES; i++) {
            if (handleCallbacks[i] == null) {
                handleCallbacks[i] = mgfHandlerFromType(context, HandlerRoleContext.DISCARD_UNNEEDED);
            }
        }
    }

    private static void initMgf(ParseRuntimeContext context) {
        // Related to ColorContext
        context.readerStackState.handleCallbacks[EntityTypeContext.COLOR] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
        context.readerStackState.handleCallbacks[EntityTypeContext.CXY] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);
        context.readerStackState.handleCallbacks[EntityTypeContext.C_MIX] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_COLOR);

        // Related to MgfMaterialContext
        context.readerStackState.handleCallbacks[EntityTypeContext.MGF_MATERIAL] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
        context.readerStackState.handleCallbacks[EntityTypeContext.ED] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
        context.readerStackState.handleCallbacks[EntityTypeContext.IR] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
        context.readerStackState.handleCallbacks[EntityTypeContext.RD] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
        context.readerStackState.handleCallbacks[EntityTypeContext.RS] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
        context.readerStackState.handleCallbacks[EntityTypeContext.SIDES] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
        context.readerStackState.handleCallbacks[EntityTypeContext.TD] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);
        context.readerStackState.handleCallbacks[EntityTypeContext.TS] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_MATERIAL);

        // Related to TransformStackContext
        context.readerStackState.handleCallbacks[EntityTypeContext.TRANSFORM] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_TRANSFORM);

        // Related to object, no explicit context
        context.readerStackState.handleCallbacks[EntityTypeContext.OBJECT] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_OBJECT);

        // Related to geometry elements, no explicit context
        context.readerStackState.handleCallbacks[EntityTypeContext.FACE] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_FACE);
        context.readerStackState.handleCallbacks[EntityTypeContext.FACE_WITH_HOLES] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_FACE_WITH_HOLES);
        context.readerStackState.handleCallbacks[EntityTypeContext.VERTEX] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
        context.readerStackState.handleCallbacks[EntityTypeContext.MGF_POINT] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
        context.readerStackState.handleCallbacks[EntityTypeContext.MGF_NORMAL] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_VERTEX);
        context.readerStackState.handleCallbacks[EntityTypeContext.SPHERE] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);
        context.readerStackState.handleCallbacks[EntityTypeContext.TORUS] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);
        context.readerStackState.handleCallbacks[EntityTypeContext.RING] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);
        context.readerStackState.handleCallbacks[EntityTypeContext.CYLINDER] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);
        context.readerStackState.handleCallbacks[EntityTypeContext.CONE] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);
        context.readerStackState.handleCallbacks[EntityTypeContext.PRISM] = mgfHandlerFromType(context, HandlerRoleContext.HANDLE_SURFACE);

        mgfAlternativeInit(context.readerStackState.handleCallbacks, context);
    }

    private static ParseSnapshotContext mgfBuildModel(ParseRuntimeContext context) {
        if (context == null) {
            return null;
        }

        if (context.model == null) {
            context.model = new ParseSnapshotContext();
        }

        ParseSnapshotContext model = context.model;
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
Reads in a mgf file. The result is that the attributes
context->geometries and context->materials are filled in, and a ParseSnapshotContext
snapshot with parser outputs/state pointers is returned.

Note: this is an implementation of MGF file format with major version number 2.
    */
    public static ParseSnapshotContext readMgf(String filename, ParseRuntimeContext context) {
        mgfSetNrQuartCircDivs(context.numberOfQuarterCircleDivisions);
        mgfSetMonochrome(context.monochrome, context);

        initMgf(context);

        context.currentGeometryList = new ArrayList<Geometry>();
        context.geometryBuildState.currentGeometryList = context.currentGeometryList;

        if (context.materials == null) {
            context.materials = new ArrayList<Material>();
            context.materialState.materials = context.materials;
        }

        context.geometryStackHeadIndex = 0;
        context.geometryBuildState.geometryStackHeadIndex = 0;

        context.inComplex = false;
        context.inSurface = false;
        context.geometryBuildState.inComplex = false;
        context.geometryBuildState.inSurface = false;

        MgfObjectNameSupport.mgfObjectNewSurface(context);

        ReaderContext mgfReaderContext = new ReaderContext();
        int status;
        if (filename != null && filename.length() > 0 && filename.charAt(0) == '#') {
            status = MgfEntityControl.mgfOpen(mgfReaderContext, null, context);
        } else {
            status = MgfEntityControl.mgfOpen(mgfReaderContext, filename, context);
        }
        if (status != 0) {
            MgfEntityControl.doError(context.errorCodeMessages[status], context);
        } else {
            while (mgfReadNextLine(context) > 0 && status == 0) {
                status = mgfParseCurrentLine(context);
                if (status != 0) {
                    MgfEntityControl.doError(context.errorCodeMessages[status], context);
                }
            }
            MgfEntityControl.mgfClose(context);
        }
        mgfClear(context);

        if (context.inSurface) {
            MgfObjectNameSupport.mgfObjectSurfaceDone(context);
        }
        context.geometries = context.currentGeometryList;
        context.geometryBuildState.geometries = context.geometries;

        return mgfBuildModel(context);
    }

    public static void mgfFreeMemory(ParseRuntimeContext context) {
        if (context.currentGeometryList != null) {
            long surfaces = 0;
            long patchSets = 0;
            long compounds = 0;
            long compoundChildren = 0;
            long innerCompoundChildren = 0;
            long unknowns = 0;
            for (int i = 0; i < context.currentGeometryList.size(); i++) {
                Geometry geometry = context.currentGeometryList.get(i);
                if (geometry.className == GeometryClassId.SURFACE_MESH) {
                    surfaces++;
                } else if (geometry.className == GeometryClassId.PATCH_SET) {
                    patchSets++;
                } else if (geometry.className == GeometryClassId.COMPOUND) {
                    Compound compound = (Compound)geometry;
                    if (compound.children != null) {
                        compoundChildren += compound.children.size();
                    }
                    compounds++;
                } else {
                    unknowns++;
                }
            }
            System.out.printf("  - MeshSurfaces: %d\n", surfaces);
            System.out.printf("  - Patch sets: %d\n", patchSets);
            System.out.printf("  - Compounds: %d\n", compounds);
            System.out.printf("    . Children: %d\n", compoundChildren);
            System.out.printf("    . Inner children: %d\n", innerCompoundChildren);
            System.out.printf("  - Unknowns: %d\n", unknowns);
        }

        if (context.allGeometries != null) {
            context.allGeometries.clear();
        }

        if (context.currentGeometryList != null) {
            context.currentGeometryList.clear();
            context.currentGeometryList = null;
            context.geometries = null;
        }

        if (context.materials != null) {
            context.materials.clear();
            context.materials = null;
        }

        context.currentObjectName = null;

        if (context.model != null) {
            context.model = null;
        }

        MgfObjectNameSupport.mgfObjectFreeMemory(context);
        MgfTransformationSupport.mgfTransformFreeMemory(context);
        MgfEntityControl.mgfLookUpFreeMemory(context);
    }
}
