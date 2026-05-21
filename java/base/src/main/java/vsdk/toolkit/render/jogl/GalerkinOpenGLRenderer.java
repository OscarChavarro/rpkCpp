package vsdk.toolkit.render.jogl;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.galerkin.GalerkinBasis;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinElementRenderMode;
import vsdk.toolkit.render.jogl.visualDebugTools.GlutDebugState;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class GalerkinOpenGLRenderer {
    private GalerkinOpenGLRenderer() {
    }

    public static void galerkinRenderPatch(Patch patch, Camera camera, RenderOptions renderOptions) {
        if ( patch == null ) {
            return;
        }
        renderElementHierarchy(GalerkinElement.fromPatch(patch), renderOptions);
    }

    public static void renderElementHierarchy(GalerkinElement element, RenderOptions renderOptions) {
        if ( element == null || renderOptions == null ) {
            return;
        }

        if ( element.regularSubElements == null ) {
            drawElement(element, GalerkinElement.renderMode(renderOptions), renderOptions);
            return;
        }

        for ( int i = 0; i < 4; i++ ) {
            if ( element.regularSubElements[i] instanceof GalerkinElement ) {
                renderElementHierarchy((GalerkinElement)element.regularSubElements[i], renderOptions);
            }
        }
    }

    public static void drawElement(GalerkinElement element, int mode, RenderOptions renderOptions) {
        if ( element == null || renderOptions == null ) {
            return;
        }

        if ( element.isCluster() ) {
            if ( (mode & GalerkinElementRenderMode.OUTLINE.value) != 0 ) {
                RenderOpenGL.renderBoundingBox(element.geometry.getBoundingBox());
            }
            return;
        }

        Vector3D[] p = new Vector3D[] {
            new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(),
            new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D()
        };
        int numberOfVertices = element.vertices(p);

        if ( renderOptions.drawSurfaces ) {
            ToneMappingContext toneMapOptions =
                element.galerkinState == null ? null : element.galerkinState.toneMapOptions;
            if ( toneMapOptions == null ) {
                return;
            }

            if ( (mode & GalerkinElementRenderMode.GOURAUD.value) == 0 ) {
                ColorRgb color = new ColorRgb();
                ColorRgb rho = element.patch.radianceData.Rd;

                if ( element.galerkinState != null && element.galerkinState.useAmbientRadiance != 0 ) {
                    ColorRgb radVis = new ColorRgb();
                    radVis.scalarProduct(rho, element.galerkinState.ambientRadiance);
                    radVis.add(radVis, element.radiance[0]);
                    ToneMap.radianceToRgb(radVis, color, toneMapOptions);
                }
                else {
                    ToneMap.radianceToRgb(element.radiance[0], color, toneMapOptions);
                }

                Opengl.openGlRenderSetColor(color, renderOptions);
                Opengl.openGlRenderPolygonFlat(numberOfVertices, p);
            }
            else {
                ColorRgb[] vertexRadiance = new ColorRgb[] {
                    new ColorRgb(), new ColorRgb(), new ColorRgb(), new ColorRgb()
                };

                if ( numberOfVertices == 3 ) {
                    vertexRadiance[0] = GalerkinBasis.radianceAtPoint(element, element.radiance, 0.0, 0.0);
                    vertexRadiance[1] = GalerkinBasis.radianceAtPoint(element, element.radiance, 1.0, 0.0);
                    vertexRadiance[2] = GalerkinBasis.radianceAtPoint(element, element.radiance, 0.0, 1.0);
                }
                else {
                    vertexRadiance[0] = GalerkinBasis.radianceAtPoint(element, element.radiance, 0.0, 0.0);
                    vertexRadiance[1] = GalerkinBasis.radianceAtPoint(element, element.radiance, 1.0, 0.0);
                    vertexRadiance[2] = GalerkinBasis.radianceAtPoint(element, element.radiance, 1.0, 1.0);
                    vertexRadiance[3] = GalerkinBasis.radianceAtPoint(element, element.radiance, 0.0, 1.0);
                }

                if ( element.galerkinState != null && element.galerkinState.useAmbientRadiance != 0 ) {
                    ColorRgb reflectivity = element.patch.radianceData.Rd;
                    ColorRgb ambient = new ColorRgb();
                    ambient.scalarProduct(reflectivity, element.galerkinState.ambientRadiance);
                    for ( int i = 0; i < numberOfVertices; i++ ) {
                        vertexRadiance[i].add(vertexRadiance[i], ambient);
                    }
                }

                ColorRgb[] vertexColors = new ColorRgb[] {
                    new ColorRgb(), new ColorRgb(), new ColorRgb(), new ColorRgb()
                };
                for ( int i = 0; i < numberOfVertices; i++ ) {
                    ToneMap.radianceToRgb(vertexRadiance[i], vertexColors[i], toneMapOptions);
                }
                Opengl.openGlRenderPolygonGouraud(numberOfVertices, p, vertexColors, renderOptions);
            }
        }

        if ( (mode & GalerkinElementRenderMode.OUTLINE.value) != 0 ) {
            Opengl.openGlRenderSetColor(renderOptions.outlineColor, renderOptions);
            if ( numberOfVertices == 3 ) {
                Opengl.openGlRenderLine(p[0], p[1]);
                Opengl.openGlRenderLine(p[1], p[2]);
                Opengl.openGlRenderLine(p[2], p[0]);
            }
            else {
                Opengl.openGlRenderLine(p[0], p[1]);
                Opengl.openGlRenderLine(p[1], p[2]);
                Opengl.openGlRenderLine(p[2], p[3]);
                Opengl.openGlRenderLine(p[3], p[0]);
            }
        }
    }

    public static void renderScene(Scene scene, RenderOptions renderOptions, GlutDebugState debugState) {
        if ( scene == null || renderOptions == null ) {
            return;
        }

        if ( renderOptions.frustumCulling ) {
            Opengl.openGlRenderWorldOctree(scene, GalerkinOpenGLRenderer::galerkinRenderPatch, renderOptions);
            return;
        }

        if ( debugState == null ) {
            for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                galerkinRenderPatch(scene.patchList.get(i), scene.camera, renderOptions);
            }
            return;
        }

        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            RenderOptions modified = new RenderOptions();
            modified.outlineColor = new ColorRgb(renderOptions.outlineColor.getR(), renderOptions.outlineColor.getG(), renderOptions.outlineColor.getB());
            modified.boundingBoxColor = new ColorRgb(renderOptions.boundingBoxColor.getR(), renderOptions.boundingBoxColor.getG(), renderOptions.boundingBoxColor.getB());
            modified.clusterColor = new ColorRgb(renderOptions.clusterColor.getR(), renderOptions.clusterColor.getG(), renderOptions.clusterColor.getB());
            modified.lineWidth = renderOptions.lineWidth;
            modified.drawOutlines = renderOptions.drawOutlines;
            modified.drawSurfaces = renderOptions.drawSurfaces;
            modified.noShading = renderOptions.noShading;
            modified.smoothShading = renderOptions.smoothShading;
            modified.backfaceCulling = renderOptions.backfaceCulling;
            modified.drawBoundingBoxes = renderOptions.drawBoundingBoxes;
            modified.drawClusters = renderOptions.drawClusters;
            modified.frustumCulling = renderOptions.frustumCulling;
            modified.renderRayTracedImage = renderOptions.renderRayTracedImage;
            modified.trace = renderOptions.trace;

            if ( debugState.showSelectedPathOnly ) {
                if ( i == debugState.primarySelectedPatch ) {
                    modified.drawOutlines = true;
                    modified.outlineColor = new ColorRgb(1.0f, 0.0f, 0.0f);
                }
                else {
                    modified.drawOutlines = false;
                }
            }
            else {
                modified.outlineColor = new ColorRgb(0.4f, 0.1f, 0.1f);
                if ( i == debugState.primarySelectedPatch ) {
                    modified.outlineColor = new ColorRgb(0.0f, 0.0f, 1.0f);
                }
            }
            galerkinRenderPatch(scene.patchList.get(i), scene.camera, modified);
        }
    }
}
