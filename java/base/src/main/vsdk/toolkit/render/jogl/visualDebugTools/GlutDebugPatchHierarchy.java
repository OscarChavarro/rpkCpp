package vsdk.toolkit.render.jogl.visualDebugTools;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.render.jogl.GalerkinOpenGLRenderer;
import vsdk.toolkit.render.jogl.Opengl;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.ElementTypes;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class GlutDebugPatchHierarchy {
    private static final float GRAY_DARKEN_FACTOR = 0.42f;
    private static final float GRAY_CONTRAST_GAMMA = 1.20f;
    private static final float OUTLINE_MIN_GRAY = 0.05f;
    private static final float OUTLINE_FROM_SURFACE_FACTOR = 0.65f;

    private GlutDebugPatchHierarchy() {
    }

    private static float clamp01(float value) {
        if ( value < 0.0f ) {
            return 0.0f;
        }
        if ( value > 1.0f ) {
            return 1.0f;
        }
        return value;
    }

    private static float toneMappedGrayAndDarkened(float value01) {
        float adjusted = clamp01(value01);
        adjusted = (float)Math.pow(adjusted, GRAY_CONTRAST_GAMMA);
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
        if ( patch == null || patch.radianceData == null ) {
            return null;
        }
        if ( patch.radianceData.className != ElementTypes.ELEMENT_GALERKIN ) {
            return null;
        }

        return (GalerkinElement)patch.radianceData;
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

    private static void renderElementGray(GalerkinElement element, RenderOptions renderOptions) {
        if ( element == null || renderOptions == null ) {
            return;
        }

        ToneMappingContext toneMapOptions = element.galerkinState == null ? null : element.galerkinState.toneMapOptions;
        if ( toneMapOptions == null || element.isCluster() ) {
            return;
        }

        Vector3D[] vertices = new Vector3D[] {
            new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(),
            new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D()
        };
        int numberOfVertices = element.vertices(vertices);
        if ( numberOfVertices < 3 ) {
            return;
        }

        float grayValue = OUTLINE_MIN_GRAY;

        if ( renderOptions.drawSurfaces ) {
            ColorRgb radianceSample = new ColorRgb();
            if ( element.radiance != null && element.radiance.length > 0 ) {
                radianceSample = new ColorRgb(element.radiance[0].getR(), element.radiance[0].getG(), element.radiance[0].getB());
            }

            if ( element.galerkinState != null
                 && element.galerkinState.useAmbientRadiance != 0
                 && element.patch != null
                 && element.patch.radianceData != null ) {
                ColorRgb ambient = new ColorRgb();
                ambient.scalarProduct(element.patch.radianceData.Rd, element.galerkinState.ambientRadiance);
                radianceSample.add(radianceSample, ambient);
            }

            ColorRgb rgbColor = new ColorRgb();
            ToneMap.radianceToRgb(radianceSample, rgbColor, toneMapOptions);
            grayValue = toneMappedGrayAndDarkened(Cie.spectrumLuminance(rgbColor.getR(), rgbColor.getG(), rgbColor.getB()));
            Opengl.openGlRenderSetColor(new ColorRgb(grayValue, grayValue, grayValue), renderOptions);
            Opengl.openGlRenderPolygonFlat(numberOfVertices, vertices);
        }

        float outlineGray = grayValue;
        if ( renderOptions.drawSurfaces ) {
            outlineGray *= OUTLINE_FROM_SURFACE_FACTOR;
        }
        else {
            outlineGray = toneMappedGrayAndDarkened(Cie.spectrumLuminance(
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

    private static void renderNonSelectedPatchesGray(Scene scene, RenderOptions renderOptions, int primaryPatchIndex, int secondaryPatchIndex) {
        if ( scene == null || renderOptions == null || scene.patchList == null ) {
            return;
        }

        for ( int patchIndex = 0; patchIndex < scene.patchList.size(); patchIndex++ ) {
            if ( patchIndex == primaryPatchIndex || patchIndex == secondaryPatchIndex ) {
                continue;
            }

            Patch patch = scene.patchList.get(patchIndex);
            if ( patch == null || patch.radianceData == null ) {
                continue;
            }
            if ( patch.radianceData.className != ElementTypes.ELEMENT_GALERKIN ) {
                continue;
            }

            renderElementGray((GalerkinElement)patch.radianceData, renderOptions);
        }
    }

    private static void renderElementAtLevel(GalerkinElement element, int hierarchyLevel, RenderOptions renderOptions) {
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

    private static void drawSelectedPatchCenterMarker(GalerkinElement topLevelElement, RenderOptions renderOptions) {
        if ( topLevelElement == null || renderOptions == null ) {
            return;
        }

        Vector3D[] vertices = new Vector3D[] {
            new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(),
            new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D()
        };
        int numberOfVertices = topLevelElement.vertices(vertices);
        if ( numberOfVertices < 3 ) {
            return;
        }

        Vector3D axisU = new Vector3D();
        axisU.subtraction(vertices[1], vertices[0]);
        if ( axisU.norm2() < Numeric.EPSILON_FLOAT ) {
            axisU.subtraction(vertices[2], vertices[0]);
        }
        if ( axisU.norm2() < Numeric.EPSILON_FLOAT ) {
            axisU.set(1.0f, 0.0f, 0.0f);
        }
        else {
            axisU.normalize(Numeric.EPSILON_FLOAT);
        }

        Vector3D normal = new Vector3D();
        if ( topLevelElement.patch != null ) {
            normal.copy(topLevelElement.patch.normal);
        }
        else {
            Vector3D edgeA = new Vector3D();
            Vector3D edgeB = new Vector3D();
            edgeA.subtraction(vertices[1], vertices[0]);
            edgeB.subtraction(vertices[2], vertices[0]);
            normal.crossProduct(edgeA, edgeB);
        }

        if ( normal.norm2() < Numeric.EPSILON_FLOAT ) {
            normal.set(0.0f, 0.0f, 1.0f);
        }
        else {
            normal.normalize(Numeric.EPSILON_FLOAT);
        }

        Vector3D axisV = new Vector3D();
        axisV.crossProduct(normal, axisU);
        if ( axisV.norm2() < Numeric.EPSILON_FLOAT ) {
            axisV.set(0.0f, 1.0f, 0.0f);
        }
        else {
            axisV.normalize(Numeric.EPSILON_FLOAT);
        }

        float averageEdgeSize = 0.0f;
        for ( int i = 0; i < numberOfVertices; i++ ) {
            int nextIndex = (i + 1) % numberOfVertices;
            averageEdgeSize += vertices[i].distance(vertices[nextIndex]);
        }
        averageEdgeSize /= numberOfVertices;
        if ( averageEdgeSize < Numeric.EPSILON_FLOAT ) {
            averageEdgeSize = 0.1f;
        }

        float halfSize = 0.08f * averageEdgeSize;
        Vector3D center = topLevelElement.midPoint();
        Vector3D normalOffset = new Vector3D();
        normalOffset.scaledCopy(0.002f * averageEdgeSize, normal);

        Vector3D[] marker = new Vector3D[] {new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D()};
        marker[0].combine3(center, -halfSize, axisU, -halfSize, axisV);
        marker[1].combine3(center, halfSize, axisU, -halfSize, axisV);
        marker[2].combine3(center, halfSize, axisU, halfSize, axisV);
        marker[3].combine3(center, -halfSize, axisU, halfSize, axisV);
        for ( int i = 0; i < marker.length; i++ ) {
            marker[i].addition(marker[i], normalOffset);
        }

        Opengl.openGlRenderSetColor(new ColorRgb(1.0f, 1.0f, 0.0f), renderOptions);
        Opengl.openGlRenderLine(marker[0], marker[1]);
        Opengl.openGlRenderLine(marker[1], marker[2]);
        Opengl.openGlRenderLine(marker[2], marker[3]);
        Opengl.openGlRenderLine(marker[3], marker[0]);
    }

    private static void drawSecondarySelectedPatchMarker(GalerkinElement topLevelElement, RenderOptions renderOptions) {
        if ( topLevelElement == null || renderOptions == null || topLevelElement.isCluster() ) {
            return;
        }

        RenderOptions secondary = new RenderOptions();
        secondary.outlineColor = new ColorRgb(1.0f, 1.0f, 0.0f);
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
        RenderOptions renderOptions,
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

            RenderOptions selected = new RenderOptions();
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

    public static void renderSecondarySelectedPatchMarker(Scene scene, RenderOptions renderOptions, int secondaryPatchIndex) {
        if ( scene == null || renderOptions == null ) {
            return;
        }
        GalerkinElement secondaryTopLevelElement = selectedPatchRoot(scene, secondaryPatchIndex);
        drawSecondarySelectedPatchMarker(secondaryTopLevelElement, renderOptions);
    }
}
