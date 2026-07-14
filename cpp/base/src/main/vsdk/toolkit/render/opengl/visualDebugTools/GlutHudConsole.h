#ifndef VISUAL_DEBUG_TOOLS_GLUT_HUD_CONSOLE__
#define VISUAL_DEBUG_TOOLS_GLUT_HUD_CONSOLE__

class GlutHudConsole final {
  public:
    static void printTextLine(
        const char *text,
        int textColumn,
        int textLine,
        int viewportWidth,
        int viewportHeight);

  private:
    static constexpr int HUD_CHAR_WIDTH = 9;
    static constexpr int HUD_LINE_HEIGHT = 15;
    static constexpr int HUD_PADDING_X = 4;
    static constexpr int HUD_PADDING_Y = 4;
    static constexpr int HUD_BASELINE_OFFSET = 12;
};

#endif
