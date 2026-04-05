package vsdk.toolkit.render.jogl;

import java.util.ArrayList;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
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
        if ( camera == null ) {
            return;
        }

        if ( sceneGeometries == null || sceneGeometries.isEmpty() ) {
            camera.far = 10.0f;
            camera.near = 0.1f;
            return;
        }

        BoundingBox bounds = new BoundingBox();
        Geometry.listBounds(sceneGeometries, bounds);

        Vector3D minimum = new Vector3D(bounds.minX(), bounds.minY(), bounds.minZ());
        Vector3D maximum = new Vector3D(bounds.maxX(), bounds.maxY(), bounds.maxZ());
        Vector3D d = new Vector3D();

        camera.far = -Numeric.HUGE_FLOAT_VALUE;
        camera.near = Numeric.HUGE_FLOAT_VALUE;

        for ( int i = 0; i <= 1; i++ ) {
            for ( int j = 0; j <= 1; j++ ) {
                for ( int k = 0; k <= 1; k++ ) {
                    d.set(
                        i != 0 ? maximum.x : minimum.x,
                        j != 0 ? maximum.y : minimum.y,
                        k != 0 ? maximum.z : minimum.z);

                    d.subtraction(d, camera.eyePosition);
                    float z = d.dotProduct(camera.Z);

                    if ( z > camera.far ) {
                        camera.far = z;
                    }
                    if ( z < camera.near ) {
                        camera.near = z;
                    }
                }
            }
        }

        camera.far += 0.02f * camera.far;
        camera.near -= 0.02f * camera.near;
        if ( camera.far < Numeric.EPSILON_FLOAT ) {
            camera.far = camera.viewDistance;
        }
        if ( camera.near < Numeric.EPSILON_FLOAT ) {
            camera.near = camera.viewDistance / 100.0f;
        }
    }

    public static void renderBoundingBox(BoundingBox boundingBox) {
        if ( boundingBox == null ) {
            return;
        }

        Vector3D[] p = new Vector3D[8];
        for ( int i = 0; i < p.length; i++ ) {
            p[i] = new Vector3D();
        }

        float minX = boundingBox.minX();
        float minY = boundingBox.minY();
        float minZ = boundingBox.minZ();
        float maxX = boundingBox.maxX();
        float maxY = boundingBox.maxY();
        float maxZ = boundingBox.maxZ();

        p[0].set(minX, minY, minZ);
        p[1].set(maxX, minY, minZ);
        p[2].set(minX, maxY, minZ);
        p[3].set(maxX, maxY, minZ);
        p[4].set(minX, minY, maxZ);
        p[5].set(maxX, minY, maxZ);
        p[6].set(minX, maxY, maxZ);
        p[7].set(maxX, maxY, maxZ);

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
