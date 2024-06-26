#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

#include "raycasting/common/Raytracer.h"
#include "app/BatchOptions.h"

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
extern void rayMattingParseOptions(int *argc, char **argv);
extern void stochasticRayTracerParseOptions(int *argc, char **argv);
extern void biDirectionalPathParseOptions(int *argc, char **argv);
extern void photonMapParseOptions(int *argc, char **argv);
extern void toneMapParseOptions(int *argc, char **argv, char *toneMapName);
extern void radianceMethodParseOptions(int *argc, char **argv, char *radianceMethodsString);
extern void renderParseOptions(int *argc, char **argv, RenderOptions *renderOptions);
extern void batchParseOptions(int *argc, char **argv, BatchOptions *batchOptions);

extern void
rayTracingParseOptions(
    int *argc,
    char **argv,
    char raytracingMethodsString[],
    char *rayTracerName);

extern void
galerkinParseOptions(int *argc, char **argv);

#endif
