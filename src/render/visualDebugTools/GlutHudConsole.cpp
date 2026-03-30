#include "render/visualDebugTools/GlutHudConsole.h"

#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

static constexpr int HUD_CHAR_WIDTH = 9;
static constexpr int HUD_LINE_HEIGHT = 15;
static constexpr int HUD_PADDING_X = 4;
static constexpr int HUD_PADDING_Y = 4;
static constexpr int HUD_BASELINE_OFFSET = 12;

void
GlutHudConsole::printTextLine(
    const char *text,
    int textColumn,
    int textLine,
    int viewportWidth,
    int viewportHeight)
{
    if ( text == nullptr || viewportWidth <= 0 || viewportHeight <= 0 ) {
        return;
    }

    if ( textColumn < 0 ) {
        textColumn = 0;
    }
    if ( textLine < 0 ) {
        textLine = 0;
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(viewportWidth), static_cast<double>(viewportHeight), 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 0.0f);

    const int x = HUD_PADDING_X + textColumn * HUD_CHAR_WIDTH;
    const int y = HUD_PADDING_Y + textLine * HUD_LINE_HEIGHT + HUD_BASELINE_OFFSET;
    glRasterPos2i(x, y);

    for ( const char *cursor = text; *cursor != '\0'; cursor++ ) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *cursor);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
