package vsdk.toolkit.render.jogl;

import com.jogamp.opengl.GL;
import com.jogamp.opengl.GL2;
import com.jogamp.opengl.glu.GLU;
import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.render.Canvas;
import vsdk.toolkit.render.OctreeChild;
import vsdk.toolkit.render.RenderHookList;
import vsdk.toolkit.render.jogl.visualDebugTools.GlutDebugState;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.RadianceMethodAlgorithm;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Geometry;
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
        gl.glClearColor(camera.background.r, camera.background.g, camera.background.b, 0.0f);
        gl.glClearDepth(1.0);
        gl.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT);
    }

    public static void openGlRenderLine(Vector3D x, Vector3D y) {
        GL2 gl = gl();
        if ( gl == null || x == null || y == null ) {
            return;
        }

        gl.glDisable(GL2.GL_POLYGON_OFFSET_FILL);
        gl.glBegin(GL2.GL_LINES);
        gl.glVertex3f(x.x, x.y, x.z);
        gl.glVertex3f(y.x, y.y, y.z);
        gl.glEnd();
        gl.glEnable(GL2.GL_POLYGON_OFFSET_FILL);
        gl.glPolygonOffset(1.0f, 1.0f);
    }

    public static void openGlRenderSetColor(ColorRgb rgb, RenderOptions renderOptions) {
        GL2 gl = gl();
        if ( gl == null || rgb == null ) {
            return;
        }

        ColorRgb corrected = new ColorRgb(rgb.r, rgb.g, rgb.b);
        if ( activeToneMapOptions != null ) {
            ToneMap.toneMappingGammaCorrection(corrected, activeToneMapOptions);
        }
        else if ( !openGlMissingToneMapWarningShown ) {
            Logger.warning("Opengl::openGlRenderSetColor", "Tone mapping context not set in active scene, using uncorrected color");
            openGlMissingToneMapWarningShown = true;
        }
        gl.glColor3f(corrected.r, corrected.g, corrected.b);
    }

    public static void openGlRenderPolygonFlat(int numberOfVertices, Vector3D[] vertices) {
        GL2 gl = gl();
        if ( gl == null || vertices == null || numberOfVertices <= 0 ) {
            return;
        }
        gl.glBegin(GL2.GL_POLYGON);
        for ( int i = 0; i < numberOfVertices; i++ ) {
            gl.glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
        }
        gl.glEnd();
    }

    public static void openGlRenderPolygonGouraud(int numberOfVertices, Vector3D[] vertices, ColorRgb[] verticesColors, RenderOptions renderOptions) {
        GL2 gl = gl();
        if ( gl == null || vertices == null || verticesColors == null || numberOfVertices <= 0 ) {
            return;
        }

        gl.glBegin(GL2.GL_POLYGON);
        for ( int i = 0; i < numberOfVertices; i++ ) {
            openGlRenderSetColor(verticesColors[i], renderOptions);
            gl.glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
        }
        gl.glEnd();
    }

    private static void openGlRenderPatchFlat(Patch patch, RenderOptions renderOptions) {
        if ( patch == null ) {
            return;
        }
        openGlRenderSetColor(patch.color, renderOptions);
        GL2 gl = gl();
        if ( gl == null ) {
            return;
        }

        int primitive = patch.numberOfVertices == 3
            ? GL2.GL_TRIANGLES
            : (patch.numberOfVertices == 4 ? GL2.GL_QUADS : GL2.GL_POLYGON);
        gl.glBegin(primitive);
        for ( int i = 0; i < patch.numberOfVertices; i++ ) {
            gl.glVertex3f(patch.vertex[i].point.x, patch.vertex[i].point.y, patch.vertex[i].point.z);
        }
        gl.glEnd();
    }

    private static void openGlRenderPatchSmooth(Patch patch, RenderOptions renderOptions) {
        if ( patch == null ) {
            return;
        }
        GL2 gl = gl();
        if ( gl == null ) {
            return;
        }

        int primitive = patch.numberOfVertices == 3
            ? GL2.GL_TRIANGLES
            : (patch.numberOfVertices == 4 ? GL2.GL_QUADS : GL2.GL_POLYGON);
        gl.glBegin(primitive);
        for ( int i = 0; i < patch.numberOfVertices; i++ ) {
            openGlRenderSetColor(patch.vertex[i].color, renderOptions);
            gl.glVertex3f(patch.vertex[i].point.x, patch.vertex[i].point.y, patch.vertex[i].point.z);
        }
        gl.glEnd();
    }

    public static void openGlRenderPatchOutline(Patch patch) {
        GL2 gl = gl();
        if ( gl == null || patch == null ) {
            return;
        }
        gl.glBegin(GL2.GL_LINE_LOOP);
        for ( int i = 0; i < patch.numberOfVertices; i++ ) {
            gl.glVertex3f(patch.vertex[i].point.x, patch.vertex[i].point.y, patch.vertex[i].point.z);
        }
        gl.glEnd();
    }

    private static void openGlInvokeRenderPatch(OpenGlRenderTraversalCallback renderPatch, Patch patch, Camera camera, RenderOptions renderOptions) {
        if ( renderPatch == null ) {
            return;
        }
        if ( renderPatch.callbackWithData != null ) {
            renderPatch.callbackWithData.apply(patch, camera, renderOptions, renderPatch.callbackData);
        }
        else if ( renderPatch.callbackWithoutData != null ) {
            renderPatch.callbackWithoutData.apply(patch, camera, renderOptions);
        }
    }

    private static void openGlReallyRenderOctreeLeaf(Camera camera, Geometry geometry, OpenGlRenderTraversalCallback renderPatch, RenderOptions renderOptions) {
        ArrayList<Patch> patchList = Geometry.patchListReference(geometry);
        for ( int i = 0; patchList != null && i < patchList.size(); i++ ) {
            openGlInvokeRenderPatch(renderPatch, patchList.get(i), camera, renderOptions);
        }
    }

    private static void openGlRenderOctreeLeaf(Camera camera, Geometry geometry, OpenGlRenderTraversalCallback renderPatch, RenderOptions renderOptions) {
        openGlReallyRenderOctreeLeaf(camera, geometry, renderPatch, renderOptions);
    }

    private static boolean openGlViewCullBounds(Camera camera, BoundingBox bounds) {
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

    private static float openGlBoundsDistance2(Vector3D p, BoundingBox boundingBox) {
        Vector3D mid = boundingBox.center();
        Vector3D d = new Vector3D();
        d.subtraction(mid, p);
        return d.norm2();
    }

    private static void openGlRenderOctreeNonLeaf(Camera camera, Geometry geometry, OpenGlRenderTraversalCallback renderPatchCallback, RenderOptions renderOptions) {
        OctreeChild[] octreeChildren = new OctreeChild[8];
        for ( int i = 0; i < octreeChildren.length; i++ ) {
            octreeChildren[i] = new OctreeChild();
        }

        ArrayList<Geometry> children = Geometry.primitiveListCopy(geometry);

        int i = 0;
        for ( int j = 0; children != null && j < children.size(); j++ ) {
            Geometry child = children.get(j);
            if ( child.isCompound() ) {
                if ( i >= 8 ) {
                    Logger.error("openGlRenderOctreeNonLeaf", "Invalid octree geometry node (more than 8 compound children)");
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
                octreeChildren[i].distance = Numeric.HUGE_FLOAT_VALUE;
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
            octreeChildren[closest].distance = Numeric.HUGE_FLOAT_VALUE;
            remaining--;
        }
    }

    public static void openGlRenderWorldOctree(Scene scene, OpenGlRenderPatchCallback renderPatchCallback, RenderOptions renderOptions) {
        if ( scene == null || scene.clusteredRootGeometry == null ) {
            return;
        }

        OpenGlRenderTraversalCallback callbackContext = new OpenGlRenderTraversalCallback();
        if ( renderPatchCallback == null ) {
            renderPatchCallback = Opengl::openGlRenderPatchCallBack;
        }
        callbackContext.callbackWithoutData = renderPatchCallback;
        callbackContext.callbackWithData = null;
        callbackContext.callbackData = null;

        if ( scene.clusteredRootGeometry.isCompound() ) {
            openGlRenderOctreeNonLeaf(scene.camera, scene.clusteredRootGeometry, callbackContext, renderOptions);
        }
        else {
            openGlRenderOctreeLeaf(scene.camera, scene.clusteredRootGeometry, callbackContext, renderOptions);
        }
    }

    public static void openGlRenderPatchCallBack(Patch patch, Camera camera, RenderOptions renderOptions) {
        if ( patch == null || camera == null || renderOptions == null ) {
            return;
        }

        if ( !renderOptions.noShading ) {
            if ( renderOptions.smoothShading ) {
                openGlRenderPatchSmooth(patch, renderOptions);
            }
            else {
                openGlRenderPatchFlat(patch, renderOptions);
            }
        }

        if ( renderOptions.drawOutlines
             && (patch.normal.dotProduct(camera.eyePosition) + patch.planeConstant > Numeric.EPSILON_FLOAT) ) {
            openGlRenderSetColor(renderOptions.outlineColor, renderOptions);
            openGlRenderPatchOutline(patch);
        }
    }

    public static void openGlRenderSetLineWidth(float width) {
        GL2 gl = gl();
        if ( gl == null ) {
            return;
        }
        gl.glLineWidth(width);
    }

    public static void openGlRenderSetCamera(Camera camera, ArrayList<Geometry> sceneGeometries) {
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
            camera.eyePosition.x, camera.eyePosition.y, camera.eyePosition.z,
            camera.lookPosition.x, camera.lookPosition.y, camera.lookPosition.z,
            camera.upDirection.x, camera.upDirection.y, camera.upDirection.z);
    }

    private static Vector3D sceneRotationPivot(Scene scene) {
        if ( scene == null ) {
            return new Vector3D(0.0f, 0.0f, 0.0f);
        }

        if ( scene.clusteredRootGeometry != null && scene.clusteredRootGeometry.bounded ) {
            return scene.clusteredRootGeometry.boundingBox.center();
        }

        if ( scene.geometryList != null && !scene.geometryList.isEmpty() ) {
            BoundingBox sceneBounds = new BoundingBox();
            Geometry.listBounds(scene.geometryList, sceneBounds);
            return sceneBounds.center();
        }

        return new Vector3D(0.0f, 0.0f, 0.0f);
    }

    private static void viewportAxesInWorld(Scene scene, Vector3D axisU, Vector3D axisV) {
        if ( axisU == null || axisV == null ) {
            return;
        }

        axisU.set(1.0f, 0.0f, 0.0f);
        axisV.set(0.0f, 1.0f, 0.0f);

        if ( scene == null || scene.camera == null ) {
            return;
        }

        Camera camera = scene.camera;
        Vector3D cameraU = new Vector3D();
        cameraU.copy(camera.X);
        Vector3D cameraV = new Vector3D();
        cameraV.copy(camera.Y);

        if ( cameraU.norm2() < Numeric.EPSILON_FLOAT || cameraV.norm2() < Numeric.EPSILON_FLOAT ) {
            Vector3D viewDirection = new Vector3D();
            viewDirection.subtraction(camera.lookPosition, camera.eyePosition);
            if ( viewDirection.norm2() < Numeric.EPSILON_FLOAT ) {
                return;
            }
            viewDirection.normalize(Numeric.EPSILON_FLOAT);

            Vector3D upDirection = new Vector3D();
            upDirection.copy(camera.upDirection);
            if ( upDirection.norm2() < Numeric.EPSILON_FLOAT ) {
                upDirection.set(0.0f, 0.0f, 1.0f);
            }
            else {
                upDirection.normalize(Numeric.EPSILON_FLOAT);
            }

            cameraU.crossProduct(viewDirection, upDirection);
            if ( cameraU.norm2() < Numeric.EPSILON_FLOAT ) {
                upDirection.set(0.0f, 1.0f, 0.0f);
                cameraU.crossProduct(viewDirection, upDirection);
            }
            if ( cameraU.norm2() < Numeric.EPSILON_FLOAT ) {
                return;
            }
            cameraU.normalize(Numeric.EPSILON_FLOAT);
            cameraV.crossProduct(viewDirection, cameraU);
        }

        if ( cameraU.norm2() < Numeric.EPSILON_FLOAT || cameraV.norm2() < Numeric.EPSILON_FLOAT ) {
            return;
        }

        cameraU.normalize(Numeric.EPSILON_FLOAT);
        cameraV.normalize(Numeric.EPSILON_FLOAT);
        axisU.copy(cameraU);
        axisV.copy(cameraV);
    }

    public static void openGlApplyDebugSceneRotation(Scene scene, GlutDebugState debugState) {
        GL2 gl = gl();
        if ( gl == null || debugState == null ) {
            return;
        }

        boolean hasRotation = debugState.angleAroundViewportU != 0.0f || debugState.angleAroundViewportV != 0.0f;
        if ( !hasRotation ) {
            return;
        }

        Vector3D pivot = sceneRotationPivot(scene);
        Vector3D axisU = new Vector3D();
        Vector3D axisV = new Vector3D();
        viewportAxesInWorld(scene, axisU, axisV);

        gl.glTranslatef(pivot.x, pivot.y, pivot.z);
        gl.glRotatef(debugState.angleAroundViewportU, axisU.x, axisU.y, axisU.z);
        gl.glRotatef(debugState.angleAroundViewportV, axisV.x, axisV.y, axisV.z);
        gl.glTranslatef(-pivot.x, -pivot.y, -pivot.z);
    }

    private static void openGlReallyRender(Scene scene, RadianceMethod radianceMethod, RenderOptions renderOptions, GlutDebugState debugState) {
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
            openGlRenderWorldOctree(scene, Opengl::openGlRenderPatchCallBack, renderOptions);
        }
        else {
            for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                openGlRenderPatchCallBack(scene.patchList.get(i), scene.camera, renderOptions);
            }
        }

        gl.glPopMatrix();
    }

    private static void openGlRenderRadiance(Scene scene, RadianceMethod radianceMethod, RenderOptions renderOptions, GlutDebugState debugState) {
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

    public static void openGlRenderScene(Scene scene, RadianceMethod radianceMethod, ToneMappingContext toneMapOptions, RenderOptions renderOptions, GlutDebugState debugState) {
        if ( scene == null ) {
            Logger.fatal(-1, "Opengl::openGlRenderScene", "Scene not provided");
        }

        activeToneMapOptions = toneMapOptions;
        if ( toneMapOptions == null && !openGlMissingToneMapWarningShown ) {
            Logger.warning("Opengl::openGlRenderScene", "Tone mapping context not provided, using uncorrected color");
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
