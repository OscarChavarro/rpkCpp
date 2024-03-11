#ifndef __RADIANCE_METHOD__
#define __RADIANCE_METHOD__

#include <cstdio>

#include "java/util/ArrayList.h"
#include "skin/Patch.h"

class RadianceMethod {
  public:
    RadianceMethod();
    virtual ~RadianceMethod();

    virtual void defaultValues() = 0;

    virtual void parseOptions(int *argc, char **argv) = 0;

    virtual void initialize(java::ArrayList<Patch *> *scenePatches) = 0;

    // Does one step or iteration of the radiance computation, typically a unit
    // of computations after which the scene is to be redrawn. Returns TRUE when
    // done
    virtual int doStep(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) = 0;

    virtual void terminate(java::ArrayList<Patch *> *scenePatches) = 0;

    virtual COLOR getRadiance(Patch *patch, double u, double v, Vector3D dir) = 0;

    virtual Element *createPatchData(Patch *patch) = 0;

    virtual void destroyPatchData(Patch *patch) = 0;

    virtual char *getStats() = 0;

    virtual void renderScene(java::ArrayList<Patch *> *scenePatches) = 0;

    virtual void writeVRML(FILE *fp) = 0;
};

class RADIANCEMETHOD {
  public:
    // A one-word name for the method: among others used to select it using
    // the radiance method with the -radiance command line option
    const char *shortName;

    // To how many characters can be abbreviated the short name
    int shortNameMinimumLength;

    // A longer name for the method
    const char *fullName;

    // A function to set default values for the method
    void (*defaultValues)();

    // A function to parse command line arguments for the method
    void (*parseOptions)(int *argc, char **argv);

    // Initializes the current scene for radiance computations. Called when a new
    // scene is loaded or when selecting a particular radiance algorithm
    void (*initialize)(java::ArrayList<Patch *> *scenePatches);

    // Terminates radiance computations on the current scene
    void (*terminate)(java::ArrayList<Patch *> *scenePatches);

    // Returns the radiance being emitted from the specified patch, at
    // the point with given (u,v) parameters and into the given direction
    COLOR(*getRadiance)(Patch *patch, double u, double v, Vector3D dir);

    // Allocates memory for the radiance data for the given patch. Fills in the pointer in patch->radianceData
    Element *(*createPatchData)(Patch *patch);

    // Destroys the radiance data for the patch. Clears the patch->radianceData pointer
    void (*destroyPatchData)(Patch *patch);

    // Returns a string with statistics information about the current run so far
    char *(*getStats)();

    // Renders the scene using the specific data. This routine can be
    // a nullptr pointer. In that case, the default hardware assisted rendering
    // method (in render.c) is used: render all the patches with the RGB color
    // triplet they were assigned
    void (*renderScene)(java::ArrayList<Patch *> *scenePatches);

    // If defined, this routine will save the current model in VRML format.
    // If not defined, the default method implemented in write vrml.[ch] will
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
