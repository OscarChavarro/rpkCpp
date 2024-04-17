#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

extern void parseCameraOptions(int *argc, char **argv, Camera *camera, int imageWidth, int imageHeight);
extern void parseGeneralProgramOptions(int *argc, char **argv, bool *oneSidedSurfaces, int *conicSubDivisions);

#endif
