package vsdk.toolkit.render.jogl.visualDebugTools;

import com.jogamp.opengl.GL2;
import com.jogamp.opengl.GLContext;
import com.jogamp.opengl.fixedfunc.GLMatrixFunc;
import com.jogamp.opengl.util.gl2.GLUT;

public final class GlutHudConsole {
    private static final GLUT GLUT_HELPER = new GLUT();

    private GlutHudConsole() {
    }

    public static void printTextLine(String text, int textColumn, int textLine, int viewportWidth, int viewportHeight) {
        if ( text == null || viewportWidth <= 0 || viewportHeight <= 0 ) {
            return;
        }
        if ( textColumn < 0 ) {
            textColumn = 0;
        }
        if ( textLine < 0 ) {
            textLine = 0;
        }

        int x = 4 + textColumn * 9;
        int y = viewportHeight - (4 + textLine * 16 + 14);

        GLContext currentContext = GLContext.getCurrent();
        if ( currentContext == null || currentContext.getGL() == null ) {
            return;
        }

        GL2 gl;
        try {
            gl = currentContext.getGL().getGL2();
        }
        catch ( RuntimeException e ) {
            return;
        }

        int[] previousMatrixMode = new int[1];
        gl.glGetIntegerv(GLMatrixFunc.GL_MATRIX_MODE, previousMatrixMode, 0);

        gl.glPushAttrib(GL2.GL_ENABLE_BIT | GL2.GL_CURRENT_BIT);
        gl.glMatrixMode(GLMatrixFunc.GL_PROJECTION);
        gl.glPushMatrix();
        gl.glLoadIdentity();
        gl.glOrtho(0.0, viewportWidth, 0.0, viewportHeight, -1.0, 1.0);

        gl.glMatrixMode(GLMatrixFunc.GL_MODELVIEW);
        gl.glPushMatrix();
        gl.glLoadIdentity();

        try {
            gl.glDisable(GL2.GL_LIGHTING);
            gl.glDisable(GL2.GL_TEXTURE_2D);
            gl.glColor3f(1.0f, 1.0f, 0.0f);
            gl.glRasterPos2i(x, y);

            for ( int i = 0; i < text.length(); i++ ) {
                GLUT_HELPER.glutBitmapCharacter(GLUT.BITMAP_9_BY_15, text.charAt(i));
            }
        }
        finally {
            gl.glPopMatrix();
            gl.glMatrixMode(GLMatrixFunc.GL_PROJECTION);
            gl.glPopMatrix();
            gl.glPopAttrib();
            gl.glMatrixMode(previousMatrixMode[0]);
        }
    }
}
