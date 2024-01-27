/**
Declarations common to all radiance algorithms

This should migrate to a superclass / interface on an
inheritance hierarchy
*/

#ifndef __RADIANCE__
#define __RADIANCE__

#include <cstdio>

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"

typedef COLOR(*GETRADIANCE_FT)(Patch *patch, double u, double v, Vector3D dir);

/**
Routines to be implemented for each radiance algorithm
*/
class RADIANCEMETHOD {
  public:
    // A one-word name for the method: among others used to select it using
    // the radiance method with the -radiance command line option
    const char *shortName;

    // To how many characters can be abbreviated the short name
    int nameAbbrev;

    // A longer name for the method
    const char *fullName;

    // A function to set default values for the method
    void (*Defaults)();

    // A function to parse command line arguments for the method
    void (*ParseOptions)(int *argc, char **argv);

    // Initializes the current scene for radiance computations. Called when a new
    // scene is loaded or when selecting a particular radiance algorithm
    void (*Initialize)();

    // Does one step or iteration of the radiance computation, typically a unit
    // of computations after which the scene is to be redrawn. Returns TRUE when
    // done
    int (*DoStep)();

    // Terminates radiance computations on the current scene
    void (*Terminate)();

    // Returns the radiance being emitted from the specified patch, at
    // the point with given (u,v) parameters and into the given direction
    GETRADIANCE_FT GetRadiance;

    // Allocates memory for the radiance data for the given patch. Fills in the pointer in patch->radianceData
    Element *(*CreatePatchData)(Patch *patch);

    // Print radiance data for the patch to file out
    void (*PrintPatchData)(FILE *out, Patch *patch);

    // Destroys the radiance data for the patch. Clears the patch->radianceData pointer
    void (*DestroyPatchData)(Patch *patch);

    // Returns a string with statistics information about the current run so far
    char *(*GetStats)();

    // Renders the scene using the specific data. This routine can be
    // a nullptr pointer. In that case, the default hardware assisted rendering
    // method (in render.c) is used: render all the patches with the RGB color
    // triplet they were assigned
    void (*RenderScene)();

    // If defined, this routine will save the current model in VRML format.
    // If not defined, the default method implemented in writevrml.[ch] will
    // be used
    void (*writeVRML)(FILE *fp);
};

// Available radiance methods, terminated with a nullptr pointer
extern RADIANCEMETHOD *GLOBAL_radiance_radianceMethods[];

extern RADIANCEMETHOD *GLOBAL_radiance_currentRadianceMethodHandle;
extern void setRadianceMethod(RADIANCEMETHOD *newMethod, java::ArrayList<Patch *> *scenePatches);
extern void radianceDefaults(java::ArrayList<Patch *> *scenePatches);
extern void parseRadianceOptions(int *argc, char **argv);

#endif
