package vsdk.toolkit.render.jogl;

import java.util.ArrayList;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Geometry;

public final class RenderOpenGL {
    private RenderOpenGL() {
    }

    private static void renderGeomBounds(Camera camera, Geometry geometry) {
        if ( geometry == null ) {
            return;
        }
        BoundingBox geometryBoundingBox = geometry.getBoundingBox();
        if ( geometry.bounded ) {
            renderBoundingBox(geometryBoundingBox);
        }

        if ( geometry.isCompound() ) {
            ArrayList<Geometry> geometryList = Geometry.primitiveListCopy(geometry);
            for ( int i = 0; geometryList != null && i < geometryList.size(); i++ ) {
                renderGeomBounds(camera, geometryList.get(i));
            }
        }
    }

    public static void renderGetNearFar(Camera camera, ArrayList<Geometry> sceneGeometries) {
        BoundingBox bounds = new BoundingBox();

        if ( sceneGeometries == null || sceneGeometries.isEmpty() ) {
            camera.far = 10.0F;
            camera.near = 0.1F;
            return;
        }

        Geometry.listBounds(sceneGeometries, bounds);
        camera.fitClippingPlanes(bounds);
    }

    public static void renderBoundingBox(BoundingBox boundingBox) {
        if ( boundingBox == null ) {
            return;
        }

        Vector3Df[] p = new Vector3Df[8];
        for ( int i = 0; i < p.length; i++ ) {
            p[i] = new Vector3Df();
        }

        float minX = boundingBox.minX();
        float minY = boundingBox.minY();
        float minZ = boundingBox.minZ();
        float maxX = boundingBox.maxX();
        float maxY = boundingBox.maxY();
        float maxZ = boundingBox.maxZ();

        p[0] = new Vector3Df(minX, minY, minZ);
        p[1] = new Vector3Df(maxX, minY, minZ);
        p[2] = new Vector3Df(minX, maxY, minZ);
        p[3] = new Vector3Df(maxX, maxY, minZ);
        p[4] = new Vector3Df(minX, minY, maxZ);
        p[5] = new Vector3Df(maxX, minY, maxZ);
        p[6] = new Vector3Df(minX, maxY, maxZ);
        p[7] = new Vector3Df(maxX, maxY, maxZ);

        Opengl.openGlRenderLine(p[0], p[1]);
        Opengl.openGlRenderLine(p[1], p[3]);
        Opengl.openGlRenderLine(p[3], p[2]);
        Opengl.openGlRenderLine(p[2], p[0]);
        Opengl.openGlRenderLine(p[4], p[5]);
        Opengl.openGlRenderLine(p[5], p[7]);
        Opengl.openGlRenderLine(p[7], p[6]);
        Opengl.openGlRenderLine(p[6], p[4]);
        Opengl.openGlRenderLine(p[0], p[4]);
        Opengl.openGlRenderLine(p[1], p[5]);
        Opengl.openGlRenderLine(p[2], p[6]);
        Opengl.openGlRenderLine(p[3], p[7]);
    }

    public static void renderBoundingBoxHierarchy(Camera camera, ArrayList<Geometry> sceneGeometries, RenderOptions renderOptions) {
        if ( renderOptions == null ) {
            return;
        }
        Opengl.openGlRenderSetColor(renderOptions.boundingBoxColor, renderOptions);
        for ( int i = 0; sceneGeometries != null && i < sceneGeometries.size(); i++ ) {
            renderGeomBounds(camera, sceneGeometries.get(i));
        }
    }

    public static void renderClusterHierarchy(Camera camera, ArrayList<Geometry> clusteredGeometryList, RenderOptions renderOptions) {
        if ( renderOptions == null ) {
            return;
        }
        Opengl.openGlRenderSetColor(renderOptions.clusterColor, renderOptions);
        for ( int i = 0; clusteredGeometryList != null && i < clusteredGeometryList.size(); i++ ) {
            renderGeomBounds(camera, clusteredGeometryList.get(i));
        }
    }
}
