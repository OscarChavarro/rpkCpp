package vsdk.toolkit.app.options;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;

public final class OptionsGroupRender {
    private static int trueValue = 1;
    private static RenderOptions renderOptionsState = new RenderOptions();
    private static ColorRgb outlineColor = new ColorRgb();

    private static void flatOption(TypedOption.MutableValue<Integer> value) {
        renderOptionsState.smoothShading = false;
    }

    private static void noCullingOption(TypedOption.MutableValue<Integer> value) {
        renderOptionsState.backfaceCulling = false;
    }

    private static void outlinesOption(TypedOption.MutableValue<Integer> value) {
        renderOptionsState.drawOutlines = true;
    }

    private static void traceOption(TypedOption.MutableValue<Integer> value) {
        renderOptionsState.trace = true;
    }

    private static boolean parseColor3(
        int argc,
        String[] argv,
        TypedOption.MutableValue<ColorRgb> value)
    {
        if (argc < 3 || argv == null || argv[0] == null || argv[1] == null || argv[2] == null) {
            return false;
        }
        try {
            value.value.r = Float.parseFloat(argv[0]);
            value.value.g = Float.parseFloat(argv[1]);
            value.value.b = Float.parseFloat(argv[2]);
            return true;
        }
        catch (NumberFormatException e) {
            return false;
        }
    }

    private static void copyFrom(RenderOptions source, RenderOptions target) {
        if (source == null || target == null) {
            return;
        }

        target.outlineColor = new ColorRgb(source.outlineColor.r, source.outlineColor.g, source.outlineColor.b);
        target.boundingBoxColor = new ColorRgb(source.boundingBoxColor.r, source.boundingBoxColor.g, source.boundingBoxColor.b);
        target.clusterColor = new ColorRgb(source.clusterColor.r, source.clusterColor.g, source.clusterColor.b);
        target.lineWidth = source.lineWidth;
        target.drawOutlines = source.drawOutlines;
        target.drawSurfaces = source.drawSurfaces;
        target.noShading = source.noShading;
        target.smoothShading = source.smoothShading;
        target.backfaceCulling = source.backfaceCulling;
        target.drawBoundingBoxes = source.drawBoundingBoxes;
        target.drawClusters = source.drawClusters;
        target.frustumCulling = source.frustumCulling;
        target.renderRayTracedImage = source.renderRayTracedImage;
        target.trace = source.trace;
    }

    public static void renderParseOptions(
        int[] argc,
        String[] argv,
        RenderOptions renderOptions)
    {
        TypedOption<Integer> flatOpt = new TypedOption<>(
            "-flat-shading",
            TypedOption.valueRef(trueValue),
            0,
            OptionsGroupRender::flatOption,
            null);
        TypedOption<Integer> raycastOpt = new TypedOption<>(
            "-raycast",
            TypedOption.valueRef(trueValue),
            0,
            OptionsGroupRender::traceOption,
            null);
        TypedOption<Integer> noCullingOpt = new TypedOption<>(
            "-no-culling",
            TypedOption.valueRef(trueValue),
            0,
            OptionsGroupRender::noCullingOption,
            null);
        TypedOption<Integer> outlinesOpt = new TypedOption<>(
            "-outlines",
            TypedOption.valueRef(trueValue),
            0,
            OptionsGroupRender::outlinesOption,
            null);
        TypedOption<ColorRgb> outlineColorOpt = new TypedOption<>(
            "-outline-color",
            TypedOption.valueRef(outlineColor),
            3,
            null,
            OptionsGroupRender::parseColor3);
        OptionBase[] renderingOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(flatOpt, 5),
            TypedOption.REGISTER_OPTION(raycastOpt, 5),
            TypedOption.REGISTER_OPTION(noCullingOpt, 5),
            TypedOption.REGISTER_OPTION(outlinesOpt, 5),
            TypedOption.REGISTER_OPTION(outlineColorOpt, 10)
        };

        copyFrom(renderOptions, renderOptionsState);
        OptionGroup[] renderGroups = new OptionGroup[] {
            new OptionGroup("render", renderingOptions, 5)
        };
        OptionParser.parse(argc, argv, renderGroups, 1);

        copyFrom(renderOptionsState, renderOptions);
        renderOptions.outlineColor.r = outlineColor.r;
        renderOptions.outlineColor.g = outlineColor.g;
        renderOptions.outlineColor.b = outlineColor.b;
    }
}
