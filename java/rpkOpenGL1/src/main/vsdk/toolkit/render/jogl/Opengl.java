package vsdk.toolkit.render.jogl;

import vsdk.toolkit.environment.geometry.elements.FaceColorTable;
import com.jogamp.opengl.GL;
import com.jogamp.opengl.GL2;
import com.jogamp.opengl.glu.GLU;
import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.render.Canvas;
import vsdk.toolkit.render.OctreeChild;
import vsdk.toolkit.render.RenderHookList;
import vsdk.toolkit.render.jogl.visualDebugTools.GlutDebugState;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.RadianceMethodAlgorithm;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class Opengl {
    private static final ThreadLocal<GL2> CURRENT_GL = new ThreadLocal<>();
    private static final GLU GLU_HELPER = new GLU();

    private static ToneMappingContext activeToneMapOptions = null;
    private static boolean openGlMissingToneMapWarningShown = false;

    private Opengl() {
    }

    public static void setCurrentGl(GL2 gl) {
        CURRENT_GL.set(gl);
    }

    private static GL2 gl() {
        return CURRENT_GL.get();
    }

    public static void openGlRenderClearWindow(Camera camera) {
        GL2 gl = gl();
        if ( gl == null || camera == null ) {
            return;
        }
        gl.glClearColor((float)camera.background.getR(), (float)camera.background.getG(), (float)camera.background.getB(), 0.0f); // OpenGL clears with float colors
        gl.glClearDepth(1.0);
        gl.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT);
    }

    public static void openGlRenderLine(Vector3Dd x, Vector3Dd y) {
        GL2 gl = gl();
        if ( gl == null || x == null || y == null ) {
            return;
        }

        gl.glDisable(GL2.GL_POLYGON_OFFSET_FILL);
        gl.glBegin(GL2.GL_LINES);
        gl.glVertex3d(x.x(), x.y(), x.z());
        gl.glVertex3d(y.x(), y.y(), y.z());
        gl.glEnd();
        gl.glEnable(GL2.GL_POLYGON_OFFSET_FILL);
        gl.glPolygonOffset(1.0f, 1.0f);
    }

    public static void openGlRenderSetColor(ColorRgb rgb, RendererConfiguration renderOptions) {
        GL2 gl = gl();
        if ( gl == null || rgb == null ) {
            return;
        }

        ColorRgb corrected = new ColorRgb(rgb.getR(), rgb.getG(), rgb.getB());
        if ( activeToneMapOptions != null ) {
            ToneMap.toneMappingGammaCorrection(corrected, activeToneMapOptions);
        }
        else if ( !openGlMissingToneMapWarningShown ) {
            Logger.reportMessage(null, Logger.WARNING, "Opengl::openGlRenderSetColor", "Tone mapping context not set in active scene, using uncorrected color");
            openGlMissingToneMapWarningShown = true;
        }
        gl.glColor3d((double)corrected.getR(), (double)corrected.getG(), (double)corrected.getB());
    }

    public static void openGlRenderPolygonFlat(int numberOfVertices, Vector3Dd[] vertices) {
        GL2 gl = gl();
        if ( gl == null || vertices == null || numberOfVertices <= 0 ) {
            return;
        }
        gl.glBegin(GL2.GL_POLYGON);
        for ( int i = 0; i < numberOfVertices; i++ ) {
            gl.glVertex3d(vertices[i].x(), vertices[i].y(), vertices[i].z());
        }
        gl.glEnd();
    }

    public static void openGlRenderPolygonGouraud(int numberOfVertices, Vector3Dd[] vertices, ColorRgb[] verticesColors, RendererConfiguration renderOptions) {
        GL2 gl = gl();
        if ( gl == null || vertices == null || verticesColors == null || numberOfVertices <= 0 ) {
            return;
        }

        gl.glBegin(GL2.GL_POLYGON);
        for ( int i = 0; i < numberOfVertices; i++ ) {
            openGlRenderSetColor(verticesColors[i], renderOptions);
            gl.glVertex3d(vertices[i].x(), vertices[i].y(), vertices[i].z());
        }
        gl.glEnd();
    }

    private static void openGlRenderPatchFlat(Patch patch, ColorRgb color, RendererConfiguration renderOptions) {
        if ( patch == null ) {
            return;
        }
        openGlRenderSetColor(color, renderOptions);
        GL2 gl = gl();
        if ( gl == null ) {
            return;
        }

        int primitive = patch.getNumberOfVertices() == 3
            ? GL2.GL_TRIANGLES
            : (patch.getNumberOfVertices() == 4 ? GL2.GL_QUADS : GL2.GL_POLYGON);
        gl.glBegin(primitive);
        for ( int i = 0; i < patch.getNumberOfVertices(); i++ ) {
            gl.glVertex3d(patch.getVertices()[i].point.x(), patch.getVertices()[i].point.y(), patch.getVertices()[i].point.z());
        }
        gl.glEnd();
    }

    private static void openGlRenderPatchSmooth(Patch patch, RendererConfiguration renderOptions) {
        if ( patch == null ) {
            return;
        }
        GL2 gl = gl();
        if ( gl == null ) {
            return;
        }

        int primitive = patch.getNumberOfVertices() == 3
            ? GL2.GL_TRIANGLES
            : (patch.getNumberOfVertices() == 4 ? GL2.GL_QUADS : GL2.GL_POLYGON);
        gl.glBegin(primitive);
        for ( int i = 0; i < patch.getNumberOfVertices(); i++ ) {
            openGlRenderSetColor(patch.getVertices()[i].color, renderOptions);
            gl.glVertex3d(patch.getVertices()[i].point.x(), patch.getVertices()[i].point.y(), patch.getVertices()[i].point.z());
        }
        gl.glEnd();
    }

    public static void openGlRenderPatchOutline(Patch patch) {
        GL2 gl = gl();
        if ( gl == null || patch == null ) {
            return;
        }
        gl.glBegin(GL2.GL_LINE_LOOP);
        for ( int i = 0; i < patch.getNumberOfVertices(); i++ ) {
            gl.glVertex3d(patch.getVertices()[i].point.x(), patch.getVertices()[i].point.y(), patch.getVertices()[i].point.z());
        }
        gl.glEnd();
    }

    private static void openGlInvokeRenderPatch(OpenGlRenderTraversalCallback<?> renderPatch, Patch patch, Camera camera, RendererConfiguration renderOptions) {
        if ( renderPatch == null ) {
            return;
        }
        renderPatch.apply(patch, camera, renderOptions);
    }

    private static void openGlReallyRenderOctreeLeaf(Camera camera, RpkGeometry geometry, OpenGlRenderTraversalCallback<?> renderPatch, RendererConfiguration renderOptions) {
        ArrayList<Patch> patchList = RpkGeometry.patchListReference(geometry);
        for ( int i = 0; patchList != null && i < patchList.size(); i++ ) {
            openGlInvokeRenderPatch(renderPatch, patchList.get(i), camera, renderOptions);
        }
    }

    private static void openGlRenderOctreeLeaf(Camera camera, RpkGeometry geometry, OpenGlRenderTraversalCallback<?> renderPatch, RendererConfiguration renderOptions) {
        openGlReallyRenderOctreeLeaf(camera, geometry, renderPatch, renderOptions);
    }

    private static boolean openGlViewCullBounds(Camera camera, RpkAxisAlignedBoundingBox bounds) {
        if ( camera == null || bounds == null ) {
            return false;
        }
        for ( int i = 0; i < Camera.NUMBER_OF_VIEW_PLANES; i++ ) {
            if ( bounds.behindPlane(camera.viewPlanes[i].normal, camera.viewPlanes[i].d) ) {
                return true;
            }
        }
        return false;
    }

    private static double openGlBoundsDistance2(Vector3Dd p, RpkAxisAlignedBoundingBox boundingBox) {
        Vector3Dd mid = boundingBox.center();
        Vector3Dd d = new Vector3Dd();
        d = mid.subtract(p);
        return d.lengthSquared();
    }

    private static void openGlRenderOctreeNonLeaf(Camera camera, RpkGeometry geometry, OpenGlRenderTraversalCallback<?> renderPatchCallback, RendererConfiguration renderOptions) {
        OctreeChild[] octreeChildren = new OctreeChild[8];
        for ( int i = 0; i < octreeChildren.length; i++ ) {
            octreeChildren[i] = new OctreeChild();
        }

        ArrayList<RpkGeometry> children = RpkGeometry.primitiveListCopy(geometry);

        int i = 0;
        for ( int j = 0; children != null && j < children.size(); j++ ) {
            RpkGeometry child = children.get(j);
            if ( child.isCompound() ) {
                if ( i >= 8 ) {
                    Logger.reportMessage(null, Logger.ERROR, "openGlRenderOctreeNonLeaf", "Invalid octree geometry node (more than 8 compound children)");
                    return;
                }
                octreeChildren[i++].geometry = child;
            }
            else {
                openGlRenderOctreeLeaf(camera, child, renderPatchCallback, renderOptions);
            }
        }

        int n = i;
        for ( i = 0; i < n; i++ ) {
            if ( openGlViewCullBounds(camera, octreeChildren[i].geometry.boundingBox) ) {
                octreeChildren[i].geometry = null;
                octreeChildren[i].distance = Double.MAX_VALUE;
            }
            else {
                octreeChildren[i].distance = openGlBoundsDistance2(camera.eyePosition, octreeChildren[i].geometry.boundingBox);
            }
        }

        int remaining = n;
        while ( remaining > 0 ) {
            int closest = 0;
            for ( i = 1; i < n; i++ ) {
                if ( octreeChildren[i].distance < octreeChildren[closest].distance ) {
                    closest = i;
                }
            }

            if ( octreeChildren[closest].geometry == null ) {
                break;
            }

            openGlRenderOctreeNonLeaf(camera, octreeChildren[closest].geometry, renderPatchCallback, renderOptions);
            octreeChildren[closest].geometry = null;
            octreeChildren[closest].distance = Double.MAX_VALUE;
            remaining--;
        }
    }

    public static void openGlRenderWorldOctree(Scene scene, OpenGlRenderPatchCallback renderPatchCallback, RendererConfiguration renderOptions) {
        if ( scene == null || scene.clusteredRootGeometry == null ) {
            return;
        }

        OpenGlRenderTraversalCallback<FaceColorTable> callbackContext =
            new OpenGlRenderTraversalCallback<>(
                renderPatchCallback,
                renderPatchCallback == null ? Opengl::openGlRenderPatchCallBack : null,
                renderPatchCallback == null ? scene.getFaceColors() : null);

        if ( scene.clusteredRootGeometry.isCompound() ) {
            openGlRenderOctreeNonLeaf(scene.camera, scene.clusteredRootGeometry, callbackContext, renderOptions);
        }
        else {
            openGlRenderOctreeLeaf(scene.camera, scene.clusteredRootGeometry, callbackContext, renderOptions);
        }
    }

    /**
    Renders the all the patches using default colors
    */
    public static void openGlRenderPatchCallBack(Patch patch, Camera camera, RendererConfiguration renderOptions, FaceColorTable faceColors) {
        if ( patch == null || camera == null || renderOptions == null ) {
            return;
        }

        if ( !renderOptions.noShading ) {
            if ( renderOptions.smoothShading ) {
                openGlRenderPatchSmooth(patch, renderOptions);
            }
            else {
                openGlRenderPatchFlat(patch, faceColors.get(patch.getFaceIndex()), renderOptions);
            }
        }

        if ( renderOptions.drawOutlines
             && (patch.getNormal().dotProduct(camera.eyePosition) + patch.getPlaneConstant() > Numeric.EPSILON) ) {
            openGlRenderSetColor(renderOptions.outlineColor, renderOptions);
            openGlRenderPatchOutline(patch);
        }
    }

    public static void openGlRenderSetLineWidth(double width) {
        GL2 gl = gl();
        if ( gl == null ) {
            return;
        }
        gl.glLineWidth((float)width); // OpenGL has no double line width
    }

    public static void openGlRenderSetCamera(Camera camera, ArrayList<RpkGeometry> sceneGeometries) {
        GL2 gl = gl();
        if ( gl == null || camera == null ) {
            return;
        }

        openGlRenderClearWindow(camera);
        gl.glViewport(0, 0, camera.xSize, camera.ySize);

        RenderOpenGL.renderGetNearFar(camera, sceneGeometries);

        gl.glMatrixMode(GL2.GL_PROJECTION);
        gl.glLoadIdentity();
        GLU_HELPER.gluPerspective(
            camera.verticalFov * 2.0,
            (double)camera.xSize / (double)camera.ySize,
            camera.near / 10.0,
            camera.far * 10.0);

        gl.glMatrixMode(GL2.GL_MODELVIEW);
        gl.glLoadIdentity();
        GLU_HELPER.gluLookAt(
            camera.eyePosition.x(), camera.eyePosition.y(), camera.eyePosition.z(),
            camera.lookPosition.x(), camera.lookPosition.y(), camera.lookPosition.z(),
            camera.upDirection.x(), camera.upDirection.y(), camera.upDirection.z());
    }

    private static Vector3Dd sceneRotationPivot(Scene scene) {
        if ( scene == null ) {
            return new Vector3Dd(0.0, 0.0, 0.0);
        }

        if ( scene.clusteredRootGeometry != null && scene.clusteredRootGeometry.bounded ) {
            return scene.clusteredRootGeometry.boundingBox.center();
        }

        if ( scene.geometryList != null && !scene.geometryList.isEmpty() ) {
            RpkAxisAlignedBoundingBox sceneBounds = new RpkAxisAlignedBoundingBox();
            RpkGeometry.listBounds(scene.geometryList, sceneBounds);
            return sceneBounds.center();
        }

        return new Vector3Dd(0.0, 0.0, 0.0);
    }

    private static void viewportAxesInWorld(Scene scene, Vector3Dd[] axisU, Vector3Dd[] axisV) {
        if ( axisU == null || axisV == null ) {
            return;
        }

        axisU[0] = new Vector3Dd(1.0, 0.0, 0.0);
        axisV[0] = new Vector3Dd(0.0, 1.0, 0.0);

        if ( scene == null || scene.camera == null ) {
            return;
        }

        Camera camera = scene.camera;
        Vector3Dd cameraU = new Vector3Dd();
        cameraU = camera.X;
        Vector3Dd cameraV = new Vector3Dd();
        cameraV = camera.Y;

        if ( cameraU.lengthSquared() < Numeric.EPSILON || cameraV.lengthSquared() < Numeric.EPSILON ) {
            Vector3Dd viewDirection = new Vector3Dd();
            viewDirection = camera.lookPosition.subtract(camera.eyePosition);
            if ( viewDirection.lengthSquared() < Numeric.EPSILON ) {
                return;
            }
            viewDirection = viewDirection.normalized(Numeric.EPSILON);

            Vector3Dd upDirection = new Vector3Dd();
            upDirection = camera.upDirection;
            if ( upDirection.lengthSquared() < Numeric.EPSILON ) {
                upDirection = new Vector3Dd(0.0, 0.0, 1.0);
            }
            else {
                upDirection = upDirection.normalized(Numeric.EPSILON);
            }

            cameraU = viewDirection.crossProduct(upDirection);
            if ( cameraU.lengthSquared() < Numeric.EPSILON ) {
                upDirection = new Vector3Dd(0.0, 1.0, 0.0);
                cameraU = viewDirection.crossProduct(upDirection);
            }
            if ( cameraU.lengthSquared() < Numeric.EPSILON ) {
                return;
            }
            cameraU = cameraU.normalized(Numeric.EPSILON);
            cameraV = viewDirection.crossProduct(cameraU);
        }

        if ( cameraU.lengthSquared() < Numeric.EPSILON || cameraV.lengthSquared() < Numeric.EPSILON ) {
            return;
        }

        cameraU = cameraU.normalized(Numeric.EPSILON);
        cameraV = cameraV.normalized(Numeric.EPSILON);
        axisU[0] = cameraU;
        axisV[0] = cameraV;
    }

    public static void openGlApplyDebugSceneRotation(Scene scene, GlutDebugState debugState) {
        GL2 gl = gl();
        if ( gl == null || debugState == null ) {
            return;
        }

        boolean hasRotation = debugState.angleAroundViewportU != 0.0 || debugState.angleAroundViewportV != 0.0;
        if ( !hasRotation ) {
            return;
        }

        Vector3Dd pivot = sceneRotationPivot(scene);
        Vector3Dd[] axisU = {new Vector3Dd()};
        Vector3Dd[] axisV = {new Vector3Dd()};
        viewportAxesInWorld(scene, axisU, axisV);

        gl.glTranslated(pivot.x(), pivot.y(), pivot.z());
        gl.glRotated(debugState.angleAroundViewportU, axisU[0].x(), axisU[0].y(), axisU[0].z());
        gl.glRotated(debugState.angleAroundViewportV, axisV[0].x(), axisV[0].y(), axisV[0].z());
        gl.glTranslated(-pivot.x(), -pivot.y(), -pivot.z());
    }

    private static void openGlReallyRender(Scene scene, RadianceMethod radianceMethod, RendererConfiguration renderOptions, GlutDebugState debugState) {
        GL2 gl = gl();
        if ( gl == null ) {
            return;
        }

        gl.glPushMatrix();
        openGlApplyDebugSceneRotation(scene, debugState);

        if ( radianceMethod != null ) {
            if ( radianceMethod.className == RadianceMethodAlgorithm.GALERKIN ) {
                GalerkinOpenGLRenderer.renderScene(scene, renderOptions, debugState);
            }
            else {
                System.err.println("OpenGL supports only rendering of Galerkin patches");
                System.exit(1);
            }
        }
        else if ( renderOptions.frustumCulling ) {
            openGlRenderWorldOctree(scene, null, renderOptions);
        }
        else {
            for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                openGlRenderPatchCallBack(scene.patchList.get(i), scene.camera, renderOptions, scene.getFaceColors());
            }
        }

        gl.glPopMatrix();
    }

    private static void openGlRenderRadiance(Scene scene, RadianceMethod radianceMethod, RendererConfiguration renderOptions, GlutDebugState debugState) {
        GL2 gl = gl();
        if ( gl == null ) {
            return;
        }

        gl.glShadeModel(renderOptions.smoothShading ? GL2.GL_SMOOTH : GL2.GL_FLAT);
        openGlRenderSetCamera(scene.camera, scene.geometryList);

        if ( renderOptions.backfaceCulling ) {
            gl.glEnable(GL.GL_CULL_FACE);
        }
        else {
            gl.glDisable(GL.GL_CULL_FACE);
        }

        openGlReallyRender(scene, radianceMethod, renderOptions, debugState);

        if ( renderOptions.drawBoundingBoxes ) {
            RenderOpenGL.renderBoundingBoxHierarchy(scene.camera, scene.geometryList, renderOptions);
        }

        if ( renderOptions.drawClusters ) {
            RenderOpenGL.renderClusterHierarchy(scene.camera, scene.clusteredGeometryList, renderOptions);
        }
    }

    public static void openGlRenderScene(Scene scene, RadianceMethod radianceMethod, ToneMappingContext toneMapOptions, RendererConfiguration renderOptions, GlutDebugState debugState) {
        if ( scene == null ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "Opengl::openGlRenderScene", "Scene not provided");
        }

        activeToneMapOptions = toneMapOptions;
        if ( toneMapOptions == null && !openGlMissingToneMapWarningShown ) {
            Logger.reportMessage(null, Logger.WARNING, "Opengl::openGlRenderScene", "Tone mapping context not provided, using uncorrected color");
            openGlMissingToneMapWarningShown = true;
        }

        openGlRenderSetLineWidth(renderOptions.lineWidth);

        Canvas.canvasPushMode();
        if ( !renderOptions.renderRayTracedImage ) {
            openGlRenderRadiance(scene, radianceMethod, renderOptions, debugState);
        }
        RenderHookList.renderHooks();

        GL2 gl = gl();
        if ( gl != null ) {
            gl.glFinish();
        }

        Canvas.canvasPullMode();
        activeToneMapOptions = null;
    }
}
