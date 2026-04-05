package jogl;

import java.awt.BorderLayout;
import javax.swing.JFrame;
import javax.swing.SwingUtilities;

import com.jogamp.opengl.awt.GLJPanel;
import com.jogamp.opengl.GLProfile;
import com.jogamp.opengl.GLCapabilities;
import com.jogamp.opengl.GL2;
import com.jogamp.opengl.GLAutoDrawable;
import com.jogamp.opengl.GLEventListener;

/**
Simple example program to show how to program OpenGL programs in Java using
JOGL2 library. Current program does not make use of interaction events, and
does not respond to mouse neither keyboard events.
*/
public class HelloWorldJOGL implements GLEventListener 
{
    private JFrame mainWindowWidget;

    /**
    Default constructor.
    */
    public HelloWorldJOGL() 
    {
        createElements();
    }

    /**
    A final method is one that does not accept to have override in subclasses
    */
    public final void createElements()
    {
        //-----------------------------------------------------------------
        GLProfile glp = GLProfile.get(GLProfile.GL2); 
        GLCapabilities caps = new GLCapabilities(glp);
        GLJPanel canvas = new GLJPanel(caps);
        canvas.addGLEventListener(this);

        //-----------------------------------------------------------------
        mainWindowWidget = new JFrame("VITRAL concept test - JOGL Hello World");
        mainWindowWidget.add(canvas, BorderLayout.CENTER);
        mainWindowWidget.pack();
        mainWindowWidget.setSize(640, 480);
        mainWindowWidget.setLocationRelativeTo(null);
        mainWindowWidget.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    }

    /**
    Start Swing thread and show application on screen.
    */
    public final void runApp()
    {
        mainWindowWidget.setVisible(true);
    }

    /** 
    Called by drawable to initiate drawing.
    @param drawable 
    */
    @Override
    public void display(GLAutoDrawable drawable) {
        GL2 gl = drawable.getGL().getGL2();

        gl.glClearColor(0, 0, 0, 1);
        gl.glClear(GL2.GL_COLOR_BUFFER_BIT);
        gl.glColor3d(1, 1, 1);

        gl.glMatrixMode(GL2.GL_PROJECTION);
        gl.glLoadIdentity();
        gl.glMatrixMode(GL2.GL_MODELVIEW);
        gl.glLoadIdentity();

        gl.glBegin(GL2.GL_LINES);
            gl.glVertex3d(0, 0, 0);
            gl.glVertex3d(0.5, 0.5, 0);
        gl.glEnd();
    }

    /**
    Not used method, but need to instance GLEventListener.
    @param drawable
    */
    @Override
    public void init(GLAutoDrawable drawable) {

    }

    /** 
    Not used method, but need to instance GLEventListener.
    @param drawable
    */
    @Override
    public void dispose(GLAutoDrawable drawable) {

    }

    /**
    Called to indicate the drawing surface has been moved and/or resized
    @param drawable
    @param x
    @param y
    @param width
    @param height
    */
    @Override
    public void reshape(
        GLAutoDrawable drawable,
        int x,
        int y,
        int width,
        int height) 
    {
        GL2 gl = drawable.getGL().getGL2();
        gl.glViewport(0, 0, width, height);
    }

    /**
    Program entry point for applications.
    @param args 
    */
    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            HelloWorldJOGL instance = new HelloWorldJOGL();
            instance.runApp();
        });
    }

}
