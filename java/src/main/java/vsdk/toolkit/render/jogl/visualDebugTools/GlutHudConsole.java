package vsdk.toolkit.render.jogl.visualDebugTools;

import com.jogamp.opengl.util.awt.TextRenderer;
import java.awt.Color;
import java.awt.Font;

public final class GlutHudConsole {
    private static final TextRenderer HUD_RENDERER = new TextRenderer(new Font("Monospaced", Font.PLAIN, 14), true, false);

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

        HUD_RENDERER.beginRendering(viewportWidth, viewportHeight);
        HUD_RENDERER.setColor(Color.YELLOW);
        HUD_RENDERER.draw(text, x, y);
        HUD_RENDERER.endRendering();
    }
}
