package vsdk.toolkit.render.jogl;

import java.util.ArrayList;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;
import vsdk.toolkit.skin.RpkGeometry;

public final class RenderOpenGL {
    private RenderOpenGL() {
    }

    private static void renderGeomBounds(Camera camera, RpkGeometry geometry) {
        if ( geometry == null ) {
            return;
        }
        RpkAxisAlignedBoundingBox geometryBoundingBox = geometry.getBoundingBox();
        if ( geometry.bounded ) {
            renderBoundingBox(geometryBoundingBox);
        }

        if ( geometry.isCompound() ) {
            ArrayList<RpkGeometry> geometryList = RpkGeometry.primitiveListCopy(geometry);
            for ( int i = 0; geometryList != null && i < geometryList.size(); i++ ) {
                renderGeomBounds(camera, geometryList.get(i));
            }
        }
    }

    public static void renderGetNearFar(Camera camera, ArrayList<RpkGeometry> sceneGeometries) {
        RpkAxisAlignedBoundingBox bounds = new RpkAxisAlignedBoundingBox();

        if ( sceneGeometries == null || sceneGeometries.isEmpty() ) {
            camera.far = 10.0;
            camera.near = 0.1;
            return;
        }

        RpkGeometry.listBounds(sceneGeometries, bounds);
        camera.fitClippingPlanes(bounds);
    }

    public static void renderBoundingBox(RpkAxisAlignedBoundingBox boundingBox) {
        if ( boundingBox == null ) {
            return;
        }

        Vector3Dd[] p = new Vector3Dd[8];
        for ( int i = 0; i < p.length; i++ ) {
            p[i] = new Vector3Dd();
        }

        double minX = boundingBox.minX();
        double minY = boundingBox.minY();
        double minZ = boundingBox.minZ();
        double maxX = boundingBox.maxX();
        double maxY = boundingBox.maxY();
        double maxZ = boundingBox.maxZ();

        p[0] = new Vector3Dd(minX, minY, minZ);
        p[1] = new Vector3Dd(maxX, minY, minZ);
        p[2] = new Vector3Dd(minX, maxY, minZ);
        p[3] = new Vector3Dd(maxX, maxY, minZ);
        p[4] = new Vector3Dd(minX, minY, maxZ);
        p[5] = new Vector3Dd(maxX, minY, maxZ);
        p[6] = new Vector3Dd(minX, maxY, maxZ);
        p[7] = new Vector3Dd(maxX, maxY, maxZ);

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

    public static void renderBoundingBoxHierarchy(Camera camera, ArrayList<RpkGeometry> sceneGeometries, RendererConfiguration renderOptions) {
        if ( renderOptions == null ) {
            return;
        }
        Opengl.openGlRenderSetColor(renderOptions.boundingBoxColor, renderOptions);
        for ( int i = 0; sceneGeometries != null && i < sceneGeometries.size(); i++ ) {
            renderGeomBounds(camera, sceneGeometries.get(i));
        }
    }

    public static void renderClusterHierarchy(Camera camera, ArrayList<RpkGeometry> clusteredGeometryList, RendererConfiguration renderOptions) {
        if ( renderOptions == null ) {
            return;
        }
        Opengl.openGlRenderSetColor(renderOptions.clusterColor, renderOptions);
        for ( int i = 0; clusteredGeometryList != null && i < clusteredGeometryList.size(); i++ ) {
            renderGeomBounds(camera, clusteredGeometryList.get(i));
        }
    }
}
