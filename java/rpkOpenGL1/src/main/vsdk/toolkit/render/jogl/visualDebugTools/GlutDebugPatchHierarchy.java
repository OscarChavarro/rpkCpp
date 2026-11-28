package vsdk.toolkit.render.jogl.visualDebugTools;

import vsdk.toolkit.galerkin.GalerkinRadianceMethod;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.skin.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.render.jogl.GalerkinOpenGLRenderer;
import vsdk.toolkit.render.jogl.Opengl;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.ElementTypes;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class GlutDebugPatchHierarchy {
    private static final double GRAY_DARKEN_FACTOR = 0.42;
    private static final double GRAY_CONTRAST_GAMMA = 1.20;
    private static final double OUTLINE_MIN_GRAY = 0.05;
    private static final double OUTLINE_FROM_SURFACE_FACTOR = 0.65;

    private GlutDebugPatchHierarchy() {
    }

    private static double clamp01(double value) {
        if ( value < 0.0 ) {
            return 0.0;
        }
        if ( value > 1.0 ) {
            return 1.0;
        }
        return value;
    }

    private static double toneMappedGrayAndDarkened(double value01) {
        double adjusted = clamp01(value01);
        adjusted = (double)Math.pow(adjusted, GRAY_CONTRAST_GAMMA);
        adjusted *= GRAY_DARKEN_FACTOR;
        return clamp01(adjusted);
    }

    private static int clampLevel(int level, int maxLevel) {
        if ( level < 0 ) {
            return 0;
        }
        return Math.min(level, maxLevel);
    }

    private static GalerkinElement selectedPatchRoot(Scene scene, int patchIndex) {
        if ( scene == null || scene.patchList == null ) {
            return null;
        }
        if ( patchIndex < 0 || patchIndex >= scene.patchList.size() ) {
            return null;
        }

        Patch patch = scene.patchList.get(patchIndex);
        if ( patch == null || GalerkinRadianceMethod.galerkinState.boundElement(patch) == null ) {
            return null;
        }
        if ( GalerkinRadianceMethod.galerkinState.boundElement(patch).className != ElementTypes.ELEMENT_GALERKIN ) {
            return null;
        }

        return (GalerkinElement)GalerkinRadianceMethod.galerkinState.boundElement(patch);
    }

    private static int maxLevelFromElement(GalerkinElement element) {
        if ( element == null || element.regularSubElements == null ) {
            return 0;
        }

        int maxDepth = 0;
        for ( int i = 0; i < 4; i++ ) {
            if ( !(element.regularSubElements[i] instanceof GalerkinElement) ) {
                continue;
            }
            int childDepth = 1 + maxLevelFromElement((GalerkinElement)element.regularSubElements[i]);
            if ( childDepth > maxDepth ) {
                maxDepth = childDepth;
            }
        }
        return maxDepth;
    }

    private static void renderElementGray(GalerkinElement element, RendererConfiguration renderOptions) {
        if ( element == null || renderOptions == null ) {
            return;
        }

        ToneMappingContext toneMapOptions = element.galerkinState == null ? null : element.galerkinState.toneMapOptions;
        if ( toneMapOptions == null || element.isCluster() ) {
            return;
        }

        Vector3Dd[] vertices = new Vector3Dd[] {
            new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd(),
            new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd()
        };
        int numberOfVertices = element.vertices(vertices);
        if ( numberOfVertices < 3 ) {
            return;
        }

        double grayValue = OUTLINE_MIN_GRAY;

        if ( renderOptions.drawSurfaces ) {
            ColorRgb radianceSample = new ColorRgb();
            if ( element.radiance != null && element.radiance.length > 0 ) {
                radianceSample = new ColorRgb(element.radiance[0].getR(), element.radiance[0].getG(), element.radiance[0].getB());
            }

            if ( element.galerkinState != null
                 && element.galerkinState.useAmbientRadiance != 0
                 && element.patch != null
                 && element.galerkinState.boundElement(element.patch) != null ) {
                ColorRgb ambient = new ColorRgb();
                ambient = element.galerkinState.topLevelElement(element.patch).Rd.multiply(element.galerkinState.ambientRadiance);
                radianceSample = radianceSample.add(ambient);
            }

            ColorRgb rgbColor = new ColorRgb();
            rgbColor = ToneMap.radianceToRgb(radianceSample, toneMapOptions);
            grayValue = toneMappedGrayAndDarkened((double)toneMapOptions.getColorSpace().spectrumLuminance(rgbColor.getR(), rgbColor.getG(), rgbColor.getB()));
            Opengl.openGlRenderSetColor(new ColorRgb(grayValue, grayValue, grayValue), renderOptions);
            Opengl.openGlRenderPolygonFlat(numberOfVertices, vertices);
        }

        double outlineGray = grayValue;
        if ( renderOptions.drawSurfaces ) {
            outlineGray *= OUTLINE_FROM_SURFACE_FACTOR;
        }
        else {
            outlineGray = toneMappedGrayAndDarkened((double)toneMapOptions.getColorSpace().spectrumLuminance(
                renderOptions.outlineColor.getR(),
                renderOptions.outlineColor.getG(),
                renderOptions.outlineColor.getB()));
        }
        outlineGray = clamp01(outlineGray);
        if ( outlineGray < OUTLINE_MIN_GRAY ) {
            outlineGray = OUTLINE_MIN_GRAY;
        }

        Opengl.openGlRenderSetColor(new ColorRgb(outlineGray, outlineGray, outlineGray), renderOptions);
        for ( int i = 0; i < numberOfVertices; i++ ) {
            int nextIndex = (i + 1) % numberOfVertices;
            Opengl.openGlRenderLine(vertices[i], vertices[nextIndex]);
        }
    }

    private static void renderNonSelectedPatchesGray(Scene scene, RendererConfiguration renderOptions, int primaryPatchIndex, int secondaryPatchIndex) {
        if ( scene == null || renderOptions == null || scene.patchList == null ) {
            return;
        }

        for ( int patchIndex = 0; patchIndex < scene.patchList.size(); patchIndex++ ) {
            if ( patchIndex == primaryPatchIndex || patchIndex == secondaryPatchIndex ) {
                continue;
            }

            Patch patch = scene.patchList.get(patchIndex);
            if ( patch == null || GalerkinRadianceMethod.galerkinState.boundElement(patch) == null ) {
                continue;
            }
            if ( GalerkinRadianceMethod.galerkinState.boundElement(patch).className != ElementTypes.ELEMENT_GALERKIN ) {
                continue;
            }

            renderElementGray((GalerkinElement)GalerkinRadianceMethod.galerkinState.boundElement(patch), renderOptions);
        }
    }

    private static void renderElementAtLevel(GalerkinElement element, int hierarchyLevel, RendererConfiguration renderOptions) {
        if ( element == null || renderOptions == null ) {
            return;
        }

        if ( hierarchyLevel <= 0 || element.regularSubElements == null ) {
            GalerkinOpenGLRenderer.drawElement(element, GalerkinElement.renderMode(renderOptions), renderOptions);
            return;
        }

        for ( int i = 0; i < 4; i++ ) {
            if ( element.regularSubElements[i] instanceof GalerkinElement ) {
                renderElementAtLevel((GalerkinElement)element.regularSubElements[i], hierarchyLevel - 1, renderOptions);
            }
        }
    }

    private static void drawSelectedPatchCenterMarker(GalerkinElement topLevelElement, RendererConfiguration renderOptions) {
        if ( topLevelElement == null || renderOptions == null ) {
            return;
        }

        Vector3Dd[] vertices = new Vector3Dd[] {
            new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd(),
            new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd()
        };
        int numberOfVertices = topLevelElement.vertices(vertices);
        if ( numberOfVertices < 3 ) {
            return;
        }

        Vector3Dd axisU = new Vector3Dd();
        axisU = vertices[1].subtract(vertices[0]);
        if ( axisU.lengthSquared() < Numeric.EPSILON ) {
            axisU = vertices[2].subtract(vertices[0]);
        }
        if ( axisU.lengthSquared() < Numeric.EPSILON ) {
            axisU = new Vector3Dd(1.0, 0.0, 0.0);
        }
        else {
            axisU = axisU.normalized(Numeric.EPSILON);
        }

        Vector3Dd normal = new Vector3Dd();
        if ( topLevelElement.patch != null ) {
            normal = topLevelElement.patch.getNormal();
        }
        else {
            Vector3Dd edgeA = new Vector3Dd();
            Vector3Dd edgeB = new Vector3Dd();
            edgeA = vertices[1].subtract(vertices[0]);
            edgeB = vertices[2].subtract(vertices[0]);
            normal = edgeA.crossProduct(edgeB);
        }

        if ( normal.lengthSquared() < Numeric.EPSILON ) {
            normal = new Vector3Dd(0.0, 0.0, 1.0);
        }
        else {
            normal = normal.normalized(Numeric.EPSILON);
        }

        Vector3Dd axisV = new Vector3Dd();
        axisV = normal.crossProduct(axisU);
        if ( axisV.lengthSquared() < Numeric.EPSILON ) {
            axisV = new Vector3Dd(0.0, 1.0, 0.0);
        }
        else {
            axisV = axisV.normalized(Numeric.EPSILON);
        }

        double averageEdgeSize = 0.0;
        for ( int i = 0; i < numberOfVertices; i++ ) {
            int nextIndex = (i + 1) % numberOfVertices;
            averageEdgeSize += vertices[i].distance(vertices[nextIndex]);
        }
        averageEdgeSize /= numberOfVertices;
        if ( averageEdgeSize < Numeric.EPSILON ) {
            averageEdgeSize = 0.1;
        }

        double halfSize = 0.08 * averageEdgeSize;
        Vector3Dd center = topLevelElement.midPoint();
        Vector3Dd normalOffset = new Vector3Dd();
        normalOffset = normal.multiply(0.002 * averageEdgeSize);

        Vector3Dd[] marker = new Vector3Dd[] {new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd()};
        marker[0] = center.add(axisU.multiply(-halfSize)).add(axisV.multiply(-halfSize));
        marker[1] = center.add(axisU.multiply(halfSize)).add(axisV.multiply(-halfSize));
        marker[2] = center.add(axisU.multiply(halfSize)).add(axisV.multiply(halfSize));
        marker[3] = center.add(axisU.multiply(-halfSize)).add(axisV.multiply(halfSize));
        for ( int i = 0; i < marker.length; i++ ) {
            marker[i] = marker[i].add(normalOffset);
        }

        Opengl.openGlRenderSetColor(new ColorRgb(1.0, 1.0, 0.0), renderOptions);
        Opengl.openGlRenderLine(marker[0], marker[1]);
        Opengl.openGlRenderLine(marker[1], marker[2]);
        Opengl.openGlRenderLine(marker[2], marker[3]);
        Opengl.openGlRenderLine(marker[3], marker[0]);
    }

    private static void drawSecondarySelectedPatchMarker(GalerkinElement topLevelElement, RendererConfiguration renderOptions) {
        if ( topLevelElement == null || renderOptions == null || topLevelElement.isCluster() ) {
            return;
        }

        RendererConfiguration secondary = new RendererConfiguration();
        secondary.outlineColor = new ColorRgb(1.0, 1.0, 0.0);
        secondary.boundingBoxColor = new ColorRgb(renderOptions.boundingBoxColor.getR(), renderOptions.boundingBoxColor.getG(), renderOptions.boundingBoxColor.getB());
        secondary.clusterColor = new ColorRgb(renderOptions.clusterColor.getR(), renderOptions.clusterColor.getG(), renderOptions.clusterColor.getB());
        secondary.lineWidth = renderOptions.lineWidth;
        secondary.drawOutlines = true;
        secondary.drawSurfaces = true;
        secondary.noShading = renderOptions.noShading;
        secondary.smoothShading = renderOptions.smoothShading;
        secondary.backfaceCulling = renderOptions.backfaceCulling;
        secondary.drawBoundingBoxes = renderOptions.drawBoundingBoxes;
        secondary.drawClusters = renderOptions.drawClusters;
        secondary.frustumCulling = renderOptions.frustumCulling;
        secondary.renderRayTracedImage = renderOptions.renderRayTracedImage;
        secondary.trace = renderOptions.trace;

        renderElementAtLevel(topLevelElement, 0, secondary);
    }

    public static int maxLevelForSelectedPatch(Scene scene, int patchIndex) {
        return maxLevelFromElement(selectedPatchRoot(scene, patchIndex));
    }

    public static void renderSelectedPatchAtLevel(
        Scene scene,
        RendererConfiguration renderOptions,
        int primaryPatchIndex,
        int secondaryPatchIndex,
        int hierarchyLevel)
    {
        if ( renderOptions == null ) {
            return;
        }

        renderNonSelectedPatchesGray(scene, renderOptions, primaryPatchIndex, secondaryPatchIndex);

        GalerkinElement primaryTopLevelElement = selectedPatchRoot(scene, primaryPatchIndex);
        if ( primaryTopLevelElement != null ) {
            int maxLevel = maxLevelFromElement(primaryTopLevelElement);
            int clampedLevel = clampLevel(hierarchyLevel, maxLevel);

            RendererConfiguration selected = new RendererConfiguration();
            selected.outlineColor = new ColorRgb(renderOptions.outlineColor.getR(), renderOptions.outlineColor.getG(), renderOptions.outlineColor.getB());
            selected.boundingBoxColor = new ColorRgb(renderOptions.boundingBoxColor.getR(), renderOptions.boundingBoxColor.getG(), renderOptions.boundingBoxColor.getB());
            selected.clusterColor = new ColorRgb(renderOptions.clusterColor.getR(), renderOptions.clusterColor.getG(), renderOptions.clusterColor.getB());
            selected.lineWidth = renderOptions.lineWidth;
            selected.drawOutlines = true;
            selected.drawSurfaces = true;
            selected.noShading = renderOptions.noShading;
            selected.smoothShading = renderOptions.smoothShading;
            selected.backfaceCulling = renderOptions.backfaceCulling;
            selected.drawBoundingBoxes = renderOptions.drawBoundingBoxes;
            selected.drawClusters = renderOptions.drawClusters;
            selected.frustumCulling = renderOptions.frustumCulling;
            selected.renderRayTracedImage = renderOptions.renderRayTracedImage;
            selected.trace = renderOptions.trace;

            renderElementAtLevel(primaryTopLevelElement, clampedLevel, selected);
            drawSelectedPatchCenterMarker(primaryTopLevelElement, renderOptions);
        }
    }

    public static void renderSecondarySelectedPatchMarker(Scene scene, RendererConfiguration renderOptions, int secondaryPatchIndex) {
        if ( scene == null || renderOptions == null ) {
            return;
        }
        GalerkinElement secondaryTopLevelElement = selectedPatchRoot(scene, secondaryPatchIndex);
        drawSecondarySelectedPatchMarker(secondaryTopLevelElement, renderOptions);
    }
}
