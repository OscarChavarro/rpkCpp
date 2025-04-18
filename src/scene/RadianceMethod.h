#ifndef __RADIANCE_METHOD__
#define __RADIANCE_METHOD__

#include <cstdio>

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "skin/Patch.h"
#include "scene/Camera.h"
#include "scene/Scene.h"
#include "scene/RadianceMethodAlgorithm.h"

class RadianceMethod {
  public:
    RadianceMethodAlgorithm className;

    RadianceMethod();
    virtual ~RadianceMethod();

    virtual const char *getRadianceMethodName() const = 0;

    // A function to parse command line arguments for the method
    virtual void parseOptions(int *argc, char **argv) = 0;

    // Initializes the current scene for radiance computations. Called when a new
    // scene is loaded or when selecting a particular radiance algorithm
    virtual void initialize(Scene *scene) = 0;

    // Does one step or iteration of the radiance computation, typically a unit
    // of computations after which the scene is to be redrawn. Returns TRUE when
    // done
    virtual bool
    doStep(Scene *scene, RenderOptions *renderOptions) = 0;

    // Terminates radiance computations on the current scene
    virtual void terminate(java::ArrayList<Patch *> *scenePatches) = 0;

    // Returns the radiance being emitted from the specified patch, at
    // the point with given (u,v) parameters and into the given direction
    virtual ColorRgb getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, const RenderOptions *renderOptions) const = 0;

    // Allocates memory for the radiance data for the given patch. Fills in the pointer in patch->radianceData
    virtual Element *createPatchData(Patch *patch) = 0;

    // Destroys the radiance data for the patch. Clears the patch->radianceData pointer
    virtual void destroyPatchData(Patch *patch) = 0;

    // Returns a string with statistics information about the current run so far
    virtual char *getStats() = 0;

    // Renders the scene using the specific data. This routine can be
    // a nullptr pointer. In that case, the default hardware assisted rendering
    // method (in render.c) is used: render all the patches with the RGB color
    // triplet they were assigned
    virtual void renderScene(const Scene *scene, const RenderOptions *renderOptions) const = 0;

    // If defined, this routine will save the current model in VRML format.
    // If not defined, the default method implemented in write vrml.[ch] will
    // be used
    virtual void writeVRML(const Camera *camera, FILE *fp, const RenderOptions *renderOptions) const  = 0;
};

#endif
