#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

extern void cameraParseOptions(int *argc, char **argv, Camera *camera, int imageWidth, int imageHeight);

extern void
commandLineGeneralProgramParseOptions(
    int *argc,
    char **argv,
    bool *oneSidedSurfaces,
    int *conicSubDivisions,
    int *imageOutputWidth,
    int *imageOutputHeight);

extern void stochasticRelaxationRadiosityParseOptions(int *argc, char **argv);
extern void randomWalkRadiosityParseOptions(int *argc, char **argv);

#endif
