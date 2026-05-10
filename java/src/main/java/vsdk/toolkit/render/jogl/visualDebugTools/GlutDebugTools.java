package vsdk.toolkit.render.jogl.visualDebugTools;

import com.jogamp.opengl.GL;
import com.jogamp.opengl.GL2;
import com.jogamp.opengl.GLAutoDrawable;
import com.jogamp.opengl.GLCapabilities;
import com.jogamp.opengl.GLException;
import com.jogamp.opengl.GLEventListener;
import com.jogamp.opengl.GLProfile;
import com.jogamp.opengl.awt.GLJPanel;
import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.awt.MouseInfo;
import java.awt.Point;
import java.awt.PointerInfo;
import java.awt.Rectangle;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseMotionAdapter;
import java.util.concurrent.CountDownLatch;
import javax.swing.JFrame;
import javax.swing.SwingUtilities;
import javax.swing.WindowConstants;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.render.jogl.Opengl;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.Patch;

public class GlutDebugTools implements GLEventListener {
    private final GlutDebugToolsModel model;
    private JFrame frame;
    private GLJPanel panel;
    private GLCapabilities panelCapabilities;
    private final CountDownLatch closeLatch = new CountDownLatch(1);
    private boolean fullscreenTransitionInProgress = false;
    private int suppressedWindowClosedEvents = 0;
    private boolean shutdownInProgress = false;

    public GlutDebugTools(GlutDebugToolsModel initialModel) {
        model = initialModel;
    }

    private GLJPanel createConfiguredPanel() {
        if ( panelCapabilities == null ) {
            GLProfile profile = GLProfile.get(GLProfile.GL2);
            panelCapabilities = new GLCapabilities(profile);
        }

        GLJPanel newPanel = new GLJPanel(panelCapabilities);
        newPanel.setPreferredSize(new Dimension(model.width, model.height));
        newPanel.setFocusable(true);
        newPanel.addGLEventListener(this);

        newPanel.addKeyListener(new KeyAdapter() {
            @Override
            public void keyTyped(KeyEvent e) {
                char c = e.getKeyChar();
                if ( c != KeyEvent.CHAR_UNDEFINED ) {
                    keypressCallback(c);
                }
            }

            @Override
            public void keyPressed(KeyEvent e) {
                if ( e.getKeyCode() != KeyEvent.VK_UNDEFINED ) {
                    extendedKeypressCallback(e.getKeyCode());
                }
            }
        });

        newPanel.addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                mouseButtonCallback(e.getButton(), MouseEvent.MOUSE_PRESSED, e.getX(), e.getY(), e.isShiftDown());
            }

            @Override
            public void mouseReleased(MouseEvent e) {
                mouseButtonCallback(e.getButton(), MouseEvent.MOUSE_RELEASED, e.getX(), e.getY(), e.isShiftDown());
            }
        });

        newPanel.addMouseMotionListener(new MouseMotionAdapter() {
            @Override
            public void mouseDragged(MouseEvent e) {
                mouseMotionCallback(e.getX(), e.getY());
            }
        });

        return newPanel;
    }

    private void syncModelSizeFromDrawable(GLAutoDrawable drawable) {
        if ( drawable == null ) {
            return;
        }
        int drawableWidth = drawable.getSurfaceWidth();
        int drawableHeight = drawable.getSurfaceHeight();
        int oldWidth = model.width;
        int oldHeight = model.height;
        resizeCallback(drawableWidth, drawableHeight);
        if ( oldWidth != model.width || oldHeight != model.height ) {
            // Keep camera projection parameters coherent with the active GL viewport.
            syncCameraToViewport();
        }
    }

    private void resizeCallback(int newWidth, int newHeight) {
        if ( newWidth <= 0 || newHeight <= 0 ) {
            return;
        }
        model.width = newWidth;
        model.height = newHeight;
        if ( !model.fullScreen ) {
            model.windowedWidth = newWidth;
            model.windowedHeight = newHeight;
        }
    }

    private void syncCameraToViewport() {
        if ( model.scene == null || model.scene.camera == null ) {
            return;
        }
        if ( model.width <= 0 || model.height <= 0 ) {
            return;
        }

        Camera camera = model.scene.camera;
        if ( camera.xSize == model.width
             && camera.ySize == model.height
             && camera.pixelWidth > Numeric.EPSILON_FLOAT
             && camera.pixelHeight > Numeric.EPSILON_FLOAT ) {
            return;
        }

        camera.set(
            camera.eyePosition,
            camera.lookPosition,
            camera.upDirection,
            camera.fieldOfVision,
            model.width,
            model.height,
            camera.background);
    }

    private void printElementHierarchy(GalerkinElement element, int level) {
        if ( element == null ) {
            return;
        }

        switch ( level ) {
            case 0:
                break;
            case 1:
                System.out.printf("  - ");
                break;
            case 2:
                System.out.printf("    . ");
                break;
            default:
                System.out.printf("      (%d) ", level);
                for ( int i = 3; i < level; i++ ) {
                    System.out.printf(" ");
                }
                System.out.printf("-> ");
                break;
        }

        ColorRgb c = element.radiance == null || element.radiance.length == 0 ? null : element.radiance[0];
        long numberOfInteractions = element.interactions == null ? 0 : element.interactions.size();

        if ( element.regularSubElements == null ) {
            if ( c == null ) {
                System.out.printf("Child element no radiance\n");
            }
            else {
                System.out.printf(
                    "Child element radiance <%0.4f, %0.4f, %0.4f>, interactions: %d\n",
                    c.r, c.g, c.b, numberOfInteractions);
            }
        }
        else {
            if ( c == null ) {
                System.out.printf("Container element no radiance\n");
            }
            else {
                System.out.printf(
                    "Container element radiance <%0.4f, %0.4f, %0.4f>, interactions: %d\n",
                    c.r, c.g, c.b, numberOfInteractions);
            }
            for ( int i = 0; i < 4; i++ ) {
                if ( element.regularSubElements[i] instanceof GalerkinElement ) {
                    printElementHierarchy((GalerkinElement)element.regularSubElements[i], level + 1);
                }
            }
        }
    }

    private void printGalerkinElementForPatch(Scene scene, int patchIndex) {
        System.out.printf("================================================================================\n");
        if ( scene == null || scene.patchList == null || patchIndex < 0 || patchIndex >= scene.patchList.size() ) {
            return;
        }

        Patch patch = scene.patchList.get(patchIndex);
        if ( patch == null || patch.radianceData == null ) {
            return;
        }

        GalerkinElement element = GalerkinElement.fromPatch(patch);
        System.out.printf("Galerkin element for patch[%d] %d\n", patchIndex, patch.id);
        printElementHierarchy(element, 0);
    }

    private void postRedisplay() {
        if ( panel == null || fullscreenTransitionInProgress ) {
            return;
        }

        SwingUtilities.invokeLater(() -> {
            if ( panel == null || fullscreenTransitionInProgress || !panel.isDisplayable() ) {
                return;
            }
            try {
                panel.display();
            }
            catch ( GLException e ) {
                // During AWT fullscreen transitions JOGL may temporarily lose the current context.
                // Ignore this transient state; a subsequent repaint will recover.
            }
        });
    }

    private void keypressCallback(char keyChar) {
        if ( keyChar == 27 ) {
            requestGracefulShutdown();
            return;
        }

        if ( GlutDebugToolsKeyControl.handleKeypress(keyChar, model, this::printGalerkinElementForPatch) ) {
            applyFullscreenStateIfNeeded();
            postRedisplay();
        }
    }

    private void requestGracefulShutdown() {
        if ( shutdownInProgress ) {
            return;
        }
        shutdownInProgress = true;

        if ( model.memoryFreeCallBack != null ) {
            model.memoryFreeCallBack.accept(model.mgfContext);
        }

        SwingUtilities.invokeLater(() -> {
            try {
                if ( panel != null ) {
                    try {
                        panel.destroy();
                    }
                    catch ( RuntimeException e ) {
                        // Best effort cleanup before closing the window.
                    }
                }
                if ( frame != null ) {
                    frame.dispose();
                }
            }
            finally {
                closeLatch.countDown();
            }
        });
    }

    private void extendedKeypressCallback(int keyCode) {
        if ( GlutDebugToolsKeyControl.handleExtendedKeypress(keyCode, model) ) {
            postRedisplay();
        }
    }

    private void mouseButtonCallback(int button, int state, int x, int y, boolean shiftDown) {
        syncModelSizeFromDrawable(panel);
        syncCameraToViewport();
        int[] drawablePoint = mousePointInDrawablePixels(x, y);
        if ( GlutDebugToolsMouseControl.handleMouseButton(
                button,
                state,
                drawablePoint[0],
                drawablePoint[1],
                shiftDown,
                model) ) {
            postRedisplay();
        }
    }

    private void mouseMotionCallback(int x, int y) {
        syncModelSizeFromDrawable(panel);
        syncCameraToViewport();
        int[] drawablePoint = mousePointInDrawablePixels(x, y);
        if ( GlutDebugToolsMouseControl.handleMouseMotion(drawablePoint[0], drawablePoint[1], model) ) {
            postRedisplay();
        }
    }

    private static int clampToSurface(int value, int surfaceSize) {
        if ( surfaceSize <= 0 ) {
            return 0;
        }
        if ( value < 0 ) {
            return 0;
        }
        if ( value >= surfaceSize ) {
            return surfaceSize - 1;
        }
        return value;
    }

    private static int scaleToSurface(int value, int componentSize, int surfaceSize) {
        if ( componentSize <= 0 || surfaceSize <= 0 ) {
            return 0;
        }
        if ( componentSize == surfaceSize ) {
            return value;
        }

        float scaled = ((float)value + 0.5f) * ((float)surfaceSize / (float)componentSize) - 0.5f;
        return Math.round(scaled);
    }

    private int[] mousePointInDrawablePixels(int awtX, int awtY) {
        int surfaceWidth = model.width;
        int surfaceHeight = model.height;
        int componentWidth = surfaceWidth;
        int componentHeight = surfaceHeight;

        if ( panel != null ) {
            int panelSurfaceWidth = panel.getSurfaceWidth();
            int panelSurfaceHeight = panel.getSurfaceHeight();
            if ( panelSurfaceWidth > 0 ) {
                surfaceWidth = panelSurfaceWidth;
            }
            if ( panelSurfaceHeight > 0 ) {
                surfaceHeight = panelSurfaceHeight;
            }

            componentWidth = panel.getWidth();
            componentHeight = panel.getHeight();
            if ( componentWidth <= 0 ) {
                componentWidth = surfaceWidth;
            }
            if ( componentHeight <= 0 ) {
                componentHeight = surfaceHeight;
            }
        }

        int mappedX = scaleToSurface(awtX, componentWidth, surfaceWidth);
        int mappedY = scaleToSurface(awtY, componentHeight, surfaceHeight);
        mappedX = clampToSurface(mappedX, surfaceWidth);
        mappedY = clampToSurface(mappedY, surfaceHeight);
        return new int[] {mappedX, mappedY};
    }

    private void applyFullscreenStateIfNeeded() {
        if ( frame == null || model.fullScreen == model.fullScreenApplied || fullscreenTransitionInProgress ) {
            return;
        }

        fullscreenTransitionInProgress = true;
        GraphicsDevice device = fullscreenTargetDevice();
        try {
            // Frame recreation is required by AWT when changing decoration/fullscreen state.
            // During this transition, temporary close events must not terminate the application.
            suppressedWindowClosedEvents++;

            if ( panel != null ) {
                frame.getContentPane().remove(panel);
                try {
                    panel.destroy();
                }
                catch ( RuntimeException e ) {
                    // Best effort cleanup of native GL resources before recreating the drawable.
                }
                panel = null;
            }

            frame.dispose();
            frame.setUndecorated(model.fullScreen);
            if ( model.fullScreen ) {
                device.setFullScreenWindow(frame);
                model.fullScreenApplied = true;
            }
            else {
                device.setFullScreenWindow(null);
                frame.setLocation(0, 0);
                frame.setSize(model.windowedWidth, model.windowedHeight);
                model.fullScreenApplied = false;
            }

            panel = createConfiguredPanel();
            frame.add(panel, BorderLayout.CENTER);
            frame.setVisible(true);
            frame.validate();
            if ( panel != null ) {
                panel.requestFocusInWindow();
                panel.display();
            }
        }
        catch ( RuntimeException e ) {
            // Keep the app alive even if platform fullscreen integration fails.
            System.err.println("ERROR: Unable to switch fullscreen mode on this platform.");
        }
        finally {
            fullscreenTransitionInProgress = false;
        }
    }

    private GraphicsDevice fullscreenTargetDevice() {
        GraphicsEnvironment environment = GraphicsEnvironment.getLocalGraphicsEnvironment();
        GraphicsDevice[] devices = environment.getScreenDevices();
        if ( devices == null || devices.length == 0 ) {
            return environment.getDefaultScreenDevice();
        }

        Point pointerLocation = null;
        try {
            PointerInfo pointerInfo = MouseInfo.getPointerInfo();
            if ( pointerInfo != null ) {
                pointerLocation = pointerInfo.getLocation();
            }
        }
        catch ( RuntimeException e ) {
            // Fall back to the default device when pointer info is unavailable.
        }

        if ( pointerLocation != null ) {
            for ( GraphicsDevice candidate : devices ) {
                Rectangle bounds = candidate.getDefaultConfiguration().getBounds();
                if ( bounds.contains(pointerLocation) ) {
                    return candidate;
                }
            }
        }

        return environment.getDefaultScreenDevice();
    }

    @Override
    public void init(GLAutoDrawable drawable) {
        syncModelSizeFromDrawable(drawable);
        if ( drawable == null || drawable.getGL() == null ) {
            return;
        }
        GL2 gl = drawable.getGL().getGL2();
        gl.glEnable(GL.GL_DEPTH_TEST);
        gl.glDisable(GL2.GL_LIGHTING);
        if ( model.renderOptions != null ) {
            model.renderOptions.frustumCulling = false;
        }
    }

    @Override
    public void dispose(GLAutoDrawable drawable) {
    }

    @Override
    public void display(GLAutoDrawable drawable) {
        if ( model.scene == null || model.renderOptions == null ) {
            return;
        }
        if ( drawable == null || drawable.getGL() == null ) {
            return;
        }

        syncModelSizeFromDrawable(drawable);
        syncCameraToViewport();

        GL2 gl;
        try {
            gl = drawable.getGL().getGL2();
        }
        catch ( RuntimeException e ) {
            return;
        }
        Opengl.setCurrentGl(gl);

        gl.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT);
        gl.glEnable(GL.GL_DEPTH_TEST);
        gl.glViewport(0, 0, drawable.getSurfaceWidth(), drawable.getSurfaceHeight());

        int totalElements = 0;
        int selectedPatchIndex = -1;
        int secondarySelectedPatchIndex = -1;
        if ( model.debugState != null ) {
            selectedPatchIndex = model.debugState.primarySelectedPatch;
            secondarySelectedPatchIndex = model.debugState.selectedSelectedPatch;
        }

        if ( selectedPatchIndex < -1 ) {
            selectedPatchIndex = -1;
        }
        if ( secondarySelectedPatchIndex < -1 ) {
            secondarySelectedPatchIndex = -1;
        }

        if ( model.scene.patchList != null ) {
            totalElements = model.scene.patchList.size();
            if ( selectedPatchIndex >= totalElements ) {
                selectedPatchIndex = totalElements - 1;
            }
            if ( secondarySelectedPatchIndex >= totalElements ) {
                secondarySelectedPatchIndex = totalElements - 1;
            }
            if ( totalElements <= 0 ) {
                selectedPatchIndex = -1;
                secondarySelectedPatchIndex = -1;
            }
        }

        if ( model.mode == GlutDebugMode.RADIANCE_SCENE ) {
            // Keep filled rasterization in radiance scene; outlines are handled by RenderOptions.
            // Forcing GL_LINE here can collapse visual output to sparse edge artifacts.
            gl.glPolygonMode(GL.GL_FRONT_AND_BACK, GL2.GL_FILL);
            Opengl.openGlRenderScene(
                model.scene,
                model.radianceMethod,
                model.toneMapOptions,
                model.renderOptions,
                model.debugState);
        }
        else if ( model.mode == GlutDebugMode.GALERKIN_ELEMENT_HIERARCHY ) {
            gl.glPolygonMode(GL.GL_FRONT_AND_BACK, GL2.GL_FILL);
            Opengl.openGlRenderSetCamera(model.scene.camera, model.scene.geometryList);
            gl.glPushMatrix();
            Opengl.openGlApplyDebugSceneRotation(model.scene, model.debugState);
            GlutDebugPatchHierarchy.renderSelectedPatchAtLevel(
                model.scene,
                model.renderOptions,
                selectedPatchIndex,
                secondarySelectedPatchIndex,
                model.selectedHierarchyLevel);
            GlutDebugPatchHierarchy.renderSecondarySelectedPatchMarker(
                model.scene,
                model.renderOptions,
                secondarySelectedPatchIndex);
            gl.glPopMatrix();
        }

        GlutHudConsole.printTextLine(
            "MODE: " + GlutDebugModeTools.modeName(model.mode) + " [m]",
            0,
            0,
            model.width,
            model.height);

        String selectedElementText;
        if ( selectedPatchIndex >= 0 ) {
            selectedElementText = String.format("Element %d/%d [1, 2, mouse click]", selectedPatchIndex + 1, totalElements);
        }
        else {
            selectedElementText = String.format("Element none/%d [1, 2, mouse click]", totalElements);
        }
        GlutHudConsole.printTextLine(selectedElementText, 0, 1, model.width, model.height);

        if ( model.mode == GlutDebugMode.GALERKIN_ELEMENT_HIERARCHY ) {
            String secondaryText;
            if ( secondarySelectedPatchIndex >= 0 ) {
                secondaryText = String.format(
                    "Secondary element %d/%d [5, 6, shift-click]",
                    secondarySelectedPatchIndex + 1,
                    totalElements);
            }
            else {
                secondaryText = String.format("Secondary element none/%d [5, 6, shift-click]", totalElements);
            }
            GlutHudConsole.printTextLine(secondaryText, 0, 2, model.width, model.height);

            int maxHierarchyLevel = 0;
            if ( selectedPatchIndex >= 0 && model.scene != null ) {
                maxHierarchyLevel = GlutDebugPatchHierarchy.maxLevelForSelectedPatch(model.scene, selectedPatchIndex);
            }

            int currentHierarchyLevel = model.selectedHierarchyLevel;
            if ( currentHierarchyLevel < 0 ) {
                currentHierarchyLevel = 0;
            }
            if ( currentHierarchyLevel > maxHierarchyLevel ) {
                currentHierarchyLevel = maxHierarchyLevel;
            }

            String currentLevelLabel;
            if ( selectedPatchIndex < 0 ) {
                currentLevelLabel = "none";
            }
            else if ( currentHierarchyLevel == 0 ) {
                currentLevelLabel = "base";
            }
            else {
                currentLevelLabel = Integer.toString(currentHierarchyLevel);
            }

            String subdivisionText = String.format(
                "Patch subdivision level: %s/%d",
                currentLevelLabel,
                maxHierarchyLevel);
            GlutHudConsole.printTextLine(subdivisionText, 0, 3, model.width, model.height);
        }
    }

    @Override
    public void reshape(GLAutoDrawable drawable, int x, int y, int width, int height) {
        syncModelSizeFromDrawable(drawable);
        syncCameraToViewport();
    }

    public void executeGlutGui(int argc, String[] argv) {
        model.fullScreenApplied = false;

        Runnable createUi = () -> {
            GLProfile profile = GLProfile.get(GLProfile.GL2);
            panelCapabilities = new GLCapabilities(profile);
            panel = createConfiguredPanel();

            frame = new JFrame("RPK");
            frame.setLayout(new BorderLayout());
            frame.add(panel, BorderLayout.CENTER);
            frame.setSize(model.width, model.height);
            frame.setLocation(0, 0);
            frame.setDefaultCloseOperation(WindowConstants.DISPOSE_ON_CLOSE);
            frame.addComponentListener(new ComponentAdapter() {
                @Override
                public void componentResized(ComponentEvent e) {
                    resizeCallback(panel.getWidth(), panel.getHeight());
                }
            });
            frame.addWindowListener(new java.awt.event.WindowAdapter() {
                @Override
                public void windowClosed(java.awt.event.WindowEvent e) {
                    if ( suppressedWindowClosedEvents > 0 ) {
                        suppressedWindowClosedEvents--;
                        return;
                    }
                    if ( fullscreenTransitionInProgress ) {
                        return;
                    }
                    closeLatch.countDown();
                }
            });

            frame.setVisible(true);
            panel.requestFocusInWindow();
            panel.display();
        };

        try {
            if ( SwingUtilities.isEventDispatchThread() ) {
                createUi.run();
            }
            else {
                SwingUtilities.invokeAndWait(createUi);
            }
            closeLatch.await();
        }
        catch ( InterruptedException e ) {
            Thread.currentThread().interrupt();
            printGraphicsSetupError();
        }
        catch ( Exception e ) {
            printGraphicsSetupError();
        }
    }

    private static void printGraphicsSetupError() {
        System.err.println("ERROR: Unable to start the JOGL debug UI. Please check your graphics setup.");
        System.err.println("If you are running on X11, make sure DISPLAY is set and your session has the required permissions.");
    }
}
