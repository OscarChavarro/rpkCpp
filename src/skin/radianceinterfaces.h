/* radiance.h: declarations common to all radiance algorithms */

#ifndef _RADIANCE_H_
#define _RADIANCE_H_

#include <cstdio>

#include "skin/Geometry.h"

typedef COLOR(*GETRADIANCE_FT)(PATCH *patch, double u, double v, Vector3D dir);

/* routines to be implemented for each radiance algorithm */
class RADIANCEMETHOD {
  public:
    /* A one-word name for the method: among others used to select it using
     * the radiance method with the -radiance command line option. */
    const char *shortName;

    /* to how many characters can be abbreviated the short name */
    int nameAbbrev;

    /* a longer name for the method */
    const char *fullName;

    /* a function to set default values for the method */
    void (*Defaults)();

    /* a function to parse command line arguments for the method. */
    void (*ParseOptions)(int *argc, char **argv);

    /* a function to print command line arguments for the method, invoked e.g.
     * using 'rpk -help' */
    void (*PrintOptions)(FILE *fp);

    /* Initializes the current scene for radiance computations. Called when a new
     * scene is loaded or when selecting a particular radiance algorithm. */
    void (*Initialize)();

    /* Does one step or iteration of the radiance computation, typically a unit
     * of computations after which the scene is to be redrawn. Returns TRUE when
     * done. */
    int (*DoStep)();

    /* Terminates radiance computations on the current scene */
    void (*Terminate)();

    /* Returns the radiance being emitted from the specified patch, at
     * the point with given (u,v) parameters and into the given direction. */
    /* COLOR (*GetRadiance)(PATCH *patch, double u, double v, Vector3D dir); */
    GETRADIANCE_FT GetRadiance;

    /* Allocates memory for the radiance data for the given patch. Fills in
    * the pointer in patch->radiance_data. */
    void *(*CreatePatchData)(PATCH *patch);

    /* Print radiance data for the patch to file out */
    void (*PrintPatchData)(FILE *out, PATCH *patch);

    /* destroys the radiance data for the patch. Clears the patch->radiance_data
     * pointer. */
    void (*DestroyPatchData)(PATCH *patch);

    /* returns a string with statistics information about the current run so
     * far */
    char *(*GetStats)();

    /* Renders the scene using the specific data. This routine can be
     * a nullptr pointer. In that case, the default hardware assisted rendering
     * method (in render.c) is used: render all the patches with the RGB color
     * triplet they were assigned. */
    void (*RenderScene)();

    /* routine for recomputing display colors if default
     * RenderScene method is being used. */
    void (*RecomputeDisplayColors)();

    /* called when a material has been updated. The radiance method recomputes
     * the color of the affected surfaces and also of the other
     * surfaces, whose colors change because of interreflections. */
    void (*UpdateMaterial)(Material *oldmaterial, Material *newmaterial);

    /* If defined, this routine will save the current model in VRML format.
     * If not defined, the default method implemented in writevrml.[ch] will
     * be used. */
    void (*WriteVRML)(FILE *fp);
};

/* able of available radiance methods, terminated with a nullptr pointer. */
extern RADIANCEMETHOD *RadianceMethods[];

/* iterator over all available radiance methods */
#define ForAllRadianceMethods(methodp) {{        \
  RADIANCEMETHOD **methodpp;                \
  for (methodpp=RadianceMethods; *methodpp; methodpp++) { \
    RADIANCEMETHOD *methodp = *methodpp;

/* pointer to current radiance method handle */
extern RADIANCEMETHOD *Radiance;

/* This routine sets the current radiance method to be used + initializes */
extern void SetRadianceMethod(RADIANCEMETHOD *newmethod);

/* Initializes. Called from Init() in main.c. */
extern void RadianceDefaults();

/* Parses (and consumes) command line options for radiance
 * computation. */
extern void ParseRadianceOptions(int *argc, char **argv);

#endif
