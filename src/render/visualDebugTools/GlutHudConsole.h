#ifndef __VISUAL_DEBUG_TOOLS_GLUT_HUD_CONSOLE__
#define __VISUAL_DEBUG_TOOLS_GLUT_HUD_CONSOLE__

class GlutHudConsole final {
  public:
    static void printTextLine(
        const char *text,
        int textColumn,
        int textLine,
        int viewportWidth,
        int viewportHeight);
};

#endif
